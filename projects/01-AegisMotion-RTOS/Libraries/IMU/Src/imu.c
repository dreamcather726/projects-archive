/**
 * @file imu.c
 * @brief JY61P 串口 IMU 驱动实现。
 */

#include "imu.h"

#include <stddef.h>
#include <string.h>

#include "app_config.h"
#include "bsp_uart.h"
#include "stm32f407xx.h"

/* JY61P 主动上传帧的固定格式：0x55 + 类型 + 8 字节数据 + 1 字节校验和。 */
#define IMU_JY61P_FRAME_HEADER (0x55U)
#define IMU_JY61P_FRAME_LENGTH (11U)

/* 三类运动数据帧：加速度、角速度和姿态角。 */
#define IMU_JY61P_FRAME_ACCELERATION (0x51U)
#define IMU_JY61P_FRAME_ANGULAR_VELOCITY (0x52U)
#define IMU_JY61P_FRAME_ANGLE (0x53U)

/* 收齐三类运动数据后才向应用层发布一组完整样本。 */
#define IMU_JY61P_UPDATE_ACCELERATION (0x01U)
#define IMU_JY61P_UPDATE_ANGULAR_VELOCITY (0x02U)
#define IMU_JY61P_UPDATE_ANGLE (0x04U)
#define IMU_JY61P_UPDATE_ALL (0x07U)

/* JY61P 写寄存器命令格式：FF AA REG DATA_L DATA_H，共 5 字节。 */
#define IMU_JY61P_COMMAND_LENGTH (5U)

/* 等待传感器完成上电启动后再修改滤波参数。 */
#define IMU_JY61P_POWER_ON_DELAY_MS (500U)

/* 官方示例要求命令之间延时；200ms 也远小于文档规定的 10 秒有效间隔。 */
#define IMU_JY61P_COMMAND_DELAY_MS (200U)

/* KEY 解锁寄存器及其 16 位解锁值。命令中低字节 0x88 在前、高字节 0xB5 在后。 */
#define IMU_JY61P_REGISTER_KEY (0x69U)
#define IMU_JY61P_KEY_UNLOCK (0xB588U)

/* 0x2A 是加速度动态滤波寄存器 ACCFILT，有效范围为 1~10000。 */
#define IMU_JY61P_REGISTER_ACCEL_FILTER (0x2AU)

/* 0x63 控制陀螺仪自动校准；官方协议中 0 表示开启，1 表示关闭。 */
#define IMU_JY61P_REGISTER_GYRO_AUTO_CALIBRATION (0x63U)
#define IMU_JY61P_GYRO_AUTO_CALIBRATION_ENABLE (0x0000U)

/* 0x02 是输出内容寄存器 RSW；0x000E 打开加速度、角速度和姿态角帧。 */
#define IMU_JY61P_REGISTER_OUTPUT_CONTENT (0x02U)
#define IMU_JY61P_OUTPUT_ACCEL_GYRO_ANGLE (0x000EU)

/*
 * 官方建议在振动环境下取 200~500；这里使用示例值 300。
 * 数值越小，Roll/Pitch 抗振效果越强，但姿态变化的响应也越慢。
 */
#define IMU_JY61P_ACCEL_FILTER_VALUE (300U)

#if ((IMU_JY61P_ACCEL_FILTER_VALUE < 1U) || (IMU_JY61P_ACCEL_FILTER_VALUE > 10000U))
#error "IMU_JY61P_ACCEL_FILTER_VALUE must be in range 1..10000"
#endif

/*
 * JY61P 是六轴 IMU，Yaw 没有磁场或其他绝对航向参考，只能由 Z 轴角速度积分。
 * 即使传感器内部开启自动校准，静止时仍可能残留很小的积分漂移。以下参数用于
 * STM32 端的零速更新：连续若干组数据同时满足“角速度很小、合加速度接近 1g”
 * 后锁住输出 Yaw，并实时吸收传感器的慢速漂移；检测到运动后保持修正量继续输出。
 *
 * 阈值不能区分真正的超慢速匀速旋转和零偏漂移，这是所有无绝对航向参考的六轴
 * IMU 的物理限制。若应用必须测量低于该阈值的匀速转动，应降低阈值或使用九轴 IMU。
 */
#define IMU_YAW_STATIONARY_GYRO_THRESHOLD_DPS (2.5f)
#define IMU_YAW_STATIONARY_ACCEL_MIN_SQUARED_G (0.81f) /* 0.90g 的平方。 */
#define IMU_YAW_STATIONARY_ACCEL_MAX_SQUARED_G (1.21f) /* 1.10g 的平方。 */
#define IMU_YAW_STATIONARY_CONFIRM_SAMPLES (20U)

/* 三类数据共用的有符号 16 位原始量程以及各自的物理量满量程。 */
#define IMU_JY61P_RAW_SCALE (32768.0f)
#define IMU_JY61P_ACCELERATION_RANGE_G (16.0f)
#define IMU_JY61P_ANGULAR_VELOCITY_RANGE_DPS (2000.0f)
#define IMU_JY61P_ANGLE_RANGE_DEG (180.0f)
#define IMU_JY61P_TEMPERATURE_SCALE (100.0f)

static uint8_t s_imu_initialized = 0U;          /**< 1 表示驱动已经完成初始化。 */
static uint8_t s_frame[IMU_JY61P_FRAME_LENGTH]; /**< 正在接收的一帧 JY61P 数据。 */
static uint8_t s_frame_index; /**< 下一字节在 s_frame 中的写入位置。 */
static uint8_t s_update_mask; /**< 标记本组数据已收到的 0x51/0x52/0x53 帧。 */
static bool s_has_sample;     /**< 是否至少成功解析过一组完整运动数据。 */
static bool s_sample_pending; /**< 是否有一组尚未被应用层取走的完整数据。 */
static uint32_t s_valid_frame_count;    /**< 校验正确的数据帧累计数量。 */
static volatile uint32_t s_sample_count; /**< 完整样本数，控制任务只读。 */
static uint32_t s_checksum_error_count; /**< 校验失败的数据帧累计数量。 */
static uint32_t s_baud_rate; /**< 当前 USART3 使用的固定波特率。 */
static IMU_Sample_t s_latest_sample;    /**< 正在由三类数据帧更新的工作样本。 */
static IMU_Sample_t s_published_sample; /**< 最近一次收齐并发布的完整运动数据。 */
static bool s_yaw_filter_initialized;   /**< Yaw 零速修正状态是否已有初始角度。 */
static bool s_yaw_stationary_locked;    /**< 当前是否判定静止并锁住输出 Yaw。 */
static uint16_t s_yaw_stationary_count; /**< 连续满足静止条件的完整样本数量。 */
static float s_yaw_correction_deg;      /**< 原始 Yaw 到连续稳定 Yaw 的修正量。 */
static float s_yaw_locked_deg;          /**< 静止锁定期间保持不变的 Yaw。 */

/** 将低字节在前的两个字节还原为有符号 16 位原始值。 */
static int16_t IMU_DecodeInt16(uint8_t low_byte, uint8_t high_byte);

/** 使用 Cortex-M4 的 DWT 周期计数器实现上电配置所需的毫秒延时。 */
static void IMU_DelayMs(uint32_t milliseconds);

/** 清空 Yaw 静止修正状态。 */
static void IMU_ResetYawFilterState(void);

/** 按 FF AA REG DATA_L DATA_H 格式向 JY61P 写一个 16 位寄存器。 */
static Project_Status_t IMU_WriteRegister(uint8_t register_address, uint16_t value);

/** 解锁配置、写入一个寄存器，并保留协议要求的命令间隔。 */
static Project_Status_t IMU_WriteProtectedRegister(uint8_t register_address, uint16_t value);

/** 上电时设置动态滤波，并打开加速度、角速度和姿态角输出。 */
static Project_Status_t IMU_ConfigureOnPowerUp(void);

/** 把角度约束到 [-180, 180]，避免跨越边界时出现 360 度跳变。 */
static float IMU_WrapAngleDeg(float angle_deg);

/** 判断数值是否位于以 0 为中心的对称阈值内。 */
static bool IMU_IsWithinThreshold(float value, float threshold);

/** 检测静止状态并修正六轴积分造成的 Yaw 缓慢漂移。 */
static void IMU_ApplyYawStationaryLock(IMU_Sample_t *sample);

/** 解析一帧校验正确的 0x51、0x52 或 0x53 运动数据。 */
static void IMU_DecodeFrame(const uint8_t *frame);

/** 将一个串口字节送入帧接收状态机。 */
static void IMU_ParseByte(uint8_t data);

/** 校验失败后在旧数据中寻找下一个 0x55，以尽快恢复同步。 */
static void IMU_ResynchronizeFrame(void);

Project_Status_t IMU_Init(void)
{
    Project_Status_t status;

    /* 清空上一次运行可能留下的帧、角度和统计状态。 */
    memset(s_frame, 0, sizeof(s_frame));
    memset(&s_latest_sample, 0, sizeof(s_latest_sample));
    memset(&s_published_sample, 0, sizeof(s_published_sample));
    s_frame_index = 0U;
    s_update_mask = 0U;
    s_has_sample = false;
    s_sample_pending = false;
    s_valid_frame_count = 0U;
    s_sample_count = 0U;
    s_checksum_error_count = 0U;
    s_baud_rate = APP_CONFIG_IMU_BAUD_RATE;
    IMU_ResetYawFilterState();

    /*
     * USART3 已由 BSP 固定配置为 9600。等待模块启动后直接设置滤波和输出内容，
     * 不再扫描其他波特率，缩短上电初始化时间并避免向错误速率发送配置命令。
     */
    IMU_DelayMs(IMU_JY61P_POWER_ON_DELAY_MS);
    status = IMU_ConfigureOnPowerUp();
    if (status != PROJECT_STATUS_OK)
    {
        return status;
    }

    /* 配置期间缓存的旧字节不能作为启动自检的实时在线证据。 */
    BSP_UART_DiscardRxData(BSP_UART_PORT_IMU);
    s_imu_initialized = 1U;
    return PROJECT_STATUS_OK;
}

Project_Status_t IMU_Process(void)
{
    uint8_t data;

    if (s_imu_initialized == 0U)
    {
        return PROJECT_STATUS_NOT_INITIALIZED;
    }

    /*
     * USART3 的 DMA 在后台接收数据；这里逐字节取出已有数据并组装成完整帧。
     * 缓冲区暂时没有数据时，BSP_UART_TryReadByte() 返回 false，循环立即结束。
     */
    while (BSP_UART_TryReadByte(BSP_UART_PORT_IMU, &data))
    {
        IMU_ParseByte(data);
    }

    return PROJECT_STATUS_OK;
}

Project_Status_t IMU_Read(IMU_Sample_t *sample)
{
    if (sample == NULL)
    {
        return PROJECT_STATUS_INVALID_ARGUMENT;
    }

    if (s_imu_initialized == 0U)
    {
        return PROJECT_STATUS_NOT_INITIALIZED;
    }

    if (!s_has_sample)
    {
        /* 串口尚未收到过有效 0x53 帧，因此没有可返回的角度。 */
        return PROJECT_STATUS_ERROR;
    }

    /* IMU_Read() 可重复读取最近值，不会清除“新数据”标志。 */
    *sample = s_published_sample;
    return PROJECT_STATUS_OK;
}

bool IMU_TryRead(IMU_Sample_t *sample)
{
    if ((sample == NULL) || (s_imu_initialized == 0U) || !s_sample_pending)
    {
        return false;
    }

    /* IMU_TryRead() 每组新数据只交给应用层一次。 */
    *sample = s_published_sample;
    s_sample_pending = false;
    return true;
}

uint32_t IMU_GetValidFrameCount(void) { return s_valid_frame_count; }
uint32_t IMU_GetSampleCount(void) { return s_sample_count; }

uint32_t IMU_GetChecksumErrorCount(void) { return s_checksum_error_count; }

uint32_t IMU_GetBaudRate(void) { return s_baud_rate; }

bool IMU_IsYawStationaryLocked(void) { return s_yaw_stationary_locked; }

static int16_t IMU_DecodeInt16(uint8_t low_byte, uint8_t high_byte)
{
    /* JY61P 使用小端格式：低 8 位先发送，高 8 位后发送。 */
    uint16_t raw_value = (uint16_t)low_byte | ((uint16_t)high_byte << 8U);

    /* 转为 int16_t 后，最高位会按补码解释为正负号。 */
    return (int16_t)raw_value;
}

static void IMU_DelayMs(uint32_t milliseconds)
{
    uint32_t start_cycle;
    uint32_t cycles_per_millisecond = SystemCoreClock / 1000U;

    /* 开启 Cortex-M4 内核的 DWT 周期计数器，计数频率等于当前 CPU 主频。 */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    /* 每次等待 1ms；无符号减法可正确处理 CYCCNT 自然回绕。 */
    while (milliseconds > 0U)
    {
        start_cycle = DWT->CYCCNT;
        while ((uint32_t)(DWT->CYCCNT - start_cycle) < cycles_per_millisecond)
        {
            __NOP();
        }
        --milliseconds;
    }
}

static void IMU_ResetYawFilterState(void)
{
    s_yaw_filter_initialized = false;
    s_yaw_stationary_locked = false;
    s_yaw_stationary_count = 0U;
    s_yaw_correction_deg = 0.0f;
    s_yaw_locked_deg = 0.0f;
}

static Project_Status_t IMU_WriteRegister(uint8_t register_address, uint16_t value)
{
    uint8_t command[IMU_JY61P_COMMAND_LENGTH];

    /* WIT 标准串口写寄存器帧头。 */
    command[0] = 0xFFU;
    command[1] = 0xAAU;

    /* 寄存器地址以及 16 位参数的小端字节序。 */
    command[2] = register_address;
    command[3] = (uint8_t)(value & 0x00FFU);
    command[4] = (uint8_t)((value >> 8U) & 0x00FFU);

    return BSP_UART_Write(BSP_UART_PORT_IMU, command, sizeof(command));
}

static Project_Status_t IMU_WriteProtectedRegister(uint8_t register_address, uint16_t value)
{
    Project_Status_t status;

    /* 每个受保护寄存器都按“解锁、等待、写入、等待”的相同顺序配置。 */
    status = IMU_WriteRegister(IMU_JY61P_REGISTER_KEY, IMU_JY61P_KEY_UNLOCK);
    if (status != PROJECT_STATUS_OK)
    {
        return status;
    }
    IMU_DelayMs(IMU_JY61P_COMMAND_DELAY_MS);

    status = IMU_WriteRegister(register_address, value);
    if (status != PROJECT_STATUS_OK)
    {
        return status;
    }
    IMU_DelayMs(IMU_JY61P_COMMAND_DELAY_MS);
    return PROJECT_STATUS_OK;
}

static Project_Status_t IMU_ConfigureOnPowerUp(void)
{
    Project_Status_t status;

    /* 开启陀螺仪自动校准；FF AA 63 00 00 不会执行 Yaw 归零。 */
    status = IMU_WriteProtectedRegister(IMU_JY61P_REGISTER_GYRO_AUTO_CALIBRATION,
                                        IMU_JY61P_GYRO_AUTO_CALIBRATION_ENABLE);
    if (status != PROJECT_STATUS_OK)
    {
        return status;
    }

    /* 设置 ACCFILT=300；实际命令为 FF AA 2A 2C 01。 */
    status =
        IMU_WriteProtectedRegister(IMU_JY61P_REGISTER_ACCEL_FILTER, IMU_JY61P_ACCEL_FILTER_VALUE);
    if (status != PROJECT_STATUS_OK)
    {
        return status;
    }

    /*
     * 打开 0x51 加速度、0x52 角速度和 0x53 姿态角输出。
     * 不发送保存命令：每次 MCU 上电都会重新设置，避免反复写 JY61P 内部 Flash。
     */
    return IMU_WriteProtectedRegister(IMU_JY61P_REGISTER_OUTPUT_CONTENT,
                                      IMU_JY61P_OUTPUT_ACCEL_GYRO_ANGLE);
}

static float IMU_WrapAngleDeg(float angle_deg)
{
    /* 正常修正量只会靠近边界，使用循环仍可防止异常输入超出一个周期。 */
    while (angle_deg > 180.0f)
    {
        angle_deg -= 360.0f;
    }
    while (angle_deg < -180.0f)
    {
        angle_deg += 360.0f;
    }
    return angle_deg;
}

static bool IMU_IsWithinThreshold(float value, float threshold)
{
    return (value >= -threshold) && (value <= threshold);
}

static void IMU_ApplyYawStationaryLock(IMU_Sample_t *sample)
{
    float acceleration_squared;
    float corrected_yaw_deg;
    bool acceleration_is_static;
    bool gyroscope_is_static;

    if (sample == NULL)
    {
        return;
    }

    /* 不开平方即可判断合加速度是否落在 0.90g~1.10g 的静止窗口内。 */
    acceleration_squared = (sample->acceleration_x_g * sample->acceleration_x_g) +
                           (sample->acceleration_y_g * sample->acceleration_y_g) +
                           (sample->acceleration_z_g * sample->acceleration_z_g);
    acceleration_is_static = (acceleration_squared >= IMU_YAW_STATIONARY_ACCEL_MIN_SQUARED_G) &&
                             (acceleration_squared <= IMU_YAW_STATIONARY_ACCEL_MAX_SQUARED_G);

    /* 三轴角速度都必须较小，避免倾斜、振动或转动过程中错误锁住 Yaw。 */
    gyroscope_is_static = IMU_IsWithinThreshold(sample->angular_velocity_x_dps,
                                                IMU_YAW_STATIONARY_GYRO_THRESHOLD_DPS) &&
                          IMU_IsWithinThreshold(sample->angular_velocity_y_dps,
                                                IMU_YAW_STATIONARY_GYRO_THRESHOLD_DPS) &&
                          IMU_IsWithinThreshold(sample->angular_velocity_z_dps,
                                                IMU_YAW_STATIONARY_GYRO_THRESHOLD_DPS);

    if (!s_yaw_filter_initialized)
    {
        /* 首组数据直接作为连续角度起点，不执行上电归零。 */
        s_yaw_filter_initialized = true;
        s_yaw_locked_deg = sample->yaw_deg;
        s_yaw_correction_deg = 0.0f;
    }

    if (acceleration_is_static && gyroscope_is_static)
    {
        if (s_yaw_stationary_count < IMU_YAW_STATIONARY_CONFIRM_SAMPLES)
        {
            ++s_yaw_stationary_count;
        }

        if (s_yaw_stationary_count >= IMU_YAW_STATIONARY_CONFIRM_SAMPLES)
        {
            if (!s_yaw_stationary_locked)
            {
                /* 第一次确认静止时记录当前连续角度，以后保持该值。 */
                s_yaw_stationary_locked = true;
                s_yaw_locked_deg = IMU_WrapAngleDeg(sample->yaw_deg + s_yaw_correction_deg);
            }

            /*
             * 原始 Yaw 在静止期间每漂移一点，就反向更新同样大小的修正量。
             * 运动恢复后继续带着该修正量输出，因此不会在解锁瞬间发生跳变。
             */
            s_yaw_correction_deg = IMU_WrapAngleDeg(s_yaw_locked_deg - sample->yaw_deg);
            sample->yaw_deg = s_yaw_locked_deg;
            return;
        }
    }
    else
    {
        /* 任一运动条件出现便立即解锁；下一次静止必须重新连续确认。 */
        s_yaw_stationary_count = 0U;
        s_yaw_stationary_locked = false;
    }

    corrected_yaw_deg = IMU_WrapAngleDeg(sample->yaw_deg + s_yaw_correction_deg);
    sample->yaw_deg = corrected_yaw_deg;
    s_yaw_locked_deg = corrected_yaw_deg;
}

static void IMU_DecodeFrame(const uint8_t *frame)
{
    int16_t raw_x;
    int16_t raw_y;
    int16_t raw_z;

    /* 三类帧的数据区都由三个低字节在前的有符号 16 位数开始。 */
    raw_x = IMU_DecodeInt16(frame[2], frame[3]);
    raw_y = IMU_DecodeInt16(frame[4], frame[5]);
    raw_z = IMU_DecodeInt16(frame[6], frame[7]);

    switch (frame[1])
    {
    case IMU_JY61P_FRAME_ACCELERATION:
        /* 0x51：三轴加速度量程为 ±16g，字节 8~9 为温度原始值。 */
        s_latest_sample.acceleration_x_g =
            ((float)raw_x / IMU_JY61P_RAW_SCALE) * IMU_JY61P_ACCELERATION_RANGE_G;
        s_latest_sample.acceleration_y_g =
            ((float)raw_y / IMU_JY61P_RAW_SCALE) * IMU_JY61P_ACCELERATION_RANGE_G;
        s_latest_sample.acceleration_z_g =
            ((float)raw_z / IMU_JY61P_RAW_SCALE) * IMU_JY61P_ACCELERATION_RANGE_G;
        s_latest_sample.temperature_c =
            (float)IMU_DecodeInt16(frame[8], frame[9]) / IMU_JY61P_TEMPERATURE_SCALE;
        s_update_mask |= IMU_JY61P_UPDATE_ACCELERATION;
        break;

    case IMU_JY61P_FRAME_ANGULAR_VELOCITY:
        /* 0x52：三轴角速度量程为 ±2000 degree/second。 */
        s_latest_sample.angular_velocity_x_dps =
            ((float)raw_x / IMU_JY61P_RAW_SCALE) * IMU_JY61P_ANGULAR_VELOCITY_RANGE_DPS;
        s_latest_sample.angular_velocity_y_dps =
            ((float)raw_y / IMU_JY61P_RAW_SCALE) * IMU_JY61P_ANGULAR_VELOCITY_RANGE_DPS;
        s_latest_sample.angular_velocity_z_dps =
            ((float)raw_z / IMU_JY61P_RAW_SCALE) * IMU_JY61P_ANGULAR_VELOCITY_RANGE_DPS;
        s_update_mask |= IMU_JY61P_UPDATE_ANGULAR_VELOCITY;
        break;

    case IMU_JY61P_FRAME_ANGLE:
        /* 0x53：数据顺序为 Roll、Pitch、Yaw，量程为 ±180 degree。 */
        s_latest_sample.roll_deg = ((float)raw_x / IMU_JY61P_RAW_SCALE) * IMU_JY61P_ANGLE_RANGE_DEG;
        s_latest_sample.pitch_deg =
            ((float)raw_y / IMU_JY61P_RAW_SCALE) * IMU_JY61P_ANGLE_RANGE_DEG;
        s_latest_sample.yaw_deg = ((float)raw_z / IMU_JY61P_RAW_SCALE) * IMU_JY61P_ANGLE_RANGE_DEG;
        s_latest_sample.version = (uint16_t)frame[8] | ((uint16_t)frame[9] << 8U);
        s_update_mask |= IMU_JY61P_UPDATE_ANGLE;
        break;

    default:
        /* 时间、磁场、端口等其他帧当前不属于 JY61P 六轴运动数据。 */
        return;
    }

    /* 只有三类帧都至少更新一次，才发布一组字段齐全的文本样本。 */
    if ((s_update_mask & IMU_JY61P_UPDATE_ALL) == IMU_JY61P_UPDATE_ALL)
    {
        s_update_mask = 0U;
        /* 发布前执行静止 Yaw 漂移修正，其他传感器原始物理量保持不变。 */
        IMU_ApplyYawStationaryLock(&s_latest_sample);
        /* 发布时复制快照，避免下一帧到来后改动应用层尚未读取的整组数据。 */
        s_published_sample = s_latest_sample;
        s_has_sample = true;
        s_sample_pending = true;
        ++s_sample_count;
    }
}

static void IMU_ParseByte(uint8_t data)
{
    uint8_t checksum = 0U;
    uint8_t index;

    /* 一帧的第一个字节必须是固定帧头 0x55，其他字节全部丢弃。 */
    if (s_frame_index == 0U)
    {
        if (data == IMU_JY61P_FRAME_HEADER)
        {
            s_frame[0] = data;
            s_frame_index = 1U;
        }
        return;
    }

    /* 已找到帧头，继续顺序保存后续字节。 */
    s_frame[s_frame_index] = data;
    ++s_frame_index;

    /* 还没有收满固定的 11 字节，等待下一次调用继续接收。 */
    if (s_frame_index < IMU_JY61P_FRAME_LENGTH)
    {
        return;
    }

    /* 校验和是前 10 个字节相加后保留最低 8 位。 */
    for (index = 0U; index < (IMU_JY61P_FRAME_LENGTH - 1U); ++index)
    {
        checksum = (uint8_t)(checksum + s_frame[index]);
    }

    /* 第 11 个字节是传感器给出的校验值。 */
    if (checksum == s_frame[IMU_JY61P_FRAME_LENGTH - 1U])
    {
        ++s_valid_frame_count;
        IMU_DecodeFrame(s_frame);
        /* 当前帧处理结束，下一字节重新按帧头处理。 */
        s_frame_index = 0U;
    }
    else
    {
        ++s_checksum_error_count;
        /* 线路干扰或中途丢字节时，尝试从当前数据中找回下一帧帧头。 */
        IMU_ResynchronizeFrame();
    }
}

static void IMU_ResynchronizeFrame(void)
{
    uint8_t header_index;
    uint8_t remaining_length;

    /* 跳过旧帧的第一个 0x55，在剩余 10 字节中寻找可能的新帧头。 */
    for (header_index = 1U; header_index < IMU_JY61P_FRAME_LENGTH; ++header_index)
    {
        if (s_frame[header_index] == IMU_JY61P_FRAME_HEADER)
        {
            /* 把疑似新帧移动到缓冲区起点，并在后续接收中继续补齐。 */
            remaining_length = (uint8_t)(IMU_JY61P_FRAME_LENGTH - header_index);
            memmove(s_frame, &s_frame[header_index], remaining_length);
            s_frame_index = remaining_length;
            return;
        }
    }

    /* 旧数据中没有候选帧头，只能清空状态并等待下一个 0x55。 */
    s_frame_index = 0U;
}
