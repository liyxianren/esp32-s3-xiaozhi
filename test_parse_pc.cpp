/**
 * PC端测试提醒参数解析功能
 * 使用 g++ 编译: g++ -o test_parse test_parse_pc.cpp -std=c++11
 */

#include <iostream>
#include <string>
#include <cstdio>
#include <cstring>

using namespace std;

// 模拟 ESP_LOGI
#define ESP_LOGI(tag, format, ...) printf("[%s] " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGW(tag, format, ...) printf("[%s] WARNING: " format "\n", tag, ##__VA_ARGS__)

#define TAG "TestParse"

// 模拟解析函数 (从 application.cc 复制)
void test_parse_reminder(const char* ai_response) {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Testing AI response:");
    ESP_LOGI(TAG, "%s", ai_response);
    ESP_LOGI(TAG, "========================================");

    string text_str = ai_response;

    if (text_str.find("% reminder_add") != string::npos) {
        ESP_LOGI(TAG, "Detected reminder_add in AI response");

        // 提取参数
        int delay_seconds = -1, hour = -1, minute = -1, repeat = 1, interval = 0;
        string reminder_message;

        // 解析 delay_seconds
        size_t delay_pos = text_str.find("delay_seconds=");
        if (delay_pos != string::npos) {
            sscanf(text_str.c_str() + delay_pos, "delay_seconds=%d", &delay_seconds);
            ESP_LOGI(TAG, "  Parsed delay_seconds=%d", delay_seconds);
        }

        // 解析 hour
        size_t hour_pos = text_str.find("hour=");
        if (hour_pos != string::npos) {
            sscanf(text_str.c_str() + hour_pos, "hour=%d", &hour);
            ESP_LOGI(TAG, "  Parsed hour=%d", hour);
        }

        // 解析 minute
        size_t minute_pos = text_str.find("minute=");
        if (minute_pos != string::npos) {
            sscanf(text_str.c_str() + minute_pos, "minute=%d", &minute);
            ESP_LOGI(TAG, "  Parsed minute=%d", minute);
        }

        // 解析 repeat
        size_t repeat_pos = text_str.find("repeat=");
        if (repeat_pos != string::npos) {
            sscanf(text_str.c_str() + repeat_pos, "repeat=%d", &repeat);
            ESP_LOGI(TAG, "  Parsed repeat=%d", repeat);
        }

        // 解析 interval
        size_t interval_pos = text_str.find("interval=");
        if (interval_pos != string::npos) {
            sscanf(text_str.c_str() + interval_pos, "interval=%d", &interval);
            ESP_LOGI(TAG, "  Parsed interval=%d", interval);
        }

        // 解析 message (格式: message='内容' 或 message="内容")
        size_t msg_pos = text_str.find("message=");
        if (msg_pos != string::npos) {
            size_t start = text_str.find_first_of("'\"", msg_pos);
            if (start != string::npos) {
                char quote = text_str[start];
                size_t end = text_str.find(quote, start + 1);
                if (end != string::npos) {
                    reminder_message = text_str.substr(start + 1, end - start - 1);
                    ESP_LOGI(TAG, "  Parsed message='%s'", reminder_message.c_str());
                }
            }
        }

        // 检查是否成功解析
        if (delay_seconds > 0 || (hour >= 0 && hour < 24) || (minute >= 0 && minute < 60)) {
            ESP_LOGI(TAG, "SUCCESS: Parsed reminder parameters:");
            ESP_LOGI(TAG, "   delay_seconds=%d, hour=%d, minute=%d", delay_seconds, hour, minute);
            ESP_LOGI(TAG, "   repeat=%d, interval=%d, message='%s'", repeat, interval, reminder_message.c_str());
        } else {
            ESP_LOGW(TAG, "No valid reminder parameters found");
        }
    } else {
        ESP_LOGI(TAG, "No reminder_add detected");
    }

    cout << endl;
}

int main() {
    ESP_LOGI(TAG, "Starting reminder parameter parsing tests...\n");

    // 测试用例 1: 倒计时提醒
    test_parse_reminder("% reminder_add(delay_seconds=10, message='eat medicine', hour=-1, minute=-1, repeat=1, interval=0)");

    // 测试用例 2: 定时提醒
    test_parse_reminder("% reminder_add(delay_seconds=29448, message='sleep', hour=20, minute=0, repeat=1, interval=0)");

    // 测试用例 3: 重复提醒
    test_parse_reminder("% reminder_add(delay_seconds=69048, message='wake up', hour=7, minute=0, repeat=10000, interval=86400)");

    // 测试用例 4: 包含前后文本
    test_parse_reminder("OK, I set a reminder for you! % reminder_add(delay_seconds=30, message='drink water', hour=-1, minute=-1, repeat=1, interval=0) Don't forget!");

    // 测试用例 5: 没有 reminder_add
    test_parse_reminder("OK, I understand!");

    // 测试用例 6: 使用双引号
    test_parse_reminder("% reminder_add(delay_seconds=60, message=\"test double quote\", hour=-1, minute=-1, repeat=1, interval=0)");

    ESP_LOGI(TAG, "All tests completed!");

    return 0;
}
