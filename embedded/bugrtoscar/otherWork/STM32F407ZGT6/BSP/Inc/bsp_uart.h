/**
 * @file bsp_uart.h
 * @brief 开发板四路串口的统一接口。
 *
 * 串口用途与引脚分配：
 * - 上位机：USART1，PA9(TX) / PA10(RX)，可连接 CH340 或 Linux 主控；
 * - 视觉：USART2，PA2(TX) / PA3(RX)；
 * - IMU：USART3，PB10(TX) / PB11(RX)；
 * - 电机：USART6，PG14(TX) / PG9(RX)。
 */

#ifndef BSP_UART_H_
#define BSP_UART_H_

#include <stdbool.h>
#include <stdint.h>

#include "project_status.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /** 串口的逻辑用途，上层代码不需要直接使用 USART1/2/3/6。 */
    typedef enum
    {
        BSP_UART_PORT_HOST = 0, /**< 上位机通信，可接 Windows、Linux 或其他主控。 */
        BSP_UART_PORT_VISION, /**< 视觉模块通信串口。 */
        BSP_UART_PORT_IMU,    /**< 陀螺仪/IMU 通信串口。 */
        BSP_UART_PORT_MOTOR,  /**< Emm_V5 电机驱动器通信串口。 */
        BSP_UART_PORT_COUNT   /**< 串口总数，只用于参数检查和数组长度。 */
    } BSP_UartPort_t;

    /**
     * @brief 使用 STM32F4 标准外设库初始化全部四路串口。
     * @return 初始化结果；成功返回 PROJECT_STATUS_OK。
     */
    Project_Status_t BSP_UART_Init(void);

    /**
     * @brief 通过指定串口发送一段二进制数据。
     * @param port 串口的逻辑用途。
     * @param data 待发送数据地址；length 非零时不能为 NULL。
     * @param length 待发送字节数。
     * @return 入队成功返回 OK；发送队列空间不足返回 ERROR，不会阻塞等待。
     */
    Project_Status_t BSP_UART_Write(BSP_UartPort_t port, const uint8_t *data, uint32_t length);

    /**
     * @brief 通过指定串口发送以 '\0' 结尾的字符串。
     * @param port 串口的逻辑用途。
     * @param text 待发送字符串。
     * @return 入队成功返回 OK；发送队列空间不足返回 ERROR，不会阻塞等待。
     */
    Project_Status_t BSP_UART_WriteString(BSP_UartPort_t port, const char *text);

    /** 故障路径专用：保留已开始帧尾、丢弃其余帧，有界轮询三次STOP，不依赖中断。 */
    Project_Status_t BSP_UART_EmergencyMotorStop(void);
    /** 优先广播停车：仅保留已开始发送的帧尾，丢弃其余排队动作。 */
    Project_Status_t BSP_UART_MotorStop(void);
    /** 软件队列为空且硬件 TC 置位才返回 true。 */
    bool BSP_UART_IsTxIdle(BSP_UartPort_t port);

    /**
     * @brief 在运行时修改一路串口的波特率。
     * @param port 串口的逻辑用途。
     * @param baud_rate 新波特率，不能为 0。
     * @return 修改成功返回OK；队列未空或硬件TC未完成返回ERROR，请稍后重试。
     * @note 数据位、停止位、校验和 DMA 配置保持不变。
     */
    Project_Status_t BSP_UART_SetBaudRate(BSP_UartPort_t port, uint32_t baud_rate);

    /**
     * @brief 丢弃指定串口 DMA 缓冲区内尚未读取的所有旧数据。
     * @param port 串口的逻辑用途。
     */
    void BSP_UART_DiscardRxData(BSP_UartPort_t port);

    /**
     * @brief 非阻塞读取一个字节。
     * @param port 串口的逻辑用途。
     * @param data 用于保存读取结果的地址。
     * @return 读取到数据返回 true；当前无数据或参数错误返回 false。
     */
    bool BSP_UART_TryReadByte(BSP_UartPort_t port, uint8_t *data);

    /**
     * @brief 获取因 DMA 接收缓冲区被覆盖而丢失的字节总数。
     * @param port 串口的逻辑用途。
     * @return 累计丢弃字节数；端口无效时返回 0。
     */
    uint32_t BSP_UART_GetDroppedByteCount(BSP_UartPort_t port);

    /**
     * @brief 获取 DMA 接收传输错误的累计次数。
     * @param port 串口的逻辑用途。
     * @return DMA TE 错误累计次数；端口无效时返回 0。
     */
    uint32_t BSP_UART_GetDmaErrorCount(BSP_UartPort_t port);

#ifdef __cplusplus
}
#endif

#endif /* BSP_UART_H_ */
