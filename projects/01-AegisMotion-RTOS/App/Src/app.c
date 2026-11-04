/**
 * @file app.c
 * @brief 应用层初始化、JY61P 数据上报和上位机多电机控制。
 */

#include "app.h"

#include <stdbool.h>

#include "app_config.h"
#include "bsp.h"
#include "bsp_uart.h"
#include "stm32f407xx.h"

#if (APP_CONFIG_HOST_CONTROL_ENABLED == 1U)
#include "mecanum_chassis.h"
#include "host_protocol.h"
#endif

#if (APP_CONFIG_IMU_ENABLED == 1U)
#include "imu.h"
#endif

#if ((APP_CONFIG_IMU_ENABLED == 1U) && (APP_CONFIG_HOST_CONTROL_ENABLED == 1U))
/* 一行完整 IMU 文本所需的最大缓冲区长度。 */
#define APP_IMU_TEXT_BUFFER_SIZE (256U)

/* 等待装入上位机二进制事件帧的一行 IMU 文本。 */
static uint8_t s_imu_text_buffer[APP_IMU_TEXT_BUFFER_SIZE];
static uint32_t s_imu_text_length;
static bool s_imu_text_pending;

/** 向文本缓冲区追加一个以 '\0' 结尾的字符串。 */
static uint32_t App_AppendText(uint8_t *buffer, uint32_t capacity, uint32_t length,
                               const char *text);

/** 向文本缓冲区追加一个无符号十进制整数。 */
static uint32_t App_AppendUnsigned(uint8_t *buffer, uint32_t capacity, uint32_t length,
                                   uint32_t value);

/** 向文本缓冲区追加一个保留三位小数的浮点数，不依赖 printf 浮点支持。 */
static uint32_t App_AppendFixed3(uint8_t *buffer, uint32_t capacity, uint32_t length, float value);

/** 把一组加速度、角速度、姿态角、温度和版本号格式化为一行 ASCII 文本。 */
static uint32_t App_FormatImuSample(const IMU_Sample_t *sample, uint8_t *buffer, uint32_t capacity);
#endif

static App_State_t s_app_state = APP_STATE_UNINITIALIZED;
static volatile Project_Status_t s_last_fault = PROJECT_STATUS_OK;

Project_Status_t App_Init(void)
{
    Project_Status_t status;

    if (s_app_state == APP_STATE_READY)
    {
        return PROJECT_STATUS_OK;
    }

    /* 初始化 USART1/2/3/6、GPIO、DMA 和中断。 */
    status = BSP_Init();
    if (status != PROJECT_STATUS_OK)
    {
        s_app_state = APP_STATE_FAULT;
        return status;
    }

#if (APP_CONFIG_HOST_CONTROL_ENABLED == 1U)
    /* 协议初始化先请求停车、启动自检；此时还未等待IMU上电配置完成。 */
    status = HostProtocol_Init();
    if (status != PROJECT_STATUS_OK)
    {
        s_app_state = APP_STATE_FAULT;
        return status;
    }
#endif

#if (APP_CONFIG_IMU_ENABLED == 1U)
    /*
     * 配置 JY61P 动态滤波，并打开加速度、角速度和姿态角三类输出帧。
     * IMU_Init() 不执行 Yaw 归零，也不修改串口波特率。
     */
    status = IMU_Init();
    if (status != PROJECT_STATUS_OK)
    {
        s_app_state = APP_STATE_FAULT;
        return status;
    }

#if (APP_CONFIG_HOST_CONTROL_ENABLED == 1U)
    s_imu_text_length = 0U;
    s_imu_text_pending = false;
#endif
#endif

#if (APP_CONFIG_HOST_CONTROL_ENABLED == 1U)
    /*
     * 该事件只发送一次，用来确认 STM32 和上位机串口链路正常。
     * 即使关闭 IMU 功能，上位机仍可收到 READY 并使用电机控制功能。
     */
    status = HostProtocol_SendEvent(HOST_PROTOCOL_EVENT_READY, (const uint8_t *)0, 0U);
    if (status != PROJECT_STATUS_OK)
    {
        s_app_state = APP_STATE_FAULT;
        return status;
    }
#endif

    /* READY表示应用初始化完成；是否允许运动仍由HostProtocol自检/故障门禁决定。 */
    s_app_state = APP_STATE_READY;
    return PROJECT_STATUS_OK;
}

Project_Status_t App_ProcessControl(void)
{
#if (APP_CONFIG_HOST_CONTROL_ENABLED == 1U)
    Project_Status_t status;
#endif

    if (s_app_state != APP_STATE_READY)
    {
        return PROJECT_STATUS_NOT_INITIALIZED;
    }

#if (APP_CONFIG_HOST_CONTROL_ENABLED == 1U)
    /* 先接收上一台电机的回复，使麦轮状态机可以确认后再发送下一地址。 */
    status = HostProtocol_Process();
    if (status != PROJECT_STATUS_OK)
    {
        s_app_state = APP_STATE_FAULT;
        return status;
    }

    /* 分步发送麦轮四电机命令；每轮最多向 USART6 放入一条命令。 */
    status = Mecanum_Process();
    if (status != PROJECT_STATUS_OK)
    {
        s_app_state = APP_STATE_FAULT;
        return status;
    }
#endif

    return PROJECT_STATUS_OK;
}

Project_Status_t App_ProcessData(void)
{
#if (APP_CONFIG_IMU_ENABLED == 1U)
    Project_Status_t status;
#endif

#if ((APP_CONFIG_IMU_ENABLED == 1U) && (APP_CONFIG_HOST_CONTROL_ENABLED == 1U))
    IMU_Sample_t sample;
#endif

    if (s_app_state != APP_STATE_READY)
    {
        return PROJECT_STATUS_NOT_INITIALIZED;
    }

#if (APP_CONFIG_IMU_ENABLED == 1U)
    /* 从 USART3 DMA 缓冲区取出数据并解析 0x51、0x52、0x53 帧。 */
    status = IMU_Process();
    if (status != PROJECT_STATUS_OK)
    {
        s_app_state = APP_STATE_FAULT;
        return status;
    }

#if (APP_CONFIG_HOST_CONTROL_ENABLED == 1U)
    /* 上位机发送队列暂满时，先重试上一轮尚未发送的完整载荷。 */
    if (s_imu_text_pending)
    {
        status = HostProtocol_SendEvent(HOST_PROTOCOL_EVENT_IMU_TEXT, s_imu_text_buffer,
                                        (uint8_t)s_imu_text_length);
        if (status == PROJECT_STATUS_ERROR)
        {
            return PROJECT_STATUS_OK;
        }
        if (status != PROJECT_STATUS_OK)
        {
            s_app_state = APP_STATE_FAULT;
            return status;
        }
        s_imu_text_pending = false;
    }

    /* 三类数据帧都更新后，格式化并发送一行新的 ASCII 文本。 */
    if (IMU_TryRead(&sample))
    {
        s_imu_text_length =
            App_FormatImuSample(&sample, s_imu_text_buffer, APP_IMU_TEXT_BUFFER_SIZE);
        status = HostProtocol_SendEvent(HOST_PROTOCOL_EVENT_IMU_TEXT, s_imu_text_buffer,
                                        (uint8_t)s_imu_text_length);
        if (status == PROJECT_STATUS_ERROR)
        {
            s_imu_text_pending = true;
            return PROJECT_STATUS_OK;
        }
        if (status != PROJECT_STATUS_OK)
        {
            s_app_state = APP_STATE_FAULT;
            return status;
        }
    }
#endif
#endif

    return PROJECT_STATUS_OK;
}

App_State_t App_GetState(void) { return s_app_state; }

Project_Status_t App_GetLastFault(void) { return s_last_fault; }

void App_EnterFault(Project_Status_t status)
{
    s_last_fault = status;
    s_app_state = APP_STATE_FAULT;
    __disable_irq();
#if (APP_CONFIG_HOST_CONTROL_ENABLED == 1U)
    /* 不可只入 TX 队列：此时中断已关闭，必须使用有界的硬件轮询发送。 */
    (void)BSP_UART_EmergencyMotorStop();
#endif

    for (;;)
    {
        __NOP();
    }
}

#if ((APP_CONFIG_IMU_ENABLED == 1U) && (APP_CONFIG_HOST_CONTROL_ENABLED == 1U))
static uint32_t App_AppendText(uint8_t *buffer, uint32_t capacity, uint32_t length,
                               const char *text)
{
    while ((*text != '\0') && (length < capacity))
    {
        buffer[length] = (uint8_t)*text;
        ++length;
        ++text;
    }

    return length;
}

static uint32_t App_AppendUnsigned(uint8_t *buffer, uint32_t capacity, uint32_t length,
                                   uint32_t value)
{
    uint8_t digits[10];
    uint32_t digit_count = 0U;

    /* 先按低位到高位保存，再反向写入输出缓冲区。 */
    do
    {
        digits[digit_count] = (uint8_t)('0' + (value % 10U));
        ++digit_count;
        value /= 10U;
    } while ((value != 0U) && (digit_count < sizeof(digits)));

    while ((digit_count > 0U) && (length < capacity))
    {
        --digit_count;
        buffer[length] = digits[digit_count];
        ++length;
    }

    return length;
}

static uint32_t App_AppendFixed3(uint8_t *buffer, uint32_t capacity, uint32_t length, float value)
{
    uint32_t scaled_value;
    uint32_t fractional_value;

    /* 先单独输出负号，后续统一按正数转换。 */
    if (value < 0.0f)
    {
        if (length < capacity)
        {
            buffer[length] = (uint8_t)'-';
            ++length;
        }
        value = -value;
    }

    /* 乘 1000 并四舍五入，将整数和三位小数统一转换为整数。 */
    scaled_value = (uint32_t)((value * 1000.0f) + 0.5f);
    fractional_value = scaled_value % 1000U;
    length = App_AppendUnsigned(buffer, capacity, length, scaled_value / 1000U);

    if (length < capacity)
    {
        buffer[length] = (uint8_t)'.';
        ++length;
    }
    if (length < capacity)
    {
        buffer[length] = (uint8_t)('0' + (fractional_value / 100U));
        ++length;
    }
    if (length < capacity)
    {
        buffer[length] = (uint8_t)('0' + ((fractional_value / 10U) % 10U));
        ++length;
    }
    if (length < capacity)
    {
        buffer[length] = (uint8_t)('0' + (fractional_value % 10U));
        ++length;
    }

    return length;
}

static uint32_t App_FormatImuSample(const IMU_Sample_t *sample, uint8_t *buffer, uint32_t capacity)
{
    uint32_t length = 0U;

    /* 加速度，单位 g，顺序为 X/Y/Z。 */
    length = App_AppendText(buffer, capacity, length, "ACC[g]=");
    length = App_AppendFixed3(buffer, capacity, length, sample->acceleration_x_g);
    length = App_AppendText(buffer, capacity, length, ",");
    length = App_AppendFixed3(buffer, capacity, length, sample->acceleration_y_g);
    length = App_AppendText(buffer, capacity, length, ",");
    length = App_AppendFixed3(buffer, capacity, length, sample->acceleration_z_g);

    /* 角速度，单位 degree/second，顺序为 X/Y/Z。 */
    length = App_AppendText(buffer, capacity, length, ";GYRO[dps]=");
    length = App_AppendFixed3(buffer, capacity, length, sample->angular_velocity_x_dps);
    length = App_AppendText(buffer, capacity, length, ",");
    length = App_AppendFixed3(buffer, capacity, length, sample->angular_velocity_y_dps);
    length = App_AppendText(buffer, capacity, length, ",");
    length = App_AppendFixed3(buffer, capacity, length, sample->angular_velocity_z_dps);

    /* 姿态角按用户要求使用 Yaw/Pitch/Roll 顺序，单位 degree。 */
    length = App_AppendText(buffer, capacity, length, ";YPR[deg]=");
    length = App_AppendFixed3(buffer, capacity, length, sample->yaw_deg);
    length = App_AppendText(buffer, capacity, length, ",");
    length = App_AppendFixed3(buffer, capacity, length, sample->pitch_deg);
    length = App_AppendText(buffer, capacity, length, ",");
    length = App_AppendFixed3(buffer, capacity, length, sample->roll_deg);

    /* 温度来自 0x51 帧，版本号来自 0x53 帧最后两个数据字节。 */
    length = App_AppendText(buffer, capacity, length, ";TEMP[C]=");
    length = App_AppendFixed3(buffer, capacity, length, sample->temperature_c);
    length = App_AppendText(buffer, capacity, length, ";VER=");
    length = App_AppendUnsigned(buffer, capacity, length, sample->version);
    length = App_AppendText(buffer, capacity, length, "\r\n");

    return length;
}
#endif
