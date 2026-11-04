/**
 * @file imu.h
 * @brief IMU 驱动公共接口。
 */

#ifndef IMU_H_
#define IMU_H_

#include <stdbool.h>
#include <stdint.h>

#include "project_status.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /** 由 JY61P 的 0x51、0x52、0x53 三类帧组成的一组完整运动数据。 */
    typedef struct
    {
        float acceleration_x_g;       /**< X 轴加速度，单位 g。 */
        float acceleration_y_g;       /**< Y 轴加速度，单位 g。 */
        float acceleration_z_g;       /**< Z 轴加速度，单位 g。 */
        float angular_velocity_x_dps; /**< X 轴角速度，单位 degree/second。 */
        float angular_velocity_y_dps; /**< Y 轴角速度，单位 degree/second。 */
        float angular_velocity_z_dps; /**< Z 轴角速度，单位 degree/second。 */
        float yaw_deg;                /**< 绕 Z 轴的航向角。 */
        float pitch_deg;              /**< 绕 Y 轴的俯仰角。 */
        float roll_deg;               /**< 绕 X 轴的横滚角。 */
        float temperature_c;          /**< 0x51 帧中的模块温度，单位摄氏度。 */
        uint16_t version;             /**< 0x53 帧中的固件版本原始值。 */
    } IMU_Sample_t;

    /**
     * @brief 初始化 IMU，设置动态滤波，并打开加速度、角速度和姿态角输出。
     * @note 调用前 BSP_UART_PORT_IMU 必须按 APP_CONFIG_IMU_BAUD_RATE 初始化为 8N1。
     * @note 本函数不发送 Yaw 归零、保存或波特率修改命令。
     * @return 初始化成功返回 PROJECT_STATUS_OK。
     */
    Project_Status_t IMU_Init(void);

    /**
     * @brief 从 USART3 接收缓冲区取出数据并解析 JY61P 帧。
     * @return 处理成功返回 PROJECT_STATUS_OK。
     */
    Project_Status_t IMU_Process(void);

    /**
     * @brief 读取最新一组完整 IMU 数据。
     * @param[out] sample 数据输出地址，不允许为 NULL。
     * @return 已获得过完整数据则返回 PROJECT_STATUS_OK。
     */
    Project_Status_t IMU_Read(IMU_Sample_t *sample);

    /**
     * @brief 尝试取出一组尚未读取的完整 IMU 数据。
     * @param[out] sample 数据输出地址，不允许为 NULL。
     * @return 获得新数据返回 true；否则返回 false。
     */
    bool IMU_TryRead(IMU_Sample_t *sample);

    /**
     * @brief 获取累计通过校验的 JY61P 数据帧数。
     * @return 有效帧累计数。
     */
    uint32_t IMU_GetValidFrameCount(void);

    /** 完整的加速度/角速度/角度样本累计数，用于跨任务的新鲜度检查。 */
    uint32_t IMU_GetSampleCount(void);

    /**
     * @brief 获取累计校验和错误数。
     * @return 校验和错误累计数。
     */
    uint32_t IMU_GetChecksumErrorCount(void);

    /**
     * @brief 获取当前配置的 JY61P 串口波特率。
     * @return app_config.h 中配置的固定波特率。
     */
    uint32_t IMU_GetBaudRate(void);

    /**
     * @brief 查询 STM32 的静止 Yaw 漂移锁定是否已经生效。
     * @return 连续确认静止后返回 true，检测到运动时返回 false。
     */
    bool IMU_IsYawStationaryLocked(void);

#ifdef __cplusplus
}
#endif

#endif /* IMU_H_ */
