#include <stdio.h>
#include <string.h>
#include "bsp_uart_tx.h"

#define CHECK(x) do { if (!(x)) { printf("FAIL line %u: %s\n", (unsigned)__LINE__, #x); return 1; } } while (0)

static void Append(BSP_UartTxBuffer_t *buffer, const uint8_t *data, uint16_t length)
{
    uint16_t index;
    for (index = 0; index < length; ++index)
    {
        buffer->data[buffer->write_index] = data[index];
        buffer->frame_end[buffer->write_index] = (index == length - 1U);
        buffer->write_index = (uint16_t)((buffer->write_index + 1U) % BSP_UART_TX_BUFFER_SIZE);
    }
}

int main(void)
{
    const uint8_t motion[] = {1U, 0xF6U, 0x6BU, 0x02U, 0x6BU};
    const uint8_t stop[] = {0U, 0xFEU, 0x98U, 0U, 0x6BU};
    BSP_UartTxBuffer_t buffer;
    uint8_t byte;
    uint16_t start, index;
    for (start = 0U; start < BSP_UART_TX_BUFFER_SIZE; ++start)
    {
        memset(&buffer, 0, sizeof(buffer));
        buffer.read_index = start;
        buffer.write_index = start;
        Append(&buffer, motion, sizeof(motion));
        Append(&buffer, motion, sizeof(motion));
        CHECK(BSP_UART_TxPopByte(&buffer, &byte) && byte == motion[0]);
        BSP_UART_TxDiscardQueuedFrames(&buffer);
        Append(&buffer, stop, sizeof(stop));
        for (index = 1U; index < sizeof(motion); ++index)
        {
            CHECK(BSP_UART_TxPopByte(&buffer, &byte) && byte == motion[index]);
        }
        for (index = 0U; index < sizeof(stop); ++index)
        {
            CHECK(BSP_UART_TxPopByte(&buffer, &byte) && byte == stop[index]);
        }
        CHECK(!BSP_UART_TxPopByte(&buffer, &byte) && !buffer.frame_in_progress);
        Append(&buffer, motion, sizeof(motion));
        BSP_UART_TxDiscardQueuedFrames(&buffer);
        CHECK(!BSP_UART_TxPopByte(&buffer, &byte));
    }
    puts("UART TX ring priority/partial-frame/wrap tests passed (256 offsets)");
    return 0;
}
