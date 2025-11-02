# Bug #7: 提醒播报音频问题

## 问题描述

**现象**：定时器触发后，需要小智自动播报提醒内容，但目前只能唤醒，无法自动发送提醒消息。

**时间**：2025-11-02

**相关文件**：
- `main/application.cc` - 提醒触发和唤醒逻辑
- `bug_handoff_reminder.md` - 之前修复的5个bug

---

## 当前状态

### ✅ Phase 1：唤醒测试 - 已完成并验证

**实现代码**（`main/application.cc` 约720-735行）：
```cpp
std::string wake_word = "你好小智";
ESP_LOGI(TAG, "[PHASE 1 TEST] Sending wake word: %s", wake_word.c_str());
protocol_->SendWakeWordDetected(wake_word);

ListeningMode mode = aec_mode_ == kAecOff ? kListeningModeAutoStop : kListeningModeRealtime;
SetListeningMode(mode);

ESP_LOGI(TAG, "[PHASE 1 TEST] Reminder message not sent yet: '%s'", pending_reminder_message_.c_str());

// 清空缓存（测试阶段不发送提醒）
pending_reminder_message_.clear();
```

**最新测试日志**（2025-11-02 运行日志）：
```
I (36654) Application: [PHASE 1 TEST] Sending wake word: 你好小智
I (36654) Application: STATE: listening
I (36684) Application: >> Hi, 小智          ← ✅ 唤醒成功！云端识别了
I (37464) Application: << 嗨～在的喔！      ← ✅ 小智回复
I (39314) Application: << 今天要来点啥～？   ← ✅ 等待用户说话
I (41174) Application: STATE: listening      ← ✅ 进入listening状态
```

**验证结果**：
- ✅ 唤醒**完全成功**
- ✅ 云端识别并回复（`>> Hi, 小智` 证明云端处理了）
- ✅ Session创建正常
- ✅ 进入listening状态，等待"用户"输入

**Phase 1 结论**：✅ **唤醒方案100%可行**

---

### ⏳ Phase 2：自动发送提醒消息 - **这是当前的任务**

**目标**：唤醒成功后，**自动构建并发送提醒请求**给云端

**当前问题**：
- ✅ 唤醒成功 → 进入listening状态
- ❌ **没有发送提醒消息** → `pending_reminder_message_` 被清空了
- ❌ 小智在等待"用户"说话 → 但我们需要程序自动"说话"

**需要做的事**：
1. ✅ 唤醒小智（已完成）
2. ⏳ **等待唤醒TTS播放完成**（监听 TTS:STOP 事件）
3. ⏳ **自动发送请求**：`"提醒时间到了，吃药"`
4. ⏳ 云端AI理解并回复
5. ⏳ 播放回复音频

**完整流程**：
```
定时器触发（10秒）
    ↓
发送唤醒词 "你好小智"
    ↓
云端识别 >> Hi, 小智
    ↓
云端回复 << 嗨～在的喔！今天要来点啥～？
    ↓
TTS:STOP 事件触发 ← 【关键时机】
    ↓
自动发送请求 "提醒时间到了，吃药"
    ↓
云端识别 >> 提醒时间到了，吃药
    ↓
AI回复 << 该吃药啦～
    ↓
播放音频 ✅
```

---

## 技术细节

### 如何发送请求给云端？

我们需要使用 `SendListeningResult` 函数发送文本消息：

**函数定义**（`protocol.cc:81-94`）：
```cpp
void Protocol::SendListeningResult(const std::string& text) {
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "session_id", session_id_.c_str());
    cJSON_AddStringToObject(root, "type", "listen");
    cJSON_AddStringToObject(root, "state", "result");
    cJSON_AddStringToObject(root, "text", text.c_str());

    char* json = cJSON_PrintUnformatted(root);
    SendText(json);
    cJSON_free(json);
    cJSON_Delete(root);
}
```

**发送的消息格式**：
```json
{
  "session_id": "a70a8047",
  "type": "listen",
  "state": "result",
  "text": "提醒时间到了，吃药"
}
```

**调用方法**：
```cpp
protocol_->SendListeningResult("提醒时间到了，吃药");
```

### 关键日志标记

理解这些日志很重要：

- `>> xxx` - **云端STT识别结果**（`application.cc:535`）
  - 这证明云端收到并处理了消息
  - 例：`>> Hi, 小智` 或 `>> 提醒时间到了，吃药`

- `<< xxx` - **云端AI回复文本**（`application.cc:525`）
  - 这是AI的回复内容
  - 例：`<< 嗨～在的喔！` 或 `<< 该吃药啦～`

- `STATE: xxx` - **设备状态变化**
  - `listening` - 等待用户说话
  - `speaking` - 播放TTS
  - `idle` - 空闲

### 成功的完整日志示例

Phase 2成功后应该看到：
```
I (36654) Application: [PHASE 1 TEST] Sending wake word: 你好小智
I (36654) Application: STATE: listening
I (36684) Application: >> Hi, 小智
I (37464) Application: << 嗨～在的喔！
I (39314) Application: << 今天要来点啥～？
I (41174) Application: STATE: listening

← 【这里发送提醒消息】

I (41200) Application: >> 提醒时间到了，吃药  ← ✅ 关键！证明云端收到
I (41300) Application: STATE: speaking
I (41500) Application: << 该吃药啦～          ← ✅ AI回复
← 【播放音频】
```

### 重要疑问 ⚠️

**`SendListeningResult` 能否工作？**

- 正常流程：用户说话 → 发送音频流 → 云端STT识别 → 返回识别结果
- 我们的流程：**直接发送文本** → 没有音频流

**可能的问题**：
- 云端可能检查是否有对应的音频流
- 如果没有音频，可能忽略 `SendListeningResult`
- **需要测试验证**

---

## Phase 2 实现方案 ⭐

### 推荐方案：在TTS:STOP后发送消息

**核心思路**：
1. 提醒触发时，**保存消息**到 `pending_reminder_message_`（不清空）
2. 添加一个flag `is_reminder_pending_`，标记有待发送的提醒
3. 发送唤醒词
4. **监听TTS:STOP事件**（唤醒TTS播放完成）
5. 在TTS:STOP处理中，检查flag并发送消息

### 具体实现步骤

#### 步骤1：添加flag变量

**文件**：`main/application.h` (约94行)

**修改**：
```cpp
// 提醒管理器
ReminderManager reminder_manager_;
std::string pending_reminder_message_;  // 待触发的提醒消息
bool is_reminder_pending_ = false;      // ← 添加这一行
```

#### 步骤2：提醒触发时设置flag

**文件**：`main/application.cc` (约720-735行)

**当前代码**：
```cpp
ESP_LOGI(TAG, "[PHASE 1 TEST] Sending wake word: %s", wake_word.c_str());
protocol_->SendWakeWordDetected(wake_word);

ListeningMode mode = aec_mode_ == kAecOff ? kListeningModeAutoStop : kListeningModeRealtime;
SetListeningMode(mode);

ESP_LOGI(TAG, "[PHASE 1 TEST] Reminder message not sent yet: '%s'", pending_reminder_message_.c_str());

// 清空缓存（测试阶段1不发送提醒）
pending_reminder_message_.clear();  // ← 删除这行
```

**修改为**：
```cpp
ESP_LOGI(TAG, "[REMINDER] Sending wake word: %s", wake_word.c_str());

// 设置flag：有待发送的提醒
is_reminder_pending_ = true;

protocol_->SendWakeWordDetected(wake_word);

ListeningMode mode = aec_mode_ == kAecOff ? kListeningModeAutoStop : kListeningModeRealtime;
SetListeningMode(mode);

ESP_LOGI(TAG, "[REMINDER] Waiting for wake TTS to complete, then will send: '%s'", pending_reminder_message_.c_str());
```

#### 步骤3：在TTS:STOP中发送消息

**文件**：`main/application.cc` (约500-509行)

**当前代码**：
```cpp
} else if (strcmp(state->valuestring, "stop") == 0) {
    Schedule([this]() {
        if (device_state_ == kDeviceStateSpeaking) {
            if (listening_mode_ == kListeningModeManualStop) {
                SetDeviceState(kDeviceStateIdle);
            } else {
                SetDeviceState(kDeviceStateListening);
            }
        }
    });
}
```

**修改为**：
```cpp
} else if (strcmp(state->valuestring, "stop") == 0) {
    Schedule([this]() {
        if (device_state_ == kDeviceStateSpeaking) {
            if (listening_mode_ == kListeningModeManualStop) {
                SetDeviceState(kDeviceStateIdle);
            } else {
                SetDeviceState(kDeviceStateListening);
            }
        }

        // 检查是否有待发送的提醒消息
        if (is_reminder_pending_ && !pending_reminder_message_.empty()) {
            ESP_LOGI(TAG, "[REMINDER] Wake TTS completed, sending reminder now");

            // 构造完整消息
            std::string msg = "提醒时间到了，" + pending_reminder_message_;
            protocol_->SendListeningResult(msg);

            ESP_LOGI(TAG, "[REMINDER] Sent: '%s'", msg.c_str());

            // 清除flag和消息
            is_reminder_pending_ = false;
            pending_reminder_message_.clear();
        }
    });
}
```

### 代码修改总结

需要修改2个文件，3个位置：

1. **application.h** - 添加1行：`bool is_reminder_pending_ = false;`
2. **application.cc 提醒触发处** - 修改3行：
   - 添加 `is_reminder_pending_ = true;`
   - 删除 `pending_reminder_message_.clear();`
   - 修改日志
3. **application.cc TTS:STOP处** - 添加10行：检查flag并发送消息

---

## 测试验证

### 测试步骤

1. **实现Phase 2代码**（按照上面的步骤）
2. **编译固件**
3. **烧录到设备**
4. **设置提醒**："10秒后提醒我吃药"
5. **等待10秒**
6. **查看日志**

### 成功的标志

如果成功，日志应该显示：
```
I (xxx) Application: [REMINDER] Sending wake word: 你好小智
I (xxx) Application: [REMINDER] Waiting for wake TTS to complete, then will send: '吃药'
I (xxx) Application: STATE: listening
I (xxx) Application: >> Hi, 小智
I (xxx) Application: << 嗨～在的喔！
I (xxx) Application: << 今天要来点啥～？
I (xxx) Application: STATE: listening
I (xxx) Application: [REMINDER] Wake TTS completed, sending reminder now
I (xxx) Application: [REMINDER] Sent: '提醒时间到了，吃药'
I (xxx) Application: >> 提醒时间到了，吃药  ← ✅ 关键！
I (xxx) Application: STATE: speaking
I (xxx) Application: << 该吃药啦～
```

### 如果失败

如果没有看到 `>> 提醒时间到了，吃药`，说明：
- 云端忽略了无音频的 `SendListeningResult` 消息
- 需要尝试其他方案（发送假音频流或使用MCP消息）

---

## 总结

### 当前状态
- ✅ **Phase 1完成**：唤醒成功，session创建正常
- ⏳ **Phase 2待实现**：自动发送提醒消息

### 核心问题
我们需要让设备在唤醒后**自动"说话"**（发送请求），模拟用户说"提醒时间到了，吃药"。

### 解决方案
在TTS:STOP事件中，调用 `SendListeningResult` 发送文本消息。

### 不确定因素
云端是否接受无音频流的文本消息？需要测试验证。

---

**文档版本**：v2.0
**状态**：Phase 1 ✅ 完成 | Phase 2 ⏳ 待实现
**最后更新**：2025-11-02
**下一步**：实现Phase 2代码并测试
