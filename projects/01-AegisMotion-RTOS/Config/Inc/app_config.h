/**
 * @file app_config.h
 * @brief 应用功能编译配置。
 */

#ifndef APP_CONFIG_H_
#define APP_CONFIG_H_

/**
 * IMU 功能开关：
 * 0U - 不编译 IMU 功能；
 * 1U - 编译并初始化 IMU 功能。
 */
#ifndef APP_CONFIG_IMU_ENABLED
#define APP_CONFIG_IMU_ENABLED (1U)
#endif

/** JY61P 固定使用的 USART3 波特率。 */
#define APP_CONFIG_IMU_BAUD_RATE (9600UL)

/**
 * 上位机二进制帧控制 Emm_V5 的功能开关：
 * 0U - 不解析上位机控制帧；
 * 1U - 解析带 CRC16 的电机及底盘控制帧。
 */
#ifndef APP_CONFIG_HOST_CONTROL_ENABLED
#define APP_CONFIG_HOST_CONTROL_ENABLED (1U)
#endif

/**
 * 上位机失联保护：
 * 收到可能引起电机运动的命令后开始计时；在该时间内没有收到任何合法上位机帧，
 * 控制器会取消尚未完成的底盘命令并向地址 0 广播立即停止。
 */
#define APP_CONFIG_HOST_LINK_TIMEOUT_MS (500U)
/** 新动作或带原动作 SEQ 的 RENEW 续期，PING 不续期。 */
#define APP_CONFIG_MOTION_LEASE_MS (500U)
/** 四轮事务的总期限，包含TX排队、每轮回复与重试；不是机械运动完成期限。 */
#define APP_CONFIG_MECANUM_TRANSACTION_TIMEOUT_MS (200U)

/** STM32 非阻塞启动/重新自检：四台电机 FLAG 查询、实时完整 IMU 样本。 */
#define APP_CONFIG_SELF_TEST_TIMEOUT_MS (5000U)
#define APP_CONFIG_SELF_TEST_MOTOR_TIMEOUT_MS (500U)
#define APP_CONFIG_SELF_TEST_MOTOR_ATTEMPTS (2U)
/** 完整 IMU 样本超过此时间未更新，锁定运动并广播停车。 */
#define APP_CONFIG_IMU_STALE_TIMEOUT_MS (1000U)
/** 已执行运动命令时，每轮巡检完成后等待此时间再查询四电机在线状态。 */
#define APP_CONFIG_MOTOR_HEALTH_PERIOD_MS (500U)

/**
 * 独立看门狗配置。看门狗只会在所有已启用的应用任务都正常运行时刷新。
 * STM32F4 的 LSI 频率存在偏差，因此实际复位时间会在标称值附近浮动。
 */
#ifndef APP_CONFIG_IWDG_ENABLED
#define APP_CONFIG_IWDG_ENABLED (1U)
#endif
#define APP_CONFIG_IWDG_TIMEOUT_MS (2000U)
#define APP_CONFIG_IWDG_CHECK_PERIOD_MS (250U)

/** 串口协议收到半帧后允许等待后续字节的最长时间。 */
#define APP_CONFIG_PROTOCOL_FRAME_TIMEOUT_MS (50U)

/** 单电机命令和麦克纳姆底盘允许的最大转速。 */
#define APP_CONFIG_MOTOR_MAX_RPM (100U)

/* 麦克纳姆底盘四个电机的总线地址。 */
#define APP_CONFIG_MECANUM_LEFT_FRONT_ADDRESS (2U)
#define APP_CONFIG_MECANUM_RIGHT_FRONT_ADDRESS (1U)
#define APP_CONFIG_MECANUM_LEFT_REAR_ADDRESS (3U)
#define APP_CONFIG_MECANUM_RIGHT_REAR_ADDRESS (4U)

/*
 * 车辆前进时各驱动器使用的 Emm_V5 方向位：0=CW，1=CCW。
 * 左右电机通常镜像安装，因此默认方向相反；首次测试时请架空车轮确认。
 */
#define APP_CONFIG_MECANUM_LEFT_FRONT_FORWARD_DIR (0U)
#define APP_CONFIG_MECANUM_RIGHT_FRONT_FORWARD_DIR (1U)
#define APP_CONFIG_MECANUM_LEFT_REAR_FORWARD_DIR (0U)
#define APP_CONFIG_MECANUM_RIGHT_REAR_FORWARD_DIR (1U)

#if ((APP_CONFIG_IMU_ENABLED != 0U) && (APP_CONFIG_IMU_ENABLED != 1U))
#error "APP_CONFIG_IMU_ENABLED must be 0U or 1U"
#endif

#if (APP_CONFIG_IMU_BAUD_RATE == 0U)
#error "APP_CONFIG_IMU_BAUD_RATE must be nonzero"
#endif

#if ((APP_CONFIG_HOST_CONTROL_ENABLED != 0U) && (APP_CONFIG_HOST_CONTROL_ENABLED != 1U))
#error "APP_CONFIG_HOST_CONTROL_ENABLED must be 0U or 1U"
#endif

#if ((APP_CONFIG_IWDG_ENABLED != 0U) && (APP_CONFIG_IWDG_ENABLED != 1U))
#error "APP_CONFIG_IWDG_ENABLED must be 0U or 1U"
#endif

#if ((APP_CONFIG_HOST_CONTROL_ENABLED == 1U) && (APP_CONFIG_HOST_LINK_TIMEOUT_MS < 100U))
#error "APP_CONFIG_HOST_LINK_TIMEOUT_MS must be at least 100 ms"
#endif

#if ((APP_CONFIG_IWDG_TIMEOUT_MS < 100U) || (APP_CONFIG_IWDG_TIMEOUT_MS > 32000U))
#error "APP_CONFIG_IWDG_TIMEOUT_MS must be in range 100..32000 ms"
#endif

#if ((APP_CONFIG_IWDG_CHECK_PERIOD_MS == 0U) ||                                             \
     (APP_CONFIG_IWDG_CHECK_PERIOD_MS * 2U >= APP_CONFIG_IWDG_TIMEOUT_MS))
#error "APP_CONFIG_IWDG_CHECK_PERIOD_MS must be nonzero and less than half the timeout"
#endif

#if (APP_CONFIG_PROTOCOL_FRAME_TIMEOUT_MS < 10U)
#error "APP_CONFIG_PROTOCOL_FRAME_TIMEOUT_MS must be at least 10 ms"
#endif

#if ((APP_CONFIG_MOTOR_MAX_RPM == 0U) || (APP_CONFIG_MOTOR_MAX_RPM > 5000U))
#error "APP_CONFIG_MOTOR_MAX_RPM must be in range 1..5000"
#endif

#if ((APP_CONFIG_MECANUM_LEFT_FRONT_ADDRESS == 0U) ||                                              \
     (APP_CONFIG_MECANUM_RIGHT_FRONT_ADDRESS == 0U) ||                                             \
     (APP_CONFIG_MECANUM_LEFT_REAR_ADDRESS == 0U) ||                                               \
     (APP_CONFIG_MECANUM_RIGHT_REAR_ADDRESS == 0U))
#error "Mecanum motor address 0 is reserved for broadcast"
#endif

#if ((APP_CONFIG_MECANUM_LEFT_FRONT_ADDRESS == APP_CONFIG_MECANUM_RIGHT_FRONT_ADDRESS) ||          \
     (APP_CONFIG_MECANUM_LEFT_FRONT_ADDRESS == APP_CONFIG_MECANUM_LEFT_REAR_ADDRESS) ||            \
     (APP_CONFIG_MECANUM_LEFT_FRONT_ADDRESS == APP_CONFIG_MECANUM_RIGHT_REAR_ADDRESS) ||           \
     (APP_CONFIG_MECANUM_RIGHT_FRONT_ADDRESS == APP_CONFIG_MECANUM_LEFT_REAR_ADDRESS) ||           \
     (APP_CONFIG_MECANUM_RIGHT_FRONT_ADDRESS == APP_CONFIG_MECANUM_RIGHT_REAR_ADDRESS) ||          \
     (APP_CONFIG_MECANUM_LEFT_REAR_ADDRESS == APP_CONFIG_MECANUM_RIGHT_REAR_ADDRESS))
#error "Mecanum motor addresses must be unique"
#endif

#if ((APP_CONFIG_MECANUM_LEFT_FRONT_FORWARD_DIR > 1U) ||                                           \
     (APP_CONFIG_MECANUM_RIGHT_FRONT_FORWARD_DIR > 1U) ||                                          \
     (APP_CONFIG_MECANUM_LEFT_REAR_FORWARD_DIR > 1U) ||                                            \
     (APP_CONFIG_MECANUM_RIGHT_REAR_FORWARD_DIR > 1U))
#error "Mecanum forward direction must be 0U or 1U"
#endif

#endif /* APP_CONFIG_H_ */
