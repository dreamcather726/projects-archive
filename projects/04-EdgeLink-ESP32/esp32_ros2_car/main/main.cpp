#include <cstdint>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "uart.hpp"

namespace {

constexpr uint8_t TEST_FUNC = 0x01;
constexpr uint32_t SEND_INTERVAL_MS = 200;
constexpr uint32_t SEND_TIMEOUT_MS = 100;

void write_u32_le(uint8_t output[4], uint32_t value)
{
    output[0] = static_cast<uint8_t>(value & 0xFF);
    output[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    output[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
    output[3] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

}  // namespace

extern "C" void app_main(void)
{
    ESP_ERROR_CHECK(uart_protocol_init());

    uint32_t frame_counter = 0;
    TickType_t last_wake_time = xTaskGetTickCount();

    while (true) {
        uint8_t payload[4];
        write_u32_le(payload, frame_counter++);

        ESP_ERROR_CHECK(uart_protocol_send_frame(TEST_FUNC,
                                                 payload,
                                                 sizeof(payload),
                                                 SEND_TIMEOUT_MS));

        xTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(SEND_INTERVAL_MS));
    }
}
