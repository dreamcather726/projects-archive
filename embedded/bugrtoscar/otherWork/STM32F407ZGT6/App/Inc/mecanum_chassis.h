/**
 * @file mecanum_chassis.h
 * @brief X 型麦克纳姆轮底盘速度分解与四电机同步控制。
 */

#ifndef MECANUM_CHASSIS_H_
#define MECANUM_CHASSIS_H_

#include <stdbool.h>
#include <stdint.h>

#include "project_status.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /** 四个车轮的有符号目标转速；正数表示车轮向车辆前进方向滚动。 */
    typedef struct
    {
        int16_t left_front_rpm;
        int16_t right_front_rpm;
        int16_t left_rear_rpm;
        int16_t right_rear_rpm;
    } Mecanum_WheelRpm_t;

    enum
    {
        MECANUM_RESULT_IDLE = 0U,
        MECANUM_RESULT_PENDING = 1U,
        MECANUM_RESULT_SYNC_SENT = 2U,
        MECANUM_RESULT_FAILED = 3U,
        MECANUM_RESULT_CANCELLED = 4U
    };

    /** 启动时初始化状态；运行中不能用它绕过故障锁定，应重新自检。 */
    void Mecanum_Init(void);
    /** 故障锁与待停车分别查询：取消软件序列不会清除故障锁。 */
    bool Mecanum_HasFault(void);
    bool Mecanum_StopPending(void);
    void Mecanum_ClearFault(void); /* 仅在停止入队且重新自检通过后调用。 */
    uint32_t Mecanum_GetFaultCount(void);
    uint8_t Mecanum_GetFaultAddress(void);
    /** SYNC_SENT只代表四轮应答+同步触发入队，不代表到位或物理停车。 */
    uint8_t Mecanum_GetResult(void);

    /**
     * @brief 将底盘三自由度指令分解为四轮转速。
     * @param forward 前后分量，-1000~1000，正数为前进。
     * @param left 左右分量，-1000~1000，正数为向左平移。
     * @param counterclockwise 旋转分量，-1000~1000，正数为逆时针。
     * @param maximum_rpm 任意车轮允许的最大转速，0~APP_CONFIG_MOTOR_MAX_RPM。
     * @param wheel_rpm 保存计算结果的地址。
     * @return 参数有效时返回 PROJECT_STATUS_OK。
     * @note 合成量超过 1000 时会整体等比例缩放，保持运动方向不变。
     */
    Project_Status_t Mecanum_CalculateWheelRpm(int16_t forward, int16_t left,
                                               int16_t counterclockwise, uint16_t maximum_rpm,
                                               Mecanum_WheelRpm_t *wheel_rpm);

    /**
     * @brief 计算四轮转速，并按 app_config.h 中配置的地址同步下发速度命令。
     * @param forward 前后分量，-1000~1000，正数为前进。
     * @param left 左右分量，-1000~1000，正数为向左平移。
     * @param counterclockwise 旋转分量，-1000~1000，正数为逆时针。
     * @param maximum_rpm 任意车轮允许的最大转速，0~APP_CONFIG_MOTOR_MAX_RPM。
     * @param acceleration Emm_V5 加速度参数，0~255；0 表示直接启动。
     * @return 接受异步序列返回 OK；已有序列或故障锁定时返回 ERROR。
     */
    Project_Status_t Mecanum_Drive(int16_t forward, int16_t left, int16_t counterclockwise,
                                   uint16_t maximum_rpm, uint8_t acceleration);

    /**
     * @brief 推进一次四电机分步发送状态机。
     * @return 正常等待、发送成功、队列暂满以及已转入故障停车均返回OK。
     * @note 由Control任务独占调用；故障通过HasFault/GetResult查询，不阻塞等待。
     */
    Project_Status_t Mecanum_Process(void);

    /**
     * @brief 把一帧 Emm_V5 回复交给底盘状态机，用于确认后发送下一台电机。
     * @param frame 以 0x6B 结尾的完整回复帧。
     * @param length 回复帧长度。
     */
    void Mecanum_HandleMotorResponse(const uint8_t *frame, uint32_t length);

    /** 仅取消软件序列，不发送停车、不丢弃UART排队帧、不清除故障锁。 */
    void Mecanum_Cancel(void);

    /** 返回当前是否仍在分步发送底盘命令。 */
    bool Mecanum_IsBusy(void);

#ifdef __cplusplus
}
#endif

#endif /* MECANUM_CHASSIS_H_ */
