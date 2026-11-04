#pragma once

#include <cstddef>
#include <cstdint>

#include "driver/uart.h"
#include "esp_err.h"

// Temporary test configuration: reuse UART0 and keep its current console pins.
inline constexpr uart_port_t UART_PROTOCOL_PORT = UART_NUM_0;
inline constexpr int UART_PROTOCOL_TX_PIN = UART_PIN_NO_CHANGE;
inline constexpr int UART_PROTOCOL_RX_PIN = UART_PIN_NO_CHANGE;
inline constexpr int UART_PROTOCOL_BAUD_RATE = 115200;

// AA | LEN | FUNC | PAYLOAD | CRC_L | CRC_H | BB
inline constexpr uint8_t UART_PROTOCOL_HEADER = 0xAA;
inline constexpr uint8_t UART_PROTOCOL_TAIL = 0xBB;
inline constexpr size_t UART_PROTOCOL_MAX_PAYLOAD_SIZE = 32;
inline constexpr size_t UART_PROTOCOL_MAX_FRAME_SIZE =
    1 + 1 + 1 + UART_PROTOCOL_MAX_PAYLOAD_SIZE + 2 + 1;

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize UART0 as 115200 8N1 for the temporary transmit test. */
esp_err_t uart_protocol_init(void);

/** CRC16-Modbus: initial value 0xFFFF, polynomial 0xA001. */
uint16_t uart_protocol_crc16(const uint8_t *data, size_t size);

/**
 * Build AA | LEN | FUNC | PAYLOAD | CRC_L | CRC_H | BB.
 * CRC covers LEN + FUNC + PAYLOAD. Returns 0 when an argument is invalid.
 */
size_t uart_protocol_build_frame(uint8_t func,
                                 const uint8_t *payload,
                                 size_t payload_len,
                                 uint8_t *output,
                                 size_t output_capacity);

/** Build and synchronously transmit one frame. */
esp_err_t uart_protocol_send_frame(uint8_t func,
                                   const uint8_t *payload,
                                   size_t payload_len,
                                   uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif
