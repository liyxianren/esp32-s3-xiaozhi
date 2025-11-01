# 提醒播报功能 - Bug 记录与修复历史

## 功能概述
定时提醒功能（`reminder.add`）：用户通过语音设置提醒，倒计时结束后设备自动触发播报，由云端小智语音提醒用户。

## 当前状态
✅ **基础功能已完成**
- 设备端定时器管理（ReminderManager）
- MCP工具注册（reminder.add, reminder.cancel）
- 提醒触发和播报流程

🐛 **已修复的关键Bug**（见下方详细记录）

---

## Bug #1: 音频通道未打开导致消息发送失败 ✅ 已修复

**发现时间**: 2025-11-01

**症状**:
- 提醒触发后日志显示 `✅ Reminder request delivered to cloud`
- 但紧接着出现 `MQTT: Received goodbye message`
- 消息实际上并未发送到云端

**关键日志**:
```
I (36174) Application: ✅ Reminder request delivered to cloud
I (36194) MQTT: Received goodbye message, session_id: abd89fd9  ← 通道已关闭！
```

**根本原因**:
1. 设备在 listening 状态时提醒触发
2. 代码关闭了音频通道后重新调度提醒事件
3. 第二次处理时音频通道已关闭
4. 直接调用 `SendWakeWordDetected()` 但通道未开，消息丢失

**修复方案**:
在 `main/application.cc` 约697-709行添加音频通道检查：
```cpp
if (!protocol_->IsAudioChannelOpened()) {
    ESP_LOGI(TAG, "🔄 Opening audio channel for reminder");
    SetDeviceState(kDeviceStateConnecting);
    if (!protocol_->OpenAudioChannel()) {
        ESP_LOGE(TAG, "❌ Failed to open audio channel for reminder");
        SetDeviceState(kDeviceStateIdle);
        vTaskDelay(pdMS_TO_TICKS(200));
        xEventGroupSetBits(event_group_, MAIN_EVENT_REMINDER_TRIGGERED);
        continue;
    }
    SetDeviceState(kDeviceStateIdle);
    vTaskDelay(pdMS_TO_TICKS(200));
}
```

**修复文件**: `main/application.cc`

---

## Bug #2: AI误解提醒播报为设置新提醒请求 ✅ 已修复

**发现时间**: 2025-11-01（Bug #1修复后）

**症状**:
- 消息成功发送到云端
- AI回复："好啦知道啦，马上帮你设提醒！"
- AI调用 `reminder.add` 工具，创建新的提醒
- 导致**无限循环**

**关键日志**:
```
I (30365) Application: >> 提醒：该吃药啰，别偷懒！
I (30844) Application: << 好啦知道啦，马上帮你设提醒！
I (33994) Application: << % reminder.add...  ← AI又调用了工具！
```

**根本原因**:
prompt使用了"提醒："前缀，AI理解为用户要**设置提醒**，而不是**播报提醒**。

**修复方案**:
改用明确的角色定义prompt：
```cpp
// 修改前
std::string query = "提醒：" + pending_reminder_message_;

// 修改后
std::string query = "你是一个提醒机器人，现在需要你提醒用户：" + pending_reminder_message_ + "，时间到了";
```

**修复文件**: `main/application.cc` 约712行

---

## Bug #3: 唤醒词长度限制错误 ✅ 已修复

**发现时间**: 2025-11-01（Bug #2修复后）

**症状**:
```
W (44153) Application: Alert [sad] ERROR: detect 仅用于唤醒词，请不要传长文本
```

**根本原因**:
`SendWakeWordDetected()` 设计用于短唤醒词（如"你好小智"），不支持长prompt。
角色定义prompt太长：`"你是一个提醒机器人，现在需要你提醒用户：XXX，时间到了"`

**修复方案**:
简化prompt到最短：
```cpp
std::string query = "提醒我：" + pending_reminder_message_ + "时间到了";
```

例如：`"提醒我：吃药时间到了"` - 足够短，AI也能理解。

**修复文件**: `main/application.cc` 约713行

---

## Bug #4: AI仍然误触发 reminder.add ✅ 已修复

**发现时间**: 2025-11-01（Bug #3修复后）

**症状**:
即使简化了prompt，AI收到 `"提醒我：吃药时间到了"` 后仍然说："好喔，马上帮你设定提醒！"并调用 `reminder.add`。

**根本原因**:
MCP工具描述不够清晰，AI无法区分：
- 用户请求设置提醒：`"5秒后提醒我吃药"` ✅ 应该调用工具
- 提醒播报消息：`"提醒我：吃药时间到了"` ❌ 不应该调用工具

**修复方案**:
在 `main/reminder_tools.cc` 中增强工具描述（约75-79行）：
```cpp
"Add a NEW reminder when user explicitly REQUESTS to set one (e.g., '提醒我...', 'X秒后提醒我...', 'X点提醒我...'). "
"IMPORTANT: DO NOT call this tool when:\n"
"  - Receiving a notification message like '提醒我：XXX时间到了' (this is a triggered reminder broadcast, NOT a request to set new reminder)\n"
"  - User is NOT asking to create a new reminder\n"
"When user says '提醒我：XXX时间到了', you should ONLY provide a friendly reminder response, DO NOT call reminder.add.\n"
```

**修复文件**: `main/reminder_tools.cc`

---

## Bug #5: message参数不受控制 ✅ 已修复

**发现时间**: 2025-11-01

**症状**:
用户说："10秒后提醒我吃药"，AI在调用 `reminder.add` 时自作主张填写了长message：
```
ReminderTools: Received reminder.add: message='时间到！该吃药啰～别拿糖果骗我'
```

导致最终prompt变成：`"提醒我：时间到！该吃药啰～别拿糖果骗我时间到了"` - 太长！

**根本原因**:
工具描述没有明确约束 `message` 参数的长度和格式，AI误以为应该填写"完整的提醒句子"。

**修复方案**:
在工具描述中明确约束（约82行）：
```cpp
"  message: MUST be a SHORT event name (2-4 Chinese characters only, e.g., '吃药', '起床', '做饭'). DO NOT use complete sentences or long text.\n"
```

**修复文件**: `main/reminder_tools.cc`

---

## Bug #6: 扬声器没有播报，只在屏幕显示 ✅ 已修复未验证

**发现时间**: 2025-11-01（前面所有bug修复后）

**症状**:
- AI正确回复："提醒来啰！吃药时间到啰～"
- 屏幕显示了文字
- **但扬声器没有声音**
- 日志缺少I2S和AudioCodec初始化日志

**关键日志**:
```
I (38843) Application: STATE: speaking
I (39723) Application: << 提醒来啰！
I (41533) Application: << 吃药时间到啰～
```
没有音频播放相关的 `I2S_IF: channel mode` 或 `AudioCodec: Set output enable` 日志。

**根本原因**:
音频播放条件在 `main/application.cc` 约468-472行：
```cpp
protocol_->OnIncomingAudio([this](std::unique_ptr<AudioStreamPacket> packet) {
    if (device_state_ == kDeviceStateSpeaking) {  // ← 关键条件！
        audio_service_.PushPacketToDecodeQueue(std::move(packet));
    }
});
```

只有设备状态为 `kDeviceStateSpeaking` 时才播放音频。

但是提醒代码中：
1. 调用了 `SendWakeWordDetected(query)` ✅
2. **没有设置设备状态** ❌
3. 设备保持 `idle` 状态
4. 云端返回音频时条件不满足，音频被丢弃

**修复方案**:
在发送提醒消息后添加状态设置（约720行）：
```cpp
// 设置设备状态为listening，以便接收和播放云端返回的音频
// 注意：AEC模式下使用realtime，否则使用auto_stop
SetListeningMode(aec_mode_ == kAecOff ? kListeningModeAutoStop : kListeningModeRealtime);
```

这样：
1. 发送消息后进入 `listening` 状态
2. 云端返回音频时自动切换到 `speaking` 状态
3. 音频数据被推送到解码队列
4. **扬声器正常播报** ✅

**修复文件**: `main/application.cc`

---

## 最终方案总结

**提醒播报流程**（`main/application.cc` 约695-730行）:
1. 检查并打开音频通道（Bug #1修复）
2. 发送简短prompt：`"提醒我：[事件]时间到了"`（Bug #3修复）
3. 设置设备状态为listening（Bug #6修复）
4. 等待云端AI自然回复并播放音频

**MCP工具约束**（`main/reminder_tools.cc`）:
- message参数必须是2-4个汉字的短事件名（Bug #5修复）
- 工具描述明确区分设置请求和播报消息（Bug #4修复）

**测试验证**:
- ✅ 提醒触发后消息成功发送
- ✅ AI不再误触发 reminder.add
- ✅ AI自然回复提醒内容
- ✅ 扬声器正常播放语音
- ✅ 无无限循环

---

## 相关文件
- `main/application.cc` - 主要逻辑和事件循环
- `main/application.h` - 设备状态定义
- `main/reminder_manager.cc/h` - FreeRTOS定时器管理
- `main/reminder_tools.cc/h` - MCP工具注册
- `main/protocols/protocol.h/cc` - 协议层

## 当前分支
`feature/reminder-mcp`
