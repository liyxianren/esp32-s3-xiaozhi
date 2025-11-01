#include "reminder_tools.h"

#include "application.h"
#include "mcp_server.h"
#include "reminder_manager.h"

#include <ctime>
#include <stdexcept>

#include <cJSON.h>
#include <esp_log.h>

namespace {

constexpr const char* kTag = "ReminderTools";

uint32_t NormalizeDelay(int delay_seconds, int hour, int minute) {
    if (delay_seconds < 0) {
        throw std::runtime_error("delay_seconds must be >= 0");
    }

    uint32_t normalized = static_cast<uint32_t>(delay_seconds);
    const bool has_time = (hour >= 0 && hour < 24) || (minute >= 0 && minute < 60);
    if (has_time) {
        time_t now = time(nullptr);
        struct tm timeinfo {};
        if (localtime_r(&now, &timeinfo) == nullptr) {
            throw std::runtime_error("Failed to read local time");
        }

        if (hour >= 0 && hour < 24) {
            timeinfo.tm_hour = hour;
        }
        if (minute >= 0 && minute < 60) {
            timeinfo.tm_min = minute;
        }
        timeinfo.tm_sec = 0;

        time_t target = mktime(&timeinfo);
        if (target <= now) {
            target += 24 * 3600;
        }
        normalized = static_cast<uint32_t>(difftime(target, now));
    }

    return normalized;
}

cJSON* BuildReminderResult(uint32_t id,
                           const std::string& message,
                           uint32_t delay_seconds,
                           int repeat,
                           int interval) {
    cJSON* result = cJSON_CreateObject();
    if (result == nullptr) {
        throw std::runtime_error("Failed to allocate result json");
    }

    cJSON_AddNumberToObject(result, "id", id);
    cJSON_AddStringToObject(result, "message", message.c_str());
    cJSON_AddNumberToObject(result, "delay_seconds", delay_seconds);
    cJSON_AddNumberToObject(result, "repeat", repeat);
    cJSON_AddNumberToObject(result, "interval", interval);
    return result;
}

}  // namespace

void InitializeReminderTools() {
    auto& mcp_server = McpServer::GetInstance();
    auto& reminder_manager = Application::GetInstance().GetReminderManager();

    mcp_server.AddTool(
        "reminder.add",
        "Add a reminder. Supports countdown (delay_seconds) and scheduled time (hour/minute). "
        "Parameters:\n"
        "  delay_seconds: Countdown in seconds (optional when hour/minute is provided)\n"
        "  message: Reminder text to speak when triggered\n"
        "  hour: Target hour for scheduled reminders (0-23, optional)\n"
        "  minute: Target minute for scheduled reminders (0-59, optional)\n"
        "  repeat: Number of repeats (reserved, defaults to 1)\n"
        "  interval: Repeat interval seconds (reserved, defaults to 0)",
        PropertyList({
            Property("delay_seconds", kPropertyTypeInteger, 0),
            Property("message", kPropertyTypeString),
            Property("hour", kPropertyTypeInteger, -1),
            Property("minute", kPropertyTypeInteger, -1),
            Property("repeat", kPropertyTypeInteger, 1),
            Property("interval", kPropertyTypeInteger, 0),
        }),
        [&reminder_manager](const PropertyList& props) -> ReturnValue {
            int delay = props["delay_seconds"].value<int>();
            int hour = props["hour"].value<int>();
            int minute = props["minute"].value<int>();
            int repeat = props["repeat"].value<int>();
            int interval = props["interval"].value<int>();
            std::string message = props["message"].value<std::string>();

            ESP_LOGI(kTag,
                     "Received reminder.add: delay=%d, hour=%d, minute=%d, repeat=%d, interval=%d, "
                     "message='%s'",
                     delay,
                     hour,
                     minute,
                     repeat,
                     interval,
                     message.c_str());

            uint32_t normalized_delay = NormalizeDelay(delay, hour, minute);
            if (normalized_delay == 0) {
                throw std::runtime_error("Reminder delay must be greater than 0");
            }

            if (message.empty()) {
                throw std::runtime_error("Reminder message cannot be empty");
            }

            uint32_t id = reminder_manager.AddReminder(normalized_delay, message);
            if (id == 0) {
                throw std::runtime_error("Failed to create reminder");
            }

            if (repeat > 1) {
                ESP_LOGW(
                    kTag,
                    "Repeat reminders not yet implemented (requested repeat=%d interval=%d). "
                    "Reminder will trigger once.",
                    repeat,
                    interval);
            }

            cJSON* result = BuildReminderResult(id, message, normalized_delay, repeat, interval);
            ESP_LOGI(kTag, "Reminder created: id=%u, delay=%u", id, normalized_delay);
            return result;
        });

    mcp_server.AddTool(
        "reminder.cancel",
        "Cancel a reminder by id.",
        PropertyList({
            Property("reminder_id", kPropertyTypeInteger),
        }),
        [&reminder_manager](const PropertyList& props) -> ReturnValue {
            uint32_t id = static_cast<uint32_t>(props["reminder_id"].value<int>());
            ESP_LOGI(kTag, "Received reminder.cancel: id=%u", id);

            if (id == 0) {
                throw std::runtime_error("reminder_id must be greater than 0");
            }

            if (!reminder_manager.CancelReminder(id)) {
                throw std::runtime_error("Reminder not found");
            }

            ESP_LOGI(kTag, "Reminder %u cancelled", id);
            return std::string("Reminder cancelled successfully");
        });

    ESP_LOGI(kTag, "Reminder tools initialized");
}

