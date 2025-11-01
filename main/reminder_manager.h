#ifndef REMINDER_MANAGER_H
#define REMINDER_MANAGER_H

#include <string>
#include <vector>
#include <functional>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <esp_log.h>

// 前向声明
class ReminderManager;

/**
 * 定时器上下文结构
 * 用于在静态回调函数中访问 ReminderManager 实例
 */
struct TimerContext {
    ReminderManager* manager;       // ReminderManager 实例指针
    uint32_t reminder_id;           // 提醒ID
};

/**
 * 提醒数据结构
 */
struct Reminder {
    uint32_t id;                    // 提醒ID
    std::string message;            // 提醒内容
    uint32_t delay_seconds;         // 延迟秒数
    uint64_t create_time_ms;        // 创建时间（毫秒）
    uint64_t trigger_time_ms;       // 触发时间（毫秒）
    TimerHandle_t timer;            // FreeRTOS 定时器句柄
};

/**
 * 提醒管理器
 * 负责管理设备端的所有定时提醒
 */
class ReminderManager {
public:
    using ReminderCallback = std::function<void(uint32_t id, const std::string& message)>;

    ReminderManager();
    ~ReminderManager();

    /**
     * 添加提醒
     * @param delay_seconds 延迟秒数
     * @param message 提醒内容
     * @return 提醒ID，失败返回0
     */
    uint32_t AddReminder(uint32_t delay_seconds, const std::string& message);

    /**
     * 取消提醒
     * @param id 提醒ID
     * @return 成功返回true
     */
    bool CancelReminder(uint32_t id);

    /**
     * 获取所有提醒
     * @return 提醒列表
     */
    std::vector<Reminder> GetAllReminders() const;

    /**
     * 设置提醒触发回调
     * @param callback 回调函数
     */
    void SetCallback(ReminderCallback callback);

private:
    static void TimerCallback(TimerHandle_t timer);
    void OnTimerExpired(uint32_t id);

    std::vector<Reminder> reminders_;
    ReminderCallback callback_;
    uint32_t next_id_;
    mutable SemaphoreHandle_t mutex_;
};

#endif // REMINDER_MANAGER_H
