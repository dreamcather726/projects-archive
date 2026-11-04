/**
 * @file bsp_uart_tx.h
 * @brief BSP私有的发送环形缓冲算法；无寄存器依赖，供驱动与原生单元测试共用。
 * @note 所有调用方必须持有UART临界区；应用层应使用 BSP/Inc/bsp_uart.h。
 */
#ifndef BSP_UART_TX_H_
#define BSP_UART_TX_H_

#include <stdbool.h>
#include <stdint.h>

#define BSP_UART_TX_BUFFER_SIZE (256U)

/* 每次 BSP_UART_Write 视为完整的一帧。frame_end 不能由数据是否等于6B推导。 */
typedef struct
{
    uint8_t data[BSP_UART_TX_BUFFER_SIZE];
    bool frame_end[BSP_UART_TX_BUFFER_SIZE];
    volatile uint16_t write_index; /* 下一个写入位置；预留一个槽区分空/满。 */
    volatile uint16_t read_index;  /* 下一个交给USART数据寄存器的字节。 */
    bool frame_in_progress;       /* true表示ISR已取出当前帧的一部分。 */
} BSP_UartTxBuffer_t;

static inline void BSP_UART_TxDiscardQueuedFrames(BSP_UartTxBuffer_t *buffer)
{
    uint16_t index = buffer->read_index;
    if (buffer->frame_in_progress)
    {
        /* 仅保留已开始帧的尾部，丢弃未开始帧；否则急停可能拼接成非法电机命令。 */
        while (index != buffer->write_index)
        {
            bool end = buffer->frame_end[index];
            index = (uint16_t)((index + 1U) % BSP_UART_TX_BUFFER_SIZE);
            if (end) { break; }
        }
    }
    buffer->write_index = index;
}

static inline bool BSP_UART_TxPopByte(BSP_UartTxBuffer_t *buffer, uint8_t *byte)
{
    /* ISR或故障轮询一次取一个字节，同时更新当前帧边界状态。 */
    uint16_t index = buffer->read_index;
    if (index == buffer->write_index) { return false; }
    *byte = buffer->data[index];
    buffer->frame_in_progress = !buffer->frame_end[index];
    buffer->read_index = (uint16_t)((index + 1U) % BSP_UART_TX_BUFFER_SIZE);
    return true;
}

#endif /* BSP_UART_TX_H_ */
