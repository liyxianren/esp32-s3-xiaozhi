# 提醒播报问题交接说明

## 背景
- 功能：定时提醒（`reminder.add`）触发后由云端小智播报。
- 当前表现：倒计时结束后设备能够发送唤醒词和伪造的识别文本，但云端未返回提醒内容；串口只看到“小智在啦，干嘛～”等唤醒回应。
- 影响：用户无法在设定时间听到小智播报的提醒。

## 现状概述
1. **设备端流程（`main/application.cc`）**
   - `ReminderManager` 正常创建 FreeRTOS 定时器，触发后通过事件队列通知主循环。
   - `MainEventLoop` 在 idle 状态下执行：
     1. 发送固定唤醒词 `小智小智`。
     2. 设置 `listening_mode_ = kListeningModeManualStop`，进入 `STATE::listening`。
     3. 按 `listen-start → listen-result → listen-stop` 顺序模拟文本（示例：“提醒时间到了时间到！该吃药啰～别拿糖果假装哦”）。
     4. 清空 `pending_reminder_message_` 并回到 `STATE::idle`。
   - 相关代码：`main/application.cc` 约 648–716 行，`SetDeviceState()` 中对 idle 的重调度逻辑。

2. **协议层改动**
   - 新增 `Protocol::SendListeningResult()`（`main/protocols/protocol.h/.cc`），负责封装 `listen-result` 报文。

3. **日志表现**
   - 定时器触发后能看到：
     ```
     Application: 🎙️ Sending wake word: 小智小智
     Application: 🗣️ Simulated reminder utterance: 提醒时间到了...
     Application: ✅ Reminder request delivered to cloud
     Application: STATE: idle
     ```
   - 随后云端仅回复“小智在啦，干嘛～”，没有后续提醒。

## 复现步骤
1. 构建并刷入当前分支（`feature/reminder-mcp`）。
2. 连接串口 `idf.py -p <PORT> flash monitor`。
3. 对话：
   ```
   小智，小智
   提醒我10秒后吃药
   ```
4. 观察 10 秒后串口输出与云端对话记录。

## 期望 vs 结果
- **期望**：云端在收到 `listen-result` 后回复类似“时间到了，该吃药了”，设备播放该语音。
- **实际**：云端只返回唤醒问候或无响应，提醒文本没有被处理。

## 已尝试的方案
| 时间 | 方案 | 结果 |
| ---- | ---- | ---- |
| 初始 | 直接 `SendWakeWordDetected("提醒我…")` | 云端认为是 detect 文本，返回错误。 |
| v2   | 唤醒词后直接 `SendListeningResult()` 并保持 speaking 状态 | 云端未播报提醒。 |
| v3   | 当前方案：唤醒词 + listen-start/result/stop，并清空 pending | 云端仍未播报提醒，只返回唤醒问候。 |

## 可能原因猜测
1. 云端需要一段真实的音频流或更长等待时间才能接受 `listen-result`；目前只发送 JSON，没有推语音包。
2. 发送顺序、字段可能仍不符合小智服务协议（比如缺少 `listen-start` 参数或 session 状态不匹配）。
3. 发送唤醒词后立即返回 idle，可能导致云端认为会话已结束。

## 建议的下一步
1. 核对云端 listen 接口的协议要求：是否必须有音频流或者特定字段（例如 `channel`、`mode` 等）。
2. 试验在发送 `listen-result` 前等待更长时间或补发空音频包，确认云端对“纯文本”是否支持。
3. 通过云端日志或模拟器确认收到的 JSON 内容；必要时与云端团队沟通查看 session 状态。
4. 如果协议允许，可考虑让云端直接调用一个“提醒播报” MCP 工具，避免模拟语音。

## 相关文件列表
- `main/application.cc`（事件循环、提醒播报逻辑）
- `main/protocols/protocol.h/.cc`（新增 `SendListeningResult`）
- 文档：`完整架构文档.md`、`实施文档.md`（已描述当前设计与待完成事项）

## 当前分支与未提交文件
- 分支：`feature/reminder-mcp`
- 未提交变更（`git status`）:
  ```
  M main/application.cc
  M main/protocols/protocol.cc
  M main/protocols/protocol.h
  ...（提醒工具、文档等改动）
  ```

请在后续修改前基于以上信息继续排查。