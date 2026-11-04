/**
 * @file bsp_uart.c
 * @brief 基于 STM32F4 标准外设库的四路非阻塞串口驱动。
 *
 * 发送：软件环形缓冲区 + TXE 中断，调用发送函数不会等待硬件。
 * 接收：DMA 循环模式，应用层仍使用 BSP_UART_TryReadByte() 读取。
 */

#include "bsp_uart.h"
#include "bsp_uart_tx.h"

#include <stddef.h>

#include "app_config.h"
#include "stm32f4xx_dma.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_usart.h"

/* 各外设的波特率集中放在这里，确定设备协议后可分别修改。 */
#define BSP_UART_HOST_BAUD_RATE (115200UL)
#define BSP_UART_VISION_BAUD_RATE (115200UL)
/* JY61P 波特率由应用配置固定指定，不再执行运行期扫描。 */
#define BSP_UART_IMU_BAUD_RATE APP_CONFIG_IMU_BAUD_RATE
#define BSP_UART_MOTOR_BAUD_RATE (115200UL)

/* 大小使用 256，便于观察和计算；TX 环形队列实际最多保存 255 字节。 */
#define BSP_UART_RX_DMA_BUFFER_SIZE (256U)

/* 等待 DMA Stream 停止时的最大轮询次数，防止硬件异常造成永久死循环。 */
#define BSP_UART_DMA_DISABLE_TIMEOUT (1000000UL)

/* 与 main.c 的 NVIC_SetPriorityGrouping(0) 一致：全部优先级位用于抢占。 */
#define BSP_UART_NVIC_PRIORITY_GROUPING (0U)
#define BSP_UART_USART_INTERRUPT_PRIORITY (8U)
#define BSP_UART_DMA_INTERRUPT_PRIORITY (9U)

/** 描述一路串口所使用的 USART、GPIO 和 RX DMA 资源。 */
typedef struct
{
    USART_TypeDef *instance;          /**< USART 外设地址，例如 USART1。 */
    GPIO_TypeDef *tx_gpio;            /**< TX 引脚所在的 GPIO 端口，例如 GPIOA。 */
    GPIO_TypeDef *rx_gpio;            /**< RX 引脚所在的 GPIO 端口，例如 GPIOA。 */
    uint16_t tx_pin;                  /**< TX 引脚位掩码，例如 GPIO_Pin_9。 */
    uint16_t rx_pin;                  /**< RX 引脚位掩码，例如 GPIO_Pin_10。 */
    uint8_t tx_pin_source;            /**< TX 引脚编号，供 GPIO_PinAFConfig() 使用。 */
    uint8_t rx_pin_source;            /**< RX 引脚编号，供 GPIO_PinAFConfig() 使用。 */
    uint8_t alternate_function;       /**< 引脚复用功能编号，例如 GPIO_AF_USART1。 */
    uint32_t baud_rate;               /**< 串口波特率，例如 115200。 */
    IRQn_Type usart_interrupt_number; /**< USART 在 NVIC 中的中断编号。 */

    DMA_Stream_TypeDef *rx_dma_stream; /**< 负责 USART 接收的 DMA 数据流。 */
    uint32_t rx_dma_channel;           /**< USART RX 请求对应的 DMA 通道。 */
    uint32_t rx_dma_tc_flag;           /**< DMA 接收一圈后的传输完成状态标志。 */
    uint32_t rx_dma_tc_interrupt;      /**< DMA 传输完成中断的待处理标志。 */
    uint32_t rx_dma_te_interrupt;      /**< DMA 传输错误中断的待处理标志。 */
    IRQn_Type rx_dma_interrupt_number; /**< DMA Stream 在 NVIC 中的中断编号。 */
} BSP_UartConfig_t;

/** DMA 写入 data，主循环使用绝对读取计数取走数据。 */
typedef struct
{
    volatile uint8_t data[BSP_UART_RX_DMA_BUFFER_SIZE]; /**< DMA 自动写入的接收区。 */
    volatile uint32_t dma_wrap_count;  /**< DMA 写满缓冲区并绕回起点的累计次数。 */
    uint32_t read_count;               /**< 应用层已经取走的累计字节数。 */
    uint32_t dropped_byte_count;       /**< 因 DMA 覆盖旧数据而丢失的累计字节数。 */
    volatile uint32_t dma_error_count; /**< DMA TE 传输错误累计次数。 */
} BSP_UartRxBuffer_t;


/*
 * 表中顺序必须与 BSP_UartPort_t 一致。
 * DMA 映射来自 STM32F407 参考手册 RM0090 的 DMA request mapping 表。
 * 修改端口或引脚后，同步更新 Docs/HOST_PROTOCOL.md 和 Docs/UART_DEBUG_GUIDE.md。
 */
static const BSP_UartConfig_t s_uart_configs[BSP_UART_PORT_COUNT] = {
    /* 上位机串口：可连接板载 CH340 或外部 Linux 主控。 */
    {USART1, GPIOA, GPIOA, GPIO_Pin_9, GPIO_Pin_10, GPIO_PinSource9, GPIO_PinSource10,
     GPIO_AF_USART1, BSP_UART_HOST_BAUD_RATE, USART1_IRQn, DMA2_Stream2, DMA_Channel_4,
     DMA_FLAG_TCIF2, DMA_IT_TCIF2, DMA_IT_TEIF2, DMA2_Stream2_IRQn},

    /* 视觉模块串口。 */
    {USART2, GPIOA, GPIOA, GPIO_Pin_2, GPIO_Pin_3, GPIO_PinSource2, GPIO_PinSource3, GPIO_AF_USART2,
     BSP_UART_VISION_BAUD_RATE, USART2_IRQn, DMA1_Stream5, DMA_Channel_4, DMA_FLAG_TCIF5,
     DMA_IT_TCIF5, DMA_IT_TEIF5, DMA1_Stream5_IRQn},

    /* 陀螺仪/IMU 串口。 */
    {USART3, GPIOB, GPIOB, GPIO_Pin_10, GPIO_Pin_11, GPIO_PinSource10, GPIO_PinSource11,
     GPIO_AF_USART3, BSP_UART_IMU_BAUD_RATE, USART3_IRQn, DMA1_Stream1, DMA_Channel_4,
     DMA_FLAG_TCIF1, DMA_IT_TCIF1, DMA_IT_TEIF1, DMA1_Stream1_IRQn},

    /* Emm_V5 电机驱动器串口。 */
    {USART6, GPIOG, GPIOG, GPIO_Pin_14, GPIO_Pin_9, GPIO_PinSource14, GPIO_PinSource9,
     GPIO_AF_USART6, BSP_UART_MOTOR_BAUD_RATE, USART6_IRQn, DMA2_Stream1, DMA_Channel_5,
     DMA_FLAG_TCIF1, DMA_IT_TCIF1, DMA_IT_TEIF1, DMA2_Stream1_IRQn},
};

/** 四路串口各自独立的 DMA 接收状态和缓冲区。 */
static BSP_UartRxBuffer_t s_rx_buffers[BSP_UART_PORT_COUNT];

/** 四路串口各自独立的中断发送状态和缓冲区。 */
static BSP_UartTxBuffer_t s_tx_buffers[BSP_UART_PORT_COUNT];

/** 标记每一路串口是否已经完成初始化。 */
static bool s_uart_initialized[BSP_UART_PORT_COUNT];

/** 进入临界区，并返回进入前的全局中断状态。 */
static uint32_t BSP_UART_EnterCritical(void);

/** 恢复进入临界区以前的全局中断状态。 */
static void BSP_UART_ExitCritical(uint32_t previous_primask);

/** 将一个 GPIO 引脚配置为指定的 USART 复用功能。 */
static void BSP_UART_ConfigurePin(GPIO_TypeDef *gpio, uint16_t pin, uint8_t pin_source,
                                  uint8_t alternate_function, GPIOPuPd_TypeDef pull_mode);

/** 配置一路 USART 的循环 DMA 接收，失败时返回超时状态。 */
static Project_Status_t BSP_UART_ConfigureRxDma(BSP_UartPort_t port);

/** 初始化一路 USART、GPIO、NVIC 和接收 DMA。 */
static Project_Status_t BSP_UART_InitPort(BSP_UartPort_t port);

/** 根据 DMA 圈数和 NDTR 计算累计接收字节数。 */
static uint32_t BSP_UART_GetProducedRxCount(BSP_UartPort_t port);

/** 处理一路 USART 的 TXE 发送中断。 */
static void BSP_UART_HandleUsartInterrupt(BSP_UartPort_t port);

/** 处理一路接收 DMA 的完成和传输错误中断。 */
static void BSP_UART_HandleRxDmaInterrupt(BSP_UartPort_t port);

Project_Status_t BSP_UART_Init(void)
{
    BSP_UartPort_t port;
    Project_Status_t status;

    /* GPIOA/B/G 分别承载四路串口引脚，DMA1/2 分别服务不同 USART。 */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOG |
                               RCC_AHB1Periph_DMA1 | RCC_AHB1Periph_DMA2,
                           ENABLE);

    /* USART2、USART3 位于 APB1；USART1、USART6 位于 APB2。 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2 | RCC_APB1Periph_USART3, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_USART6, ENABLE);

    for (port = BSP_UART_PORT_HOST; port < BSP_UART_PORT_COUNT; ++port)
    {
        status = BSP_UART_InitPort(port);
        if (status != PROJECT_STATUS_OK)
        {
            return status;
        }
    }

    return PROJECT_STATUS_OK;
}

Project_Status_t BSP_UART_Write(BSP_UartPort_t port, const uint8_t *data, uint32_t length)
{
    BSP_UartTxBuffer_t *buffer;
    const BSP_UartConfig_t *config;
    uint32_t previous_primask;
    uint32_t used_count;
    uint32_t free_count;
    uint32_t index;
    uint16_t write_index;

    if ((port >= BSP_UART_PORT_COUNT) || ((data == NULL) && (length != 0U)))
    {
        return PROJECT_STATUS_INVALID_ARGUMENT;
    }

    if (!s_uart_initialized[port])
    {
        return PROJECT_STATUS_NOT_INITIALIZED;
    }

    if (length == 0U)
    {
        return PROJECT_STATUS_OK;
    }

    buffer = &s_tx_buffers[port];
    config = &s_uart_configs[port];

    /*
     * 主循环和 TXE 中断会并发访问读写指针，所以这里必须短暂关中断。
     * 保存 PRIMASK 后再恢复，可避免破坏调用者原先的中断状态。
     */
    previous_primask = BSP_UART_EnterCritical();

    if (buffer->write_index >= buffer->read_index)
    {
        used_count = (uint32_t)(buffer->write_index - buffer->read_index);
    }
    else
    {
        used_count = BSP_UART_TX_BUFFER_SIZE - (uint32_t)(buffer->read_index - buffer->write_index);
    }
    free_count = (BSP_UART_TX_BUFFER_SIZE - 1U) - used_count;

    /* 空间不足时整帧都不入队，上层稍后重试，不会发送半帧。 */
    if (length > free_count)
    {
        BSP_UART_ExitCritical(previous_primask);
        return PROJECT_STATUS_ERROR;
    }

    write_index = buffer->write_index;
    for (index = 0U; index < length; ++index)
    {
        buffer->data[write_index] = data[index];
        buffer->frame_end[write_index] = index == (length - 1U);
        write_index = (uint16_t)((write_index + 1U) % BSP_UART_TX_BUFFER_SIZE);
    }
    buffer->write_index = write_index;

    /* TXE 当前若已置位，退出临界区后会立即进入 USART 中断开始发送。 */
    USART_ITConfig(config->instance, USART_IT_TXE, ENABLE);
    BSP_UART_ExitCritical(previous_primask);

    return PROJECT_STATUS_OK;
}

Project_Status_t BSP_UART_MotorStop(void)
{
    /* 清队列与加入STOP须在同一临界区，不能让ISR在两步之间取出另一个旧动作。 */
    static const uint8_t stop_frame[] = {0U, 0xFEU, 0x98U, 0U, 0x6BU};
    uint32_t previous_primask;
    Project_Status_t status;
    if (!s_uart_initialized[BSP_UART_PORT_MOTOR]) { return PROJECT_STATUS_NOT_INITIALIZED; }
    previous_primask = BSP_UART_EnterCritical();
    BSP_UART_TxDiscardQueuedFrames(&s_tx_buffers[BSP_UART_PORT_MOTOR]);
    status = BSP_UART_Write(BSP_UART_PORT_MOTOR, stop_frame, sizeof(stop_frame));
    BSP_UART_ExitCritical(previous_primask);
    return status;
}

Project_Status_t BSP_UART_EmergencyMotorStop(void)
{
    static const uint8_t stop_frame[] = {0U, 0xFEU, 0x98U, 0U, 0x6BU};
    const BSP_UartConfig_t *config = &s_uart_configs[BSP_UART_PORT_MOTOR];
    BSP_UartTxBuffer_t *buffer = &s_tx_buffers[BSP_UART_PORT_MOTOR];
    uint32_t previous_primask;
    uint32_t repeat;
    uint32_t index;
    uint32_t remaining;
    Project_Status_t status = PROJECT_STATUS_OK;

    if (!s_uart_initialized[BSP_UART_PORT_MOTOR])
    {
        return PROJECT_STATUS_NOT_INITIALIZED;
    }
    previous_primask = BSP_UART_EnterCritical();
    USART_ITConfig(config->instance, USART_IT_TXE, DISABLE);
    BSP_UART_TxDiscardQueuedFrames(buffer);
    /* Complete the current frame before polling STOP, even if an IRQ fault cut it short. */
    while (buffer->read_index != buffer->write_index)
    {
        uint8_t byte;
        remaining = 1000000UL;
        while ((USART_GetFlagStatus(config->instance, USART_FLAG_TXE) == RESET) && remaining)
        {
            --remaining;
        }
        if (!remaining) { status = PROJECT_STATUS_ERROR; goto exit_stop; }
        (void)BSP_UART_TxPopByte(buffer, &byte);
        USART_SendData(config->instance, byte);
    }
    /* 硬件异常不能无限等待；重复广播只提高送达概率，不代表驱动器已停车。 */
    for (repeat = 0U; repeat < 3U; ++repeat)
    {
        for (index = 0U; index < sizeof(stop_frame); ++index)
        {
            remaining = 1000000UL;
            while ((USART_GetFlagStatus(config->instance, USART_FLAG_TXE) == RESET) &&
                   (remaining != 0U))
            {
                --remaining;
            }
            if (remaining == 0U)
            {
                status = PROJECT_STATUS_ERROR;
                goto exit_stop;
            }
            USART_SendData(config->instance, stop_frame[index]);
        }
    }
    remaining = 1000000UL;
    while ((USART_GetFlagStatus(config->instance, USART_FLAG_TC) == RESET) && (remaining != 0U))
    {
        --remaining;
    }
    if (remaining == 0U)
    {
        status = PROJECT_STATUS_ERROR;
    }
exit_stop:
    BSP_UART_ExitCritical(previous_primask);
    return status;
}

bool BSP_UART_IsTxIdle(BSP_UartPort_t port)
{
    /* TXE仅说明数据寄存器可写；TC才说明最后一字节已发完。 */
    uint32_t previous_primask;
    bool idle;
    if ((port >= BSP_UART_PORT_COUNT) || !s_uart_initialized[port]) { return false; }
    previous_primask = BSP_UART_EnterCritical();
    idle = s_tx_buffers[port].read_index == s_tx_buffers[port].write_index &&
           USART_GetFlagStatus(s_uart_configs[port].instance, USART_FLAG_TC) != RESET;
    BSP_UART_ExitCritical(previous_primask);
    return idle;
}

Project_Status_t BSP_UART_WriteString(BSP_UartPort_t port, const char *text)
{
    uint32_t length = 0U;

    if ((port >= BSP_UART_PORT_COUNT) || (text == NULL))
    {
        return PROJECT_STATUS_INVALID_ARGUMENT;
    }

    while (text[length] != '\0')
    {
        ++length;
    }

    return BSP_UART_Write(port, (const uint8_t *)text, length);
}

Project_Status_t BSP_UART_SetBaudRate(BSP_UartPort_t port, uint32_t baud_rate)
{
    const BSP_UartConfig_t *config;
    BSP_UartTxBuffer_t *tx_buffer;
    BSP_UartRxBuffer_t *rx_buffer;
    USART_InitTypeDef usart_init;
    uint32_t previous_primask;

    if ((port >= BSP_UART_PORT_COUNT) || (baud_rate == 0U))
    {
        return PROJECT_STATUS_INVALID_ARGUMENT;
    }
    if (!s_uart_initialized[port])
    {
        return PROJECT_STATUS_NOT_INITIALIZED;
    }

    config = &s_uart_configs[port];
    tx_buffer = &s_tx_buffers[port];
    rx_buffer = &s_rx_buffers[port];
    previous_primask = BSP_UART_EnterCritical();

    /* 正在发送时不能修改 BRR，否则同一帧的前后字节会使用不同波特率。 */
    if ((tx_buffer->read_index != tx_buffer->write_index) ||
        (USART_GetFlagStatus(config->instance, USART_FLAG_TC) == RESET))
    {
        BSP_UART_ExitCritical(previous_primask);
        return PROJECT_STATUS_ERROR;
    }

    USART_ITConfig(config->instance, USART_IT_TXE, DISABLE);
    USART_Cmd(config->instance, DISABLE);

    USART_StructInit(&usart_init);
    usart_init.USART_BaudRate = baud_rate;
    usart_init.USART_WordLength = USART_WordLength_8b;
    usart_init.USART_StopBits = USART_StopBits_1;
    usart_init.USART_Parity = USART_Parity_No;
    usart_init.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    usart_init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(config->instance, &usart_init);
    USART_Cmd(config->instance, ENABLE);

    /* 改波特率前收到的数据无法继续可靠解析，从当前 DMA 写入位置重新开始。 */
    rx_buffer->read_count = BSP_UART_GetProducedRxCount(port);
    BSP_UART_ExitCritical(previous_primask);
    return PROJECT_STATUS_OK;
}

void BSP_UART_DiscardRxData(BSP_UartPort_t port)
{
    uint32_t previous_primask;

    if ((port >= BSP_UART_PORT_COUNT) || !s_uart_initialized[port])
    {
        return;
    }

    previous_primask = BSP_UART_EnterCritical();
    s_rx_buffers[port].read_count = BSP_UART_GetProducedRxCount(port);
    BSP_UART_ExitCritical(previous_primask);
}

bool BSP_UART_TryReadByte(BSP_UartPort_t port, uint8_t *data)
{
    BSP_UartRxBuffer_t *buffer;
    uint32_t previous_primask;
    uint32_t produced_count;
    uint32_t available_count;

    if ((port >= BSP_UART_PORT_COUNT) || (data == NULL) || !s_uart_initialized[port])
    {
        return false;
    }

    buffer = &s_rx_buffers[port];
    previous_primask = BSP_UART_EnterCritical();

    /* DMA 的剩余计数和圈数合成一个单调递增的绝对写入计数。 */
    produced_count = BSP_UART_GetProducedRxCount(port);
    available_count = produced_count - buffer->read_count;

    if (available_count == 0U)
    {
        BSP_UART_ExitCritical(previous_primask);
        return false;
    }

    if (available_count > BSP_UART_RX_DMA_BUFFER_SIZE)
    {
        /* DMA 已覆盖旧数据：跳到当前仍保留的最早字节，并累计丢失量。 */
        buffer->dropped_byte_count += available_count - BSP_UART_RX_DMA_BUFFER_SIZE;
        buffer->read_count = produced_count - BSP_UART_RX_DMA_BUFFER_SIZE;
    }

    *data = buffer->data[buffer->read_count % BSP_UART_RX_DMA_BUFFER_SIZE];
    ++buffer->read_count;

    BSP_UART_ExitCritical(previous_primask);
    return true;
}

uint32_t BSP_UART_GetDroppedByteCount(BSP_UartPort_t port)
{
    uint32_t previous_primask;
    uint32_t dropped_byte_count;

    if (port >= BSP_UART_PORT_COUNT)
    {
        return 0U;
    }

    previous_primask = BSP_UART_EnterCritical();
    dropped_byte_count = s_rx_buffers[port].dropped_byte_count;
    BSP_UART_ExitCritical(previous_primask);

    return dropped_byte_count;
}

uint32_t BSP_UART_GetDmaErrorCount(BSP_UartPort_t port)
{
    uint32_t previous_primask;
    uint32_t dma_error_count;

    if (port >= BSP_UART_PORT_COUNT)
    {
        return 0U;
    }

    previous_primask = BSP_UART_EnterCritical();
    dma_error_count = s_rx_buffers[port].dma_error_count;
    BSP_UART_ExitCritical(previous_primask);

    return dma_error_count;
}

/* USART 中断现在只处理发送；接收由 DMA 硬件自动搬运。 */
void USART1_IRQHandler(void) { BSP_UART_HandleUsartInterrupt(BSP_UART_PORT_HOST); }

void USART2_IRQHandler(void) { BSP_UART_HandleUsartInterrupt(BSP_UART_PORT_VISION); }

void USART3_IRQHandler(void) { BSP_UART_HandleUsartInterrupt(BSP_UART_PORT_IMU); }

void USART6_IRQHandler(void) { BSP_UART_HandleUsartInterrupt(BSP_UART_PORT_MOTOR); }

/* 每个 DMA 中断只在对应的 256 字节缓冲区绕回起点时发生一次。 */
void DMA1_Stream1_IRQHandler(void) { BSP_UART_HandleRxDmaInterrupt(BSP_UART_PORT_IMU); }

void DMA1_Stream5_IRQHandler(void) { BSP_UART_HandleRxDmaInterrupt(BSP_UART_PORT_VISION); }

void DMA2_Stream1_IRQHandler(void) { BSP_UART_HandleRxDmaInterrupt(BSP_UART_PORT_MOTOR); }

void DMA2_Stream2_IRQHandler(void) { BSP_UART_HandleRxDmaInterrupt(BSP_UART_PORT_HOST); }

static uint32_t BSP_UART_EnterCritical(void)
{
    uint32_t previous_primask = __get_PRIMASK();

    __disable_irq();
    return previous_primask;
}

static void BSP_UART_ExitCritical(uint32_t previous_primask)
{
    /* 只有进入前允许中断时才重新打开，支持在中断上下文中安全调用。 */
    if (previous_primask == 0U)
    {
        __enable_irq();
    }
}

static void BSP_UART_ConfigurePin(GPIO_TypeDef *gpio, uint16_t pin, uint8_t pin_source,
                                  uint8_t alternate_function, GPIOPuPd_TypeDef pull_mode)
{
    GPIO_InitTypeDef gpio_init;

    GPIO_PinAFConfig(gpio, pin_source, alternate_function);

    GPIO_StructInit(&gpio_init);
    gpio_init.GPIO_Pin = pin;
    gpio_init.GPIO_Mode = GPIO_Mode_AF;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init.GPIO_OType = GPIO_OType_PP;
    gpio_init.GPIO_PuPd = pull_mode;
    GPIO_Init(gpio, &gpio_init);
}

static Project_Status_t BSP_UART_ConfigureRxDma(BSP_UartPort_t port)
{
    const BSP_UartConfig_t *config = &s_uart_configs[port];
    BSP_UartRxBuffer_t *buffer = &s_rx_buffers[port];
    DMA_InitTypeDef dma_init;
    uint32_t timeout = BSP_UART_DMA_DISABLE_TIMEOUT;

    DMA_Cmd(config->rx_dma_stream, DISABLE);
    while (DMA_GetCmdStatus(config->rx_dma_stream) != DISABLE)
    {
        if (timeout == 0U)
        {
            return PROJECT_STATUS_TIMEOUT;
        }
        --timeout;
    }
    DMA_DeInit(config->rx_dma_stream);

    DMA_StructInit(&dma_init);
    dma_init.DMA_Channel = config->rx_dma_channel;
    dma_init.DMA_PeripheralBaseAddr = (uint32_t)&config->instance->DR;
    dma_init.DMA_Memory0BaseAddr = (uint32_t)buffer->data;
    dma_init.DMA_DIR = DMA_DIR_PeripheralToMemory;
    dma_init.DMA_BufferSize = BSP_UART_RX_DMA_BUFFER_SIZE;
    dma_init.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dma_init.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dma_init.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma_init.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    dma_init.DMA_Mode = DMA_Mode_Circular;
    dma_init.DMA_Priority = DMA_Priority_High;
    dma_init.DMA_FIFOMode = DMA_FIFOMode_Disable;
    dma_init.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
    dma_init.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    dma_init.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
    DMA_Init(config->rx_dma_stream, &dma_init);

    /* TC 记录绕回次数；TE 记录 DMA 总线或传输错误。 */
    DMA_ITConfig(config->rx_dma_stream, DMA_IT_TC | DMA_IT_TE, ENABLE);
    NVIC_ClearPendingIRQ(config->rx_dma_interrupt_number);
    NVIC_SetPriority(config->rx_dma_interrupt_number,
                     NVIC_EncodePriority(BSP_UART_NVIC_PRIORITY_GROUPING,
                                         BSP_UART_DMA_INTERRUPT_PRIORITY, 0U));
    NVIC_EnableIRQ(config->rx_dma_interrupt_number);

    USART_DMACmd(config->instance, USART_DMAReq_Rx, ENABLE);
    DMA_Cmd(config->rx_dma_stream, ENABLE);

    return PROJECT_STATUS_OK;
}

static Project_Status_t BSP_UART_InitPort(BSP_UartPort_t port)
{
    const BSP_UartConfig_t *config;
    BSP_UartRxBuffer_t *rx_buffer;
    BSP_UartTxBuffer_t *tx_buffer;
    USART_InitTypeDef usart_init;
    Project_Status_t status;

    if (port >= BSP_UART_PORT_COUNT)
    {
        return PROJECT_STATUS_INVALID_ARGUMENT;
    }

    config = &s_uart_configs[port];
    rx_buffer = &s_rx_buffers[port];
    tx_buffer = &s_tx_buffers[port];
    s_uart_initialized[port] = false;

    /* TX 空闲时为高电平；RX 使用内部上拉，避免线路未接时悬空。 */
    BSP_UART_ConfigurePin(config->tx_gpio, config->tx_pin, config->tx_pin_source,
                          config->alternate_function, GPIO_PuPd_NOPULL);
    BSP_UART_ConfigurePin(config->rx_gpio, config->rx_pin, config->rx_pin_source,
                          config->alternate_function, GPIO_PuPd_UP);

    USART_DeInit(config->instance);
    USART_StructInit(&usart_init);
    usart_init.USART_BaudRate = config->baud_rate;
    usart_init.USART_WordLength = USART_WordLength_8b;
    usart_init.USART_StopBits = USART_StopBits_1;
    usart_init.USART_Parity = USART_Parity_No;
    usart_init.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    usart_init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(config->instance, &usart_init);

    rx_buffer->dma_wrap_count = 0U;
    rx_buffer->read_count = 0U;
    rx_buffer->dropped_byte_count = 0U;
    rx_buffer->dma_error_count = 0U;
    tx_buffer->write_index = 0U;
    tx_buffer->read_index = 0U;
    tx_buffer->frame_in_progress = false;

    /* USART IRQ 只在发送队列非空时打开 TXE 中断。 */
    NVIC_ClearPendingIRQ(config->usart_interrupt_number);
    NVIC_SetPriority(config->usart_interrupt_number,
                     NVIC_EncodePriority(BSP_UART_NVIC_PRIORITY_GROUPING,
                                         BSP_UART_USART_INTERRUPT_PRIORITY, 0U));
    NVIC_EnableIRQ(config->usart_interrupt_number);
    USART_ITConfig(config->instance, USART_IT_TXE, DISABLE);

    status = BSP_UART_ConfigureRxDma(port);
    if (status != PROJECT_STATUS_OK)
    {
        return status;
    }
    USART_Cmd(config->instance, ENABLE);

    s_uart_initialized[port] = true;
    return PROJECT_STATUS_OK;
}

static uint32_t BSP_UART_GetProducedRxCount(BSP_UartPort_t port)
{
    const BSP_UartConfig_t *config = &s_uart_configs[port];
    BSP_UartRxBuffer_t *buffer = &s_rx_buffers[port];
    uint32_t remaining_count;
    uint32_t wrap_count;

    /*
     * dma_wrap_count 只允许 DMA ISR 修改。本函数在关中断状态下读取快照。
     * 若 TC 已挂起但 ISR 尚未执行，只对本地圈数补 1，并重读 NDTR；
     * 不清标志、不修改共享圈数，退出临界区后仍由 ISR 统一处理。
     */
    wrap_count = buffer->dma_wrap_count;
    remaining_count = DMA_GetCurrDataCounter(config->rx_dma_stream);

    if (DMA_GetFlagStatus(config->rx_dma_stream, config->rx_dma_tc_flag) != RESET)
    {
        ++wrap_count;
        remaining_count = DMA_GetCurrDataCounter(config->rx_dma_stream);
    }

    return (wrap_count * BSP_UART_RX_DMA_BUFFER_SIZE) +
           (BSP_UART_RX_DMA_BUFFER_SIZE - remaining_count);
}

static void BSP_UART_HandleUsartInterrupt(BSP_UartPort_t port)
{
    const BSP_UartConfig_t *config = &s_uart_configs[port];
    BSP_UartTxBuffer_t *buffer = &s_tx_buffers[port];

    if (USART_GetITStatus(config->instance, USART_IT_TXE) != RESET)
    {
        uint8_t byte;
        if (BSP_UART_TxPopByte(buffer, &byte))
        {
            USART_SendData(config->instance, byte);
        }
        else
        {
            /* 队列已空，关闭 TXE 中断，避免空转进入中断。 */
            USART_ITConfig(config->instance, USART_IT_TXE, DISABLE);
        }
    }
}

static void BSP_UART_HandleRxDmaInterrupt(BSP_UartPort_t port)
{
    const BSP_UartConfig_t *config = &s_uart_configs[port];

    if (DMA_GetITStatus(config->rx_dma_stream, config->rx_dma_te_interrupt) != RESET)
    {
        DMA_ClearITPendingBit(config->rx_dma_stream, config->rx_dma_te_interrupt);
        ++s_rx_buffers[port].dma_error_count;
    }

    if (DMA_GetITStatus(config->rx_dma_stream, config->rx_dma_tc_interrupt) != RESET)
    {
        DMA_ClearITPendingBit(config->rx_dma_stream, config->rx_dma_tc_interrupt);
        ++s_rx_buffers[port].dma_wrap_count;
    }
}
