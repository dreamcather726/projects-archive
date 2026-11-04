#include "uart.hpp"

#include <algorithm>
#include <cstring>
#include <limits>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

namespace {

constexpr char TAG[] = "UART_PROTOCOL";
constexpr int RX_DRIVER_BUFFER_SIZE = 256;
constexpr int TX_DRIVER_BUFFER_SIZE = 512;

SemaphoreHandle_t g_tx_mutex = nullptr;
bool g_initialized = false;
bool g_driver_owned = false;

TickType_t milliseconds_to_ticks(uint32_t timeout_ms)
{
    if (timeout_ms == std::numeric_limits<uint32_t>::max()) {
        return portMAX_DELAY;
    }
    if (timeout_ms == 0) {
        return 0;
    }
    return std::max<TickType_t>(1, pdMS_TO_TICKS(timeout_ms));
}

void release_resources_after_init_failure()
{
    if (g_driver_owned && uart_is_driver_installed(UART_PROTOCOL_PORT)) {
        uart_driver_delete(UART_PROTOCOL_PORT);
        g_driver_owned = false;
    }
    if (g_tx_mutex != nullptr) {
        vSemaphoreDelete(g_tx_mutex);
        g_tx_mutex = nullptr;
    }
}

}  // namespace

esp_err_t uart_protocol_init(void)
{
    if (g_initialized) {
        return ESP_OK;
    }

    g_tx_mutex = xSemaphoreCreateMutex();
    if (g_tx_mutex == nullptr) {
        return ESP_ERR_NO_MEM;
    }

    uart_config_t config = {};
    config.baud_rate = UART_PROTOCOL_BAUD_RATE;
    config.data_bits = UART_DATA_8_BITS;
    config.parity = UART_PARITY_DISABLE;
    config.stop_bits = UART_STOP_BITS_1;
    config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    config.rx_flow_ctrl_thresh = 0;
    config.source_clk = UART_SCLK_DEFAULT;

    esp_err_t error = ESP_OK;
    if (!uart_is_driver_installed(UART_PROTOCOL_PORT)) {
        error = uart_driver_install(UART_PROTOCOL_PORT,
                                    RX_DRIVER_BUFFER_SIZE,
                                    TX_DRIVER_BUFFER_SIZE,
                                    0,
                                    nullptr,
                                    0);
        if (error != ESP_OK) {
            release_resources_after_init_failure();
            return error;
        }
        g_driver_owned = true;
    }

    error = uart_param_config(UART_PROTOCOL_PORT, &config);
    if (error == ESP_OK) {
        error = uart_set_pin(UART_PROTOCOL_PORT,
                             UART_PROTOCOL_TX_PIN,
                             UART_PROTOCOL_RX_PIN,
                             UART_PIN_NO_CHANGE,
                             UART_PIN_NO_CHANGE);
    }
    if (error != ESP_OK) {
        release_resources_after_init_failure();
        return error;
    }

    g_initialized = true;
    ESP_LOGI(TAG, "UART0 transmit test ready, 115200 8N1");
    return ESP_OK;
}

uint16_t uart_protocol_crc16(const uint8_t *data, size_t size)
{
    if (data == nullptr && size != 0) {
        return 0;
    }

    uint16_t crc = 0xFFFF;
    for (size_t index = 0; index < size; ++index) {
        crc ^= data[index];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            if ((crc & 0x0001U) != 0) {
                crc = static_cast<uint16_t>((crc >> 1) ^ 0xA001U);
            } else {
                crc = static_cast<uint16_t>(crc >> 1);
            }
        }
    }
    return crc;
}

size_t uart_protocol_build_frame(uint8_t func,
                                 const uint8_t *payload,
                                 size_t payload_len,
                                 uint8_t *output,
                                 size_t output_capacity)
{
    if (output == nullptr || payload_len > UART_PROTOCOL_MAX_PAYLOAD_SIZE ||
        (payload == nullptr && payload_len != 0)) {
        return 0;
    }

    const size_t data_len = 1 + payload_len;
    const size_t frame_size = data_len + 5;
    if (output_capacity < frame_size) {
        return 0;
    }

    output[0] = UART_PROTOCOL_HEADER;
    output[1] = static_cast<uint8_t>(data_len);
    output[2] = func;
    if (payload_len > 0) {
        std::memcpy(&output[3], payload, payload_len);
    }

    const uint16_t crc = uart_protocol_crc16(&output[1], data_len + 1);
    const size_t crc_position = 3 + payload_len;
    output[crc_position] = static_cast<uint8_t>(crc & 0xFF);
    output[crc_position + 1] = static_cast<uint8_t>((crc >> 8) & 0xFF);
    output[crc_position + 2] = UART_PROTOCOL_TAIL;
    return frame_size;
}

esp_err_t uart_protocol_send_frame(uint8_t func,
                                   const uint8_t *payload,
                                   size_t payload_len,
                                   uint32_t timeout_ms)
{
    if (!g_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t frame[UART_PROTOCOL_MAX_FRAME_SIZE];
    const size_t frame_size = uart_protocol_build_frame(func,
                                                        payload,
                                                        payload_len,
                                                        frame,
                                                        sizeof(frame));
    if (frame_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    const TickType_t wait_ticks = milliseconds_to_ticks(timeout_ms);
    if (xSemaphoreTake(g_tx_mutex, wait_ticks) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t result = ESP_OK;
    const int written = uart_write_bytes(UART_PROTOCOL_PORT, frame, frame_size);
    if (written != static_cast<int>(frame_size)) {
        result = ESP_FAIL;
    } else {
        result = uart_wait_tx_done(UART_PROTOCOL_PORT, wait_ticks);
    }

    xSemaphoreGive(g_tx_mutex);
    return result;
}
