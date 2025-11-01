#include "reminder_manager.h"
#include <esp_timer.h>
#include <algorithm>

static const char* TAG = "ReminderManager";

ReminderManager::ReminderManager()
    : next_id_(1) {
    mutex_ = xSemaphoreCreateMutex();
    ESP_LOGI(TAG, "✅ ReminderManager initialized");
}

ReminderManager::~ReminderManager() {
    // 清理所有定时器
    if (xSemaphoreTake(mutex_, portMAX_DELAY) == pdTRUE) {
        for (auto& reminder : reminders_) {
            if (reminder.timer != nullptr) {
                // 清理定时器上下文
                TimerContext* ctx = (TimerContext*)pvTimerGetTimerID(reminder.timer);
                if (ctx != nullptr) {
                    delete ctx;
                }
                xTimerStop(reminder.timer, 0);
                xTimerDelete(reminder.timer, 0);
            }
        }
        reminders_.clear();
        xSemaphoreGive(mutex_);
    }
    vSemaphoreDelete(mutex_);
    ESP_LOGI(TAG, "ReminderManager destroyed");
}

uint32_t ReminderManager::AddReminder(uint32_t delay_seconds, const std::string& message) {
    ESP_LOGI(TAG, "📝 AddReminder called: delay=%us, message='%s'", delay_seconds, message.c_str());

    if (delay_seconds == 0 || message.empty()) {
        ESP_LOGE(TAG, "❌ Invalid parameters: delay=%u, message='%s'", delay_seconds, message.c_str());
        return 0;
    }

    if (xSemaphoreTake(mutex_, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "❌ Failed to take mutex");
        return 0;
    }

    // 创建 FreeRTOS 软件定时器
    uint32_t id = next_id_++;
    TickType_t delay_ticks = pdMS_TO_TICKS(delay_seconds * 1000);

    ESP_LOGI(TAG, "🔧 Creating timer: id=%u, delay_ticks=%u", id, (unsigned)delay_ticks);

    // 创建定时器上下文，存储 ReminderManager 实例指针和提醒ID
    TimerContext* ctx = new TimerContext{this, id};
    ESP_LOGI(TAG, "🔧 TimerContext created: manager=%p, reminder_id=%u", this, id);

    TimerHandle_t timer = xTimerCreate(
        "ReminderTimer",           // 定时器名称
        delay_ticks,               // 定时器周期（ticks）
        pdFALSE,                   // 不自动重载（单次触发）
        (void*)ctx,                // 定时器上下文（包含管理器指针和提醒ID）
        TimerCallback              // 回调函数
    );

    if (timer == nullptr) {
        ESP_LOGE(TAG, "❌ Failed to create timer for reminder %u", id);
        delete ctx;  // 清理上下文内存
        xSemaphoreGive(mutex_);
        return 0;
    }

    // 启动定时器
    if (xTimerStart(timer, 0) != pdPASS) {
        ESP_LOGE(TAG, "❌ Failed to start timer for reminder %u", id);
        xTimerDelete(timer, 0);
        delete ctx;  // 清理上下文内存
        xSemaphoreGive(mutex_);
        return 0;
    }

    // 保存提醒信息
    uint64_t now_ms = esp_timer_get_time() / 1000;
    Reminder reminder;
    reminder.id = id;
    reminder.message = message;
    reminder.delay_seconds = delay_seconds;
    reminder.create_time_ms = now_ms;
    reminder.trigger_time_ms = now_ms + delay_seconds * 1000;
    reminder.timer = timer;

    reminders_.push_back(reminder);

    xSemaphoreGive(mutex_);

    ESP_LOGI(TAG, "✅ Reminder added successfully: id=%u, will trigger in %u seconds", id, delay_seconds);
    ESP_LOGI(TAG, "📊 Total reminders: %d", reminders_.size());

    return id;
}

bool ReminderManager::CancelReminder(uint32_t id) {
    ESP_LOGI(TAG, "🗑️ CancelReminder called: id=%u", id);

    if (xSemaphoreTake(mutex_, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "❌ Failed to take mutex");
        return false;
    }

    auto it = std::find_if(reminders_.begin(), reminders_.end(),
        [id](const Reminder& r) { return r.id == id; });

    if (it == reminders_.end()) {
        ESP_LOGW(TAG, "⚠️ Reminder %u not found", id);
        xSemaphoreGive(mutex_);
        return false;
    }

    // 停止并删除定时器
    if (it->timer != nullptr) {
        // 获取并清理定时器上下文
        TimerContext* ctx = (TimerContext*)pvTimerGetTimerID(it->timer);
        if (ctx != nullptr) {
            ESP_LOGI(TAG, "🗑️ Cleaning up timer context for reminder %u", id);
            delete ctx;
        }
        xTimerStop(it->timer, 0);
        xTimerDelete(it->timer, 0);
    }

    reminders_.erase(it);
    xSemaphoreGive(mutex_);

    ESP_LOGI(TAG, "✅ Reminder %u cancelled successfully", id);
    ESP_LOGI(TAG, "📊 Remaining reminders: %d", reminders_.size());

    return true;
}

std::vector<Reminder> ReminderManager::GetAllReminders() const {
    std::vector<Reminder> result;

    if (xSemaphoreTake(mutex_, portMAX_DELAY) == pdTRUE) {
        result = reminders_;
        xSemaphoreGive(mutex_);
    }

    ESP_LOGI(TAG, "📋 GetAllReminders: returning %d reminders", result.size());
    return result;
}

void ReminderManager::SetCallback(ReminderCallback callback) {
    callback_ = callback;
    ESP_LOGI(TAG, "✅ Callback registered");
}

void ReminderManager::TimerCallback(TimerHandle_t timer) {
    // 从定时器获取上下文指针
    TimerContext* ctx = (TimerContext*)pvTimerGetTimerID(timer);

    if (ctx == nullptr) {
        ESP_LOGE(TAG, "❌ Timer context is NULL!");
        return;
    }

    ESP_LOGI(TAG, "⏰⏰⏰ Timer callback fired!");
    ESP_LOGI(TAG, "⏰ Reminder ID: %u", ctx->reminder_id);
    ESP_LOGI(TAG, "⏰ Manager pointer: %p", ctx->manager);

    // 调用实例方法处理提醒触发
    if (ctx->manager != nullptr) {
        ctx->manager->OnTimerExpired(ctx->reminder_id);
    } else {
        ESP_LOGE(TAG, "❌ Manager pointer is NULL!");
    }

    // 清理上下文内存
    ESP_LOGI(TAG, "🗑️ Deleting timer context");
    delete ctx;
}

void ReminderManager::OnTimerExpired(uint32_t id) {
    ESP_LOGI(TAG, "🔔 OnTimerExpired called for reminder %u", id);

    if (xSemaphoreTake(mutex_, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "❌ Failed to take mutex in OnTimerExpired");
        return;
    }

    auto it = std::find_if(reminders_.begin(), reminders_.end(),
        [id](const Reminder& r) { return r.id == id; });

    if (it == reminders_.end()) {
        ESP_LOGW(TAG, "⚠️ Reminder %u already removed", id);
        xSemaphoreGive(mutex_);
        return;
    }

    std::string message = it->message;

    // 删除定时器（已经触发，不需要再停止）
    if (it->timer != nullptr) {
        xTimerDelete(it->timer, 0);
    }

    reminders_.erase(it);
    xSemaphoreGive(mutex_);

    ESP_LOGI(TAG, "✅ Reminder %u removed from list", id);
    ESP_LOGI(TAG, "📊 Remaining reminders: %d", reminders_.size());

    // 触发回调
    if (callback_) {
        ESP_LOGI(TAG, "🚀 Triggering callback for reminder %u: '%s'", id, message.c_str());
        callback_(id, message);
    } else {
        ESP_LOGW(TAG, "⚠️ No callback registered for reminder %u", id);
    }
}
