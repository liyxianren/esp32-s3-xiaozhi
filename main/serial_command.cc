#include "serial_command.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/uart.h>
#include <string.h>

#define TAG "SerialCommand"
#define UART_NUM UART_NUM_0
#define BUF_SIZE 256

extern const char* FIRMWARE_VERSION;

static void serial_command_task(void* arg) {
    uint8_t data[BUF_SIZE];
    char cmd_buffer[BUF_SIZE];
    int cmd_len = 0;

    ESP_LOGI(TAG, "Serial command task started");
    ESP_LOGI(TAG, "Available commands: version, help");

    while (1) {
        // 增加超时时间，避免频繁轮询占用 CPU
        int len = uart_read_bytes(UART_NUM, data, BUF_SIZE - 1, pdMS_TO_TICKS(1000));
        if (len > 0) {
            for (int i = 0; i < len; i++) {
                char c = (char)data[i];

                // 回显字符
                uart_write_bytes(UART_NUM, &c, 1);

                if (c == '\r' || c == '\n') {
                    if (cmd_len > 0) {
                        cmd_buffer[cmd_len] = '\0';
                        uart_write_bytes(UART_NUM, "\r\n", 2);

                        // 处理命令
                        if (strcmp(cmd_buffer, "version") == 0 || strcmp(cmd_buffer, "v") == 0) {
                            char version_str[256];
                            snprintf(version_str, sizeof(version_str),
                                    "\r\n"
                                    "========================================\r\n"
                                    "Firmware Version: %s\r\n"
                                    "Build Date: %s %s\r\n"
                                    "Chip: ESP32-S3\r\n"
                                    "Features: ReminderManager, CustomMessage\r\n"
                                    "========================================\r\n",
                                    FIRMWARE_VERSION, __DATE__, __TIME__);
                            uart_write_bytes(UART_NUM, version_str, strlen(version_str));

                        } else if (strcmp(cmd_buffer, "help") == 0 || strcmp(cmd_buffer, "h") == 0 || strcmp(cmd_buffer, "?") == 0) {
                            const char* help_str =
                                "\r\n"
                                "Available Commands:\r\n"
                                "  version, v  - Show firmware version\r\n"
                                "  help, h, ?  - Show this help\r\n"
                                "\r\n";
                            uart_write_bytes(UART_NUM, help_str, strlen(help_str));

                        } else {
                            char error_str[320];  // 增加缓冲区大小
                            snprintf(error_str, sizeof(error_str),
                                    "\r\nUnknown command: %s\r\nType 'help' for available commands\r\n",
                                    cmd_buffer);
                            uart_write_bytes(UART_NUM, error_str, strlen(error_str));
                        }

                        uart_write_bytes(UART_NUM, "> ", 2);
                        cmd_len = 0;
                    }
                } else if (c == '\b' || c == 127) {  // 退格
                    if (cmd_len > 0) {
                        cmd_len--;
                        uart_write_bytes(UART_NUM, " \b", 2);  // 擦除字符
                    }
                } else if (c >= 32 && c < 127) {  // 可打印字符
                    if (cmd_len < BUF_SIZE - 1) {
                        cmd_buffer[cmd_len++] = c;
                    }
                }
            }
        }
    }
}

void serial_command_init(void) {
    // 降低任务优先级，避免阻塞 IDLE 任务
    xTaskCreate(serial_command_task, "serial_cmd", 4096, NULL, 1, NULL);
    ESP_LOGI(TAG, "Serial command initialized");
}
