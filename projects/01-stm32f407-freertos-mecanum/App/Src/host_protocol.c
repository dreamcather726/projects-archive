/**
 * @file host_protocol.c
 * @brief 上位机二进制帧解析、CRC16 校验和 Emm_V5 控制。
 *
 * 帧格式：5A LEN DEST SEQ INDEX DATA CRC_L CRC_H
 * LEN 是整帧字节数；CRC16-Modbus 覆盖 LEN 到 DATA 末尾。
 * Control 任务独占接收/安全/底盘状态，Data 任务只能经 SendEvent 入发送队列。
 * 本文件按“接收与命令 -> 安全状态 -> 发送与编码”组织，不直接操作 USART 寄存器。
 */

#include "host_protocol.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "Emm_V5.h"
#include "FreeRTOS.h"
#include "app_config.h"
#include "bsp_uart.h"
#if (APP_CONFIG_IMU_ENABLED == 1U)
#include "imu.h"
#endif
#include "mecanum_chassis.h"
#include "stm32f407xx.h"
#include "task.h"

#define HOST_FRAME_SYNC (0x5AU)
#define HOST_FRAME_MIN_SIZE (7U)
#define HOST_FRAME_MAX_SIZE (255U)
#define HOST_FRAME_MAX_PAYLOAD_LENGTH (HOST_FRAME_MAX_SIZE - HOST_FRAME_MIN_SIZE)
#define HOST_DESTINATION_HOST (0x00U)
#define HOST_DESTINATION_CONTROLLER (0x01U)
#define HOST_DESTINATION_BROADCAST (0xFFU)
#define HOST_MOTOR_FRAME_BUFFER_SIZE (64U)
#define HOST_MOTOR_FRAME_END (0x6BU)

/* 上位机发送 FIFO 队列深度：可同时排队等待发送的完整帧数。 */
#define HOST_TX_QUEUE_DEPTH (8U)
#define HOST_ACTION_CACHE_DEPTH (8U)
#define HOST_ACTION_CACHE_MS (2000U)

#define HOST_COMMAND_PING (0x01U)
#define HOST_COMMAND_STATUS (0x02U)
#define HOST_COMMAND_SELF_TEST (0x03U)
#define HOST_COMMAND_RENEW (0x04U)
#define HOST_COMMAND_ENABLE (0x10U)
#define HOST_COMMAND_VELOCITY (0x11U)
#define HOST_COMMAND_POSITION (0x12U)
#define HOST_COMMAND_STOP (0x13U)
#define HOST_COMMAND_EMERGENCY_STOP (0x14U)
#define HOST_COMMAND_SYNC (0x15U)
#define HOST_COMMAND_ZERO (0x16U)
#define HOST_COMMAND_READ (0x17U)
#define HOST_COMMAND_HOME (0x18U)
#define HOST_COMMAND_CHASSIS (0x20U)

#define HOST_RESPONSE_INDEX(command) ((uint8_t)((command) | 0x80U))

#define HOST_ACK_OK (0x00U)
#define HOST_ACK_LENGTH_ERROR (0x01U)
#define HOST_ACK_CRC_ERROR (0x02U)
#define HOST_ACK_UNKNOWN_COMMAND (0x03U)
#define HOST_ACK_INVALID_ARGUMENT (0x04U)
#define HOST_ACK_BUSY (0x05U)
#define HOST_ACK_MOTOR_ERROR (0x06U)
#define HOST_ACK_NOT_READY (0x07U)

#define SELF_TEST_RUNNING (1U)
#define SELF_TEST_PASSED (2U)
#define SELF_TEST_FAILED (3U)
#define SAFETY_FAULT_SELF_TEST (1U)
#define SAFETY_FAULT_MOTOR (2U)
#define SAFETY_FAULT_IMU (3U)
#define SAFETY_FAULT_ESTOP (4U)
#define SAFETY_FAULT_LEASE (5U)
#define SAFETY_FAULT_LINK (6U)
#define SAFETY_FAULT_MOTOR_STATUS (7U)
#define CONTROL_IDLE (0U)
#define CONTROL_SINGLE (1U)
#define CONTROL_CHASSIS (2U)

/* 掩码位与数组顺序固定：左前/右前/左后/右后，不能改成地址数值排序。 */
static const uint8_t s_test_addresses[4] = {
    APP_CONFIG_MECANUM_LEFT_FRONT_ADDRESS, APP_CONFIG_MECANUM_RIGHT_FRONT_ADDRESS,
    APP_CONFIG_MECANUM_LEFT_REAR_ADDRESS, APP_CONFIG_MECANUM_RIGHT_REAR_ADDRESS};
/* 自检状态只允许显式 SELF_TEST 从失败重新进入检测；通过不重放历史动作。 */
static uint8_t s_test_state;
static uint8_t s_test_motor_mask;
static uint8_t s_test_index;
static uint8_t s_test_attempts;
static bool s_test_waiting;
static bool s_test_clock_started; /* 自检总期限从Control首次运行开始，不含调度器前的IMU配置延时。 */
static TickType_t s_test_start_tick;
static TickType_t s_test_probe_tick;
static uint8_t s_safety_fault;
static uint8_t s_safety_fault_address;
static bool s_safety_event_pending;
static uint32_t s_observed_motor_faults;
static bool s_chassis_result_pending;
static uint8_t s_chassis_sequence;
/* 运行巡检与启动自检分开计时；续期不能重置巡检进度。 */
static uint8_t s_health_index;
static uint8_t s_health_attempts;
static bool s_health_waiting;
static TickType_t s_health_tick;
static TickType_t s_health_next_tick;
/* 急停锁、控制模式、运动授权分别表达不同约束，不能合成一个 busy 标志。 */
static bool s_estop_latched;
static uint8_t s_control_mode;
static uint8_t s_motion_sequence;
static TickType_t s_last_motion_tick;
static uint8_t s_staged_motor_mask;
static uint8_t s_active_motor_mask;
/* 最近8条、2秒内的动作去重缓存；不是跨复位、永久有效的事务日志。 */
typedef struct
{
    bool valid;
    uint8_t sequence;
    uint8_t command;
    uint8_t length;
    uint8_t payload[11];
    TickType_t tick;
} Host_ActionCache_t;
static Host_ActionCache_t s_action_cache[HOST_ACTION_CACHE_DEPTH];
static uint8_t s_action_cache_next;
#if (APP_CONFIG_IMU_ENABLED == 1U)
static uint32_t s_observed_imu_samples;
static bool s_test_imu_seen;
static TickType_t s_last_imu_tick;
#endif

typedef enum
{
    HOST_RX_WAIT_SYNC = 0,
    HOST_RX_WAIT_LENGTH,
    HOST_RX_READ_REMAINDER
} Host_RxState_t;

/** 发送队列中的一个帧槽：完整帧数据 + 有效长度。 */
typedef struct
{
    uint8_t data[HOST_FRAME_MAX_SIZE];
    uint8_t length;
} Host_TxFrame_t;

static Host_RxState_t s_rx_state;
static uint8_t s_rx_frame[HOST_FRAME_MAX_SIZE];
static uint16_t s_rx_index;
/* 预期总长已经保存在 s_rx_frame[1]，无需再维护一份长度状态。 */

static uint8_t s_motor_frame[HOST_MOTOR_FRAME_BUFFER_SIZE];
static uint32_t s_motor_frame_length;
static TickType_t s_motor_last_byte_tick;
static uint32_t s_motor_protocol_error_count;

/* 链路时间与运动时间独立：PING 可刷新前者，只有新动作/RENEW 可刷新后者。 */
static TickType_t s_last_valid_host_tick;
static bool s_host_failsafe_armed;
static uint32_t s_host_failsafe_trip_count;
static bool s_stop_retry_pending;
static bool s_stop_retry_is_failsafe;
static TickType_t s_host_last_byte_tick;
static uint32_t s_host_protocol_error_count;

/* FIFO 只存 head/count；尾槽由 (head + count) % depth 唯一推导，临界区内使用。 */
static Host_TxFrame_t s_tx_queue[HOST_TX_QUEUE_DEPTH];
static uint8_t s_tx_queue_head;
static uint8_t s_tx_queue_count;
static uint8_t s_event_sequence;

static uint16_t HostProtocol_Crc16(const uint8_t *data, uint16_t length);
static uint16_t HostProtocol_ReadU16(const uint8_t *data);
static uint32_t HostProtocol_ReadU32(const uint8_t *data);
static int16_t HostProtocol_ReadI16(const uint8_t *data);
static void HostProtocol_WriteU32(uint8_t *data, uint32_t value);
static void HostProtocol_ResetReceiver(uint8_t latest_byte);
static Project_Status_t HostProtocol_QueueFrame(uint8_t sequence, uint8_t command,
                                                const uint8_t *data, uint8_t length,
                                                bool replace_same_command);
static Project_Status_t HostProtocol_SendAck(uint8_t sequence, uint8_t request_command,
                                              uint8_t status);
static Project_Status_t HostProtocol_FlushPendingOutput(void);
static Project_Status_t HostProtocol_ProcessHostInput(void);
static Project_Status_t HostProtocol_ProcessMotorInput(void);
static Project_Status_t HostProtocol_ProcessFailsafe(void);
static Project_Status_t HostProtocol_ExecuteFrame(const uint8_t *frame);
static Project_Status_t HostProtocol_SendStatus(uint8_t sequence);
static bool HostProtocol_ParseReadItem(uint8_t value, SysParams_t *parameter);
static uint8_t HostProtocol_MapMotorStatus(Project_Status_t status);
static void HostProtocol_StartSelfTest(void);
static void HostProtocol_ProcessSafety(void);
static void HostProtocol_HandleSelfTestMotor(const uint8_t *frame, uint32_t length);
static uint8_t HostProtocol_WriteSafety(uint8_t *data);
static void HostProtocol_SafetyStop(uint8_t reason);
static uint8_t HostProtocol_AddressMask(uint8_t address);
static void HostProtocol_AuthorizeMotion(uint8_t sequence);
static void HostProtocol_EndMotion(void);
static uint32_t HostProtocol_MotorFrameSize(void);
static Project_Status_t HostProtocol_StopAll(void);
static void HostProtocol_CheckMotionExpiry(void);

Project_Status_t HostProtocol_Init(void)
{
    memset(s_rx_frame, 0, sizeof(s_rx_frame));
    memset(s_motor_frame, 0, sizeof(s_motor_frame));
    memset(s_tx_queue, 0, sizeof(s_tx_queue));
    memset(s_action_cache, 0, sizeof(s_action_cache));
    s_action_cache_next = 0U;
    s_rx_state = HOST_RX_WAIT_SYNC;
    s_rx_index = 0U;
    s_motor_frame_length = 0U;
    s_motor_last_byte_tick = 0U;
    s_motor_protocol_error_count = 0U;
    s_last_valid_host_tick = 0U;
    s_host_failsafe_armed = false;
    s_host_failsafe_trip_count = 0U;
    s_stop_retry_pending = false;
    s_stop_retry_is_failsafe = false;
    s_host_last_byte_tick = 0U;
    s_host_protocol_error_count = 0U;
    s_tx_queue_head = 0U;
    s_tx_queue_count = 0U;
    s_event_sequence = 0U;
    Mecanum_Init();
    s_observed_motor_faults = 0U;
    s_chassis_result_pending = false;
    s_estop_latched = false;
    s_control_mode = CONTROL_IDLE;
    s_active_motor_mask = 0U;
    s_staged_motor_mask = 0U;
    HostProtocol_StartSelfTest();
    return PROJECT_STATUS_OK;
}

Project_Status_t HostProtocol_Process(void)
{
    /* 顺序不可交换：旧回复发送 -> 电机回复 -> 故障检查 -> 新请求 -> 停车重试。 */
    Project_Status_t status = HostProtocol_FlushPendingOutput();

    if (status != PROJECT_STATUS_OK)
    {
        return status;
    }

    status = HostProtocol_ProcessMotorInput();
    if (status != PROJECT_STATUS_OK)
    {
        return status;
    }

    /* 先处理故障/自检，再处理新运动请求，不能用新请求覆盖故障。 */
    HostProtocol_ProcessSafety();

    status = HostProtocol_ProcessHostInput();
    if (status != PROJECT_STATUS_OK)
    {
        return status;
    }

    return HostProtocol_ProcessFailsafe();
}

Project_Status_t HostProtocol_SendEvent(uint8_t command, const uint8_t *data, uint8_t length)
{
    uint32_t previous_primask = __get_PRIMASK();
    uint8_t sequence;
    __disable_irq();
    sequence = ++s_event_sequence;
    __set_PRIMASK(previous_primask);

    /*
     * IMU 文本是高频快照类数据，队列中只保留最新一帧：
     * 入队前先替换尚未发出的旧文本帧，避免高频文本占满队列而挤掉应答帧。
     */
    return HostProtocol_QueueFrame(sequence, command, data, length,
                                   command == HOST_PROTOCOL_EVENT_IMU_TEXT);
}

static Project_Status_t HostProtocol_ProcessHostInput(void)
{
    uint8_t byte;
    TickType_t now = xTaskGetTickCount();

    if ((s_rx_state != HOST_RX_WAIT_SYNC) &&
        ((TickType_t)(now - s_host_last_byte_tick) >=
         pdMS_TO_TICKS(APP_CONFIG_PROTOCOL_FRAME_TIMEOUT_MS)))
    {
        HostProtocol_ResetReceiver(0U);
        ++s_host_protocol_error_count;
    }

    while (BSP_UART_TryReadByte(BSP_UART_PORT_HOST, &byte))
    {
        now = xTaskGetTickCount();
        s_host_last_byte_tick = now;

        switch (s_rx_state)
        {
        case HOST_RX_WAIT_SYNC:
            if (byte == HOST_FRAME_SYNC)
            {
                s_rx_frame[0] = byte;
                s_rx_state = HOST_RX_WAIT_LENGTH;
            }
            break;

        case HOST_RX_WAIT_LENGTH:
            if (byte < HOST_FRAME_MIN_SIZE)
            {
                ++s_host_protocol_error_count;
                HostProtocol_ResetReceiver(0U);
                break;
            }
            s_rx_frame[1] = byte;
            s_rx_index = 2U;
            s_rx_state = HOST_RX_READ_REMAINDER;
            break;

        case HOST_RX_READ_REMAINDER:
            s_rx_frame[s_rx_index++] = byte;
            if (s_rx_index >= s_rx_frame[1])
            {
                uint16_t crc_index = (uint16_t)s_rx_frame[1] - 2U;
                uint16_t received_crc = (uint16_t)s_rx_frame[crc_index] |
                                        ((uint16_t)s_rx_frame[crc_index + 1U] << 8U);
                uint16_t calculated_crc =
                    HostProtocol_Crc16(&s_rx_frame[1], (uint16_t)s_rx_frame[1] - 3U);
                bool destination_valid =
                    (s_rx_frame[2] == HOST_DESTINATION_CONTROLLER) ||
                    (s_rx_frame[2] == HOST_DESTINATION_BROADCAST);

                HostProtocol_ResetReceiver(0U);
                if (!destination_valid)
                {
                    break;
                }
                if (received_crc != calculated_crc)
                {
                    ++s_host_protocol_error_count;
                    (void)HostProtocol_SendAck(s_rx_frame[3], s_rx_frame[4],
                                                HOST_ACK_CRC_ERROR);
                    break;
                }
                /* Check the OLD link/lease before a newly received frame refreshes it. */
                HostProtocol_CheckMotionExpiry();
                s_last_valid_host_tick = now;
                (void)HostProtocol_ExecuteFrame(s_rx_frame);
                break;
            }
            break;

        default:
            HostProtocol_ResetReceiver(byte);
            break;
        }
    }

    return PROJECT_STATUS_OK;
}

static Project_Status_t HostProtocol_ExecuteFrame(const uint8_t *frame)
{
    /* 只由 CRC/DEST 校验后的接收路径调用；下列 ACK=00 仅表示接受/入队。 */
    uint8_t sequence = frame[3];
    uint8_t command = frame[4];
    uint8_t payload_length = (uint8_t)(frame[1] - HOST_FRAME_MIN_SIZE);
    const uint8_t *payload = &frame[5];
    Project_Status_t status;
    SysParams_t parameter;

    if (command == HOST_COMMAND_PING)
    {
        if (payload_length != 0U)
        {
            return HostProtocol_SendAck(sequence, command, HOST_ACK_LENGTH_ERROR);
        }
        return HostProtocol_SendAck(sequence, command, HOST_ACK_OK);
    }
    if (command == HOST_COMMAND_STATUS)
    {
        if (payload_length != 0U)
        {
            return HostProtocol_SendAck(sequence, command, HOST_ACK_LENGTH_ERROR);
        }
        return HostProtocol_SendStatus(sequence);
    }
    if (command == HOST_COMMAND_SELF_TEST)
    {
        if (payload_length != 0U)
        {
            return HostProtocol_SendAck(sequence, command, HOST_ACK_LENGTH_ERROR);
        }
        if (s_test_state == SELF_TEST_RUNNING)
        {
            return HostProtocol_SendAck(sequence, command, HOST_ACK_BUSY);
        }
        /* 重新自检先停止，不会重新执行旧动作。故障只在通过后清除。 */
        HostProtocol_StartSelfTest();
        return HostProtocol_SendAck(sequence, command, HOST_ACK_OK);
    }
    if (command == HOST_COMMAND_RENEW)
    {
        TickType_t renew_now = xTaskGetTickCount();
        if (payload_length != 1U)
        {
            return HostProtocol_SendAck(sequence, command, HOST_ACK_LENGTH_ERROR);
        }
        if ((s_test_state != SELF_TEST_PASSED) || s_estop_latched ||
            !s_host_failsafe_armed || s_stop_retry_pending || Mecanum_HasFault())
        {
            return HostProtocol_SendAck(sequence, command, HOST_ACK_NOT_READY);
        }
        if (payload[0] != s_motion_sequence)
        {
            return HostProtocol_SendAck(sequence, command, HOST_ACK_INVALID_ARGUMENT);
        }
        if (((TickType_t)(renew_now - s_last_motion_tick) >= pdMS_TO_TICKS(APP_CONFIG_MOTION_LEASE_MS)) ||
            ((TickType_t)(renew_now - s_last_valid_host_tick) >= pdMS_TO_TICKS(APP_CONFIG_HOST_LINK_TIMEOUT_MS)))
        {
            HostProtocol_SafetyStop(SAFETY_FAULT_LEASE);
            return HostProtocol_SendAck(sequence, command, HOST_ACK_NOT_READY);
        }
        s_last_motion_tick = renew_now;
        return HostProtocol_SendAck(sequence, command, HOST_ACK_OK);
    }
    /* A zero chassis request is a safe, non-latching ALL STOP, including while busy. */
    if ((command == HOST_COMMAND_CHASSIS) && (payload_length == 9U) &&
        (HostProtocol_ReadU16(&payload[6]) <= APP_CONFIG_MOTOR_MAX_RPM) &&
        (((HostProtocol_ReadI16(&payload[0]) == 0) &&
          (HostProtocol_ReadI16(&payload[2]) == 0) &&
          (HostProtocol_ReadI16(&payload[4]) == 0)) ||
         ((HostProtocol_ReadU16(&payload[6]) == 0U) &&
          (HostProtocol_ReadI16(&payload[0]) >= -1000) && (HostProtocol_ReadI16(&payload[0]) <= 1000) &&
          (HostProtocol_ReadI16(&payload[2]) >= -1000) && (HostProtocol_ReadI16(&payload[2]) <= 1000) &&
          (HostProtocol_ReadI16(&payload[4]) >= -1000) && (HostProtocol_ReadI16(&payload[4]) <= 1000))))
    {
        return HostProtocol_SendAck(sequence, command,
                                   HostProtocol_MapMotorStatus(HostProtocol_StopAll()));
    }
    if ((command == HOST_COMMAND_CHASSIS) || (command == HOST_COMMAND_ENABLE) ||
        (command == HOST_COMMAND_VELOCITY) || (command == HOST_COMMAND_POSITION) ||
        (command == HOST_COMMAND_SYNC) || (command == HOST_COMMAND_HOME))
    {
        /* 禁止故障期间使能/运动；停止、失能、查询始终保留。 */
        bool disable_request = (command == HOST_COMMAND_ENABLE) &&
                               (payload_length == 3U) && (payload[1] == 0U);
        if (!disable_request && ((s_test_state != SELF_TEST_PASSED) || s_estop_latched ||
            Mecanum_HasFault() || Mecanum_StopPending() || s_stop_retry_pending))
        {
            return HostProtocol_SendAck(sequence, command, HOST_ACK_NOT_READY);
        }
        if ((command == HOST_COMMAND_POSITION) || (command == HOST_COMMAND_HOME) ||
            (command == HOST_COMMAND_SYNC))
        {
            uint8_t index;
            TickType_t cache_now = xTaskGetTickCount();
            for (index = 0U; index < HOST_ACTION_CACHE_DEPTH; ++index)
            {
                const Host_ActionCache_t *entry = &s_action_cache[index];
                if (entry->valid && (entry->sequence == sequence) && (entry->command == command) &&
                    ((TickType_t)(cache_now - entry->tick) < pdMS_TO_TICKS(HOST_ACTION_CACHE_MS)))
                {
                    bool same = (entry->length == payload_length) &&
                                (memcmp(entry->payload, payload, payload_length) == 0);
                    /* Re-ACK only: never resend an action or renew its motion lease. */
                    return HostProtocol_SendAck(sequence, command,
                                                same ? HOST_ACK_OK : HOST_ACK_INVALID_ARGUMENT);
                }
            }
        }
        if (!disable_request && s_health_waiting)
        {
            return HostProtocol_SendAck(sequence, command, HOST_ACK_BUSY);
        }
        if ((command != HOST_COMMAND_CHASSIS) && !disable_request &&
            (Mecanum_IsBusy() || s_chassis_result_pending || s_control_mode == CONTROL_CHASSIS))
        {
            return HostProtocol_SendAck(sequence, command, HOST_ACK_BUSY);
        }
        if ((command == HOST_COMMAND_CHASSIS) && s_control_mode == CONTROL_SINGLE)
        {
            return HostProtocol_SendAck(sequence, command, HOST_ACK_BUSY);
        }
    }
    if (command == HOST_COMMAND_CHASSIS)
    {
        if (payload_length != 9U)
        {
            return HostProtocol_SendAck(sequence, command, HOST_ACK_LENGTH_ERROR);
        }
        if (s_chassis_result_pending)
        {
            return HostProtocol_SendAck(sequence, command, HOST_ACK_BUSY);
        }
        status = Mecanum_Drive(HostProtocol_ReadI16(&payload[0]),
                               HostProtocol_ReadI16(&payload[2]),
                               HostProtocol_ReadI16(&payload[4]),
                               HostProtocol_ReadU16(&payload[6]), payload[8]);
        if (status == PROJECT_STATUS_OK)
        {
            HostProtocol_AuthorizeMotion(sequence);
            s_control_mode = CONTROL_CHASSIS;
            s_active_motor_mask = 0x0FU;
            s_chassis_result_pending = true;
            s_chassis_sequence = sequence;
        }
        return HostProtocol_SendAck(sequence, command, HostProtocol_MapMotorStatus(status));
    }

    switch (command)
    {
    case HOST_COMMAND_ENABLE:
        if ((payload_length != 3U) || (payload[1] > 1U) || (payload[2] != 0U) ||
            !HostProtocol_AddressMask(payload[0]))
        {
            return HostProtocol_SendAck(sequence, command, HOST_ACK_INVALID_ARGUMENT);
        }
        if (payload[1] == 0U)
        {
            /* Disabling one wheel during chassis control must first stop the entire car. */
            status = HostProtocol_StopAll();
            if (status != PROJECT_STATUS_OK)
            {
                return HostProtocol_SendAck(sequence, command, HostProtocol_MapMotorStatus(status));
            }
        }
        status = Emm_V5_En_Control(payload[0], payload[1] != 0U, payload[2] != 0U);
        break;

    case HOST_COMMAND_VELOCITY:
        if ((payload_length != 6U) || (payload[1] > 1U) ||
            (HostProtocol_ReadU16(&payload[2]) > APP_CONFIG_MOTOR_MAX_RPM) ||
            (payload[5] > 1U) || !HostProtocol_AddressMask(payload[0]))
        {
            return HostProtocol_SendAck(sequence, command, HOST_ACK_INVALID_ARGUMENT);
        }
        Mecanum_Cancel();
        status = Emm_V5_Vel_Control(payload[0], payload[1], HostProtocol_ReadU16(&payload[2]),
                                    payload[4], payload[5] != 0U);
        break;

    case HOST_COMMAND_POSITION:
        if ((payload_length != 11U) || (payload[1] > 1U) ||
            (HostProtocol_ReadU16(&payload[2]) > APP_CONFIG_MOTOR_MAX_RPM) ||
            (payload[9] > 1U) ||
            (payload[10] > 1U) || !HostProtocol_AddressMask(payload[0]))
        {
            return HostProtocol_SendAck(sequence, command, HOST_ACK_INVALID_ARGUMENT);
        }
        Mecanum_Cancel();
        status = Emm_V5_Pos_Control(payload[0], payload[1], HostProtocol_ReadU16(&payload[2]),
                                    payload[4], HostProtocol_ReadU32(&payload[5]),
                                    payload[9] != 0U, payload[10] != 0U);
        break;

    case HOST_COMMAND_STOP:
        if ((payload_length != 2U) || (payload[1] > 1U))
        {
            return HostProtocol_SendAck(sequence, command, HOST_ACK_INVALID_ARGUMENT);
        }
        if ((s_control_mode == CONTROL_CHASSIS) || ((payload[0] == 0U) && payload[1] == 0U))
        {
            return HostProtocol_SendAck(sequence, command,
                                       HostProtocol_MapMotorStatus(HostProtocol_StopAll()));
        }
        status = Emm_V5_Stop_Now(payload[0], payload[1] != 0U);
        if ((status == PROJECT_STATUS_ERROR) && (payload[0] == 0U) && (payload[1] == 0U))
        {
            s_stop_retry_pending = true;
            s_stop_retry_is_failsafe = false;
        }
        break;

    case HOST_COMMAND_EMERGENCY_STOP:
        if (payload_length != 0U)
        {
            return HostProtocol_SendAck(sequence, command, HOST_ACK_LENGTH_ERROR);
        }
        s_estop_latched = true;
        HostProtocol_SafetyStop(SAFETY_FAULT_ESTOP);
        /* Drop any already-buffered follow-on actions; recovery requires a NEW request. */
        BSP_UART_DiscardRxData(BSP_UART_PORT_HOST);
        return HostProtocol_SendAck(sequence, command, HOST_ACK_OK);

    case HOST_COMMAND_SYNC:
        if (payload_length != 1U)
        {
            return HostProtocol_SendAck(sequence, command, HOST_ACK_LENGTH_ERROR);
        }
        if ((s_control_mode != CONTROL_SINGLE) ||
            !(s_staged_motor_mask & HostProtocol_AddressMask(payload[0])))
        {
            return HostProtocol_SendAck(sequence, command, HOST_ACK_INVALID_ARGUMENT);
        }
        status = Emm_V5_Synchronous_motion(payload[0]);
        break;

    case HOST_COMMAND_ZERO:
        if (payload_length != 1U)
        {
            return HostProtocol_SendAck(sequence, command, HOST_ACK_LENGTH_ERROR);
        }
        if ((s_test_state == SELF_TEST_RUNNING) || s_host_failsafe_armed ||
            Mecanum_IsBusy() || s_chassis_result_pending || s_health_waiting)
        {
            return HostProtocol_SendAck(sequence, command, HOST_ACK_BUSY);
        }
        status = Emm_V5_Reset_CurPos_To_Zero(payload[0]);
        break;

    case HOST_COMMAND_READ:
        if ((payload_length != 2U) || !HostProtocol_ParseReadItem(payload[1], &parameter))
        {
            return HostProtocol_SendAck(sequence, command, HOST_ACK_INVALID_ARGUMENT);
        }
        if ((s_test_state == SELF_TEST_RUNNING) || Mecanum_IsBusy() || s_health_waiting)
        {
            return HostProtocol_SendAck(sequence, command, HOST_ACK_BUSY);
        }
        status = Emm_V5_Read_Sys_Params(payload[0], parameter);
        break;

    case HOST_COMMAND_HOME:
        if ((payload_length != 3U) || (payload[1] > 3U) || (payload[2] > 1U) ||
            !HostProtocol_AddressMask(payload[0]))
        {
            return HostProtocol_SendAck(sequence, command, HOST_ACK_INVALID_ARGUMENT);
        }
        Mecanum_Cancel();
        status = Emm_V5_Origin_Trigger_Return(payload[0], payload[1], payload[2] != 0U);
        break;

    default:
        return HostProtocol_SendAck(sequence, command, HOST_ACK_UNKNOWN_COMMAND);
    }

    if (status == PROJECT_STATUS_OK)
    {
        if ((command == HOST_COMMAND_POSITION) || (command == HOST_COMMAND_HOME) ||
            (command == HOST_COMMAND_SYNC))
        {
            Host_ActionCache_t *entry = &s_action_cache[s_action_cache_next];
            entry->valid = true;
            entry->sequence = sequence;
            entry->command = command;
            entry->length = payload_length;
            memcpy(entry->payload, payload, payload_length);
            entry->tick = xTaskGetTickCount();
            s_action_cache_next = (uint8_t)((s_action_cache_next + 1U) % HOST_ACTION_CACHE_DEPTH);
        }
        if (((command == HOST_COMMAND_VELOCITY) && (HostProtocol_ReadU16(&payload[2]) != 0U)) ||
            /* Absolute position zero can move; preserve protection for all nonzero targets. */
            ((command == HOST_COMMAND_POSITION) &&
             ((HostProtocol_ReadU32(&payload[5]) != 0U) || (payload[9] != 0U))) ||
            (command == HOST_COMMAND_SYNC) || (command == HOST_COMMAND_HOME))
        {
            HostProtocol_AuthorizeMotion(sequence);
            s_control_mode = CONTROL_SINGLE;
            if (command == HOST_COMMAND_SYNC)
            {
                uint8_t mask = s_staged_motor_mask & HostProtocol_AddressMask(payload[0]);
                s_active_motor_mask |= mask;
                s_staged_motor_mask &= (uint8_t)~mask;
            }
            else
            {
                bool staged = (command == HOST_COMMAND_VELOCITY) ? payload[5] != 0U :
                              ((command == HOST_COMMAND_POSITION) ? payload[10] != 0U : payload[2] != 0U);
                if (staged) { s_staged_motor_mask |= HostProtocol_AddressMask(payload[0]); }
                else { s_active_motor_mask |= HostProtocol_AddressMask(payload[0]); }
            }
        }
    }

    return HostProtocol_SendAck(sequence, command, HostProtocol_MapMotorStatus(status));
}

static Project_Status_t HostProtocol_ProcessMotorInput(void)
{
    uint8_t byte;
    /* 完整帧在本次调用内消费完，ready 不需要保存成跨周期全局变量。 */
    bool frame_ready = false;
    TickType_t now = xTaskGetTickCount();

    if ((s_motor_frame_length != 0U) &&
        ((TickType_t)(now - s_motor_last_byte_tick) >=
         pdMS_TO_TICKS(APP_CONFIG_PROTOCOL_FRAME_TIMEOUT_MS)))
    {
        s_motor_frame_length = 0U;
        ++s_motor_protocol_error_count;
    }

    while (!frame_ready && BSP_UART_TryReadByte(BSP_UART_PORT_MOTOR, &byte))
    {
        s_motor_last_byte_tick = xTaskGetTickCount();

        if (s_motor_frame_length >= sizeof(s_motor_frame))
        {
            s_motor_frame_length = 0U;
            ++s_motor_protocol_error_count;
        }
        s_motor_frame[s_motor_frame_length++] = byte;
        while (s_motor_frame_length >= 2U)
        {
            uint32_t expected = HostProtocol_MotorFrameSize();
            if ((s_motor_frame[1] == 0x43U) && (s_motor_frame_length == 2U)) { break; }
            if (expected != 0U && s_motor_frame_length < expected) { break; }
            if (expected != 0U && s_motor_frame_length == expected &&
                s_motor_frame[expected - 1U] == HOST_MOTOR_FRAME_END)
            {
                frame_ready = true;
                break;
            }
            /* Bad function, length or tail: slide one byte; 6B in payload is NOT a delimiter. */
            memmove(s_motor_frame, &s_motor_frame[1], --s_motor_frame_length);
            ++s_motor_protocol_error_count;
        }
    }

    if (!frame_ready)
    {
        return PROJECT_STATUS_OK;
    }

    Mecanum_HandleMotorResponse(s_motor_frame, s_motor_frame_length);
    HostProtocol_HandleSelfTestMotor(s_motor_frame, s_motor_frame_length);
    byte = (uint8_t)s_motor_frame_length;
    s_motor_frame_length = 0U;

    /*
     * 电机回复事件入队失败（队列满）时直接丢弃该转发帧：
     * 电机状态机已由 Mecanum_HandleMotorResponse() 处理完，这里不向上报错。
     */
    (void)HostProtocol_SendEvent(HOST_PROTOCOL_EVENT_MOTOR_REPLY, s_motor_frame, byte);
    return PROJECT_STATUS_OK;
}

static uint32_t HostProtocol_MotorFrameSize(void)
{
    /* 6B 可出现在二进制参数中，先确定长度，再验证最后一个字节。 */
    switch (s_motor_frame[1])
    {
    case 0x1FU: case 0x24U: case 0x27U: return 5U;
    case 0x35U: return 6U;
    case 0x36U: case 0x37U: return 8U;
    case 0x43U:
        if (s_motor_frame_length < 3U) { return 0U; }
        return (s_motor_frame[2] >= 5U && s_motor_frame[2] <= sizeof(s_motor_frame)) ?
                   s_motor_frame[2] : 0U;
    case 0x00U: case 0x0AU: case 0x3AU: case 0x3BU: case 0x9AU:
    case 0xF3U: case 0xF6U: case 0xFDU: case 0xFEU: case 0xFFU: return 4U;
    default: return 0U;
    }
}

static Project_Status_t HostProtocol_ProcessFailsafe(void)
{
    /* 处理此前未成功入队的停车请求；重试成功也不解除自检/急停/底盘故障锁。 */
    Project_Status_t status;
    TickType_t now;

    if (!s_host_failsafe_armed)
    {
        if (!s_stop_retry_pending)
        {
            return PROJECT_STATUS_OK;
        }
    }

    if (s_stop_retry_pending)
    {
        status = Emm_V5_Stop_Now(0U, false);
        if (status == PROJECT_STATUS_ERROR)
        {
            return PROJECT_STATUS_OK;
        }
        if (status != PROJECT_STATUS_OK)
        {
            return status;
        }

        s_stop_retry_pending = false;
        if (s_stop_retry_is_failsafe)
        {
            ++s_host_failsafe_trip_count;
        }
        s_stop_retry_is_failsafe = false;
        s_host_failsafe_armed = false;
        s_health_waiting = false;
        return PROJECT_STATUS_OK;
    }

    now = xTaskGetTickCount();
    if ((TickType_t)(now - s_last_valid_host_tick) <
        pdMS_TO_TICKS(APP_CONFIG_HOST_LINK_TIMEOUT_MS))
    {
        return PROJECT_STATUS_OK;
    }

    HostProtocol_SafetyStop(SAFETY_FAULT_LINK);
    if (s_stop_retry_pending) { s_stop_retry_is_failsafe = true; }
    else { ++s_host_failsafe_trip_count; }
    return PROJECT_STATUS_OK;
}

static Project_Status_t HostProtocol_SendStatus(uint8_t sequence)
{
    uint8_t data[64];
    uint8_t offset = 0U;

    data[offset++] = HOST_ACK_OK;
#if (APP_CONFIG_IMU_ENABLED == 1U)
    HostProtocol_WriteU32(&data[offset], IMU_GetBaudRate());
    offset += 4U;
    data[offset++] = IMU_IsYawStationaryLocked() ? 1U : 0U;
    HostProtocol_WriteU32(&data[offset], IMU_GetValidFrameCount());
    offset += 4U;
    HostProtocol_WriteU32(&data[offset], IMU_GetChecksumErrorCount());
    offset += 4U;
    HostProtocol_WriteU32(&data[offset], BSP_UART_GetDroppedByteCount(BSP_UART_PORT_IMU));
    offset += 4U;
    HostProtocol_WriteU32(&data[offset], BSP_UART_GetDmaErrorCount(BSP_UART_PORT_IMU));
    offset += 4U;
#else
    HostProtocol_WriteU32(&data[offset], 0U);
    offset += 4U;
    data[offset++] = 0U;
    HostProtocol_WriteU32(&data[offset], 0U);
    offset += 4U;
    HostProtocol_WriteU32(&data[offset], 0U);
    offset += 4U;
    HostProtocol_WriteU32(&data[offset], 0U);
    offset += 4U;
    HostProtocol_WriteU32(&data[offset], 0U);
    offset += 4U;
#endif
    HostProtocol_WriteU32(&data[offset], BSP_UART_GetDroppedByteCount(BSP_UART_PORT_MOTOR));
    offset += 4U;
    HostProtocol_WriteU32(&data[offset], BSP_UART_GetDmaErrorCount(BSP_UART_PORT_MOTOR));
    offset += 4U;
    data[offset++] = Mecanum_IsBusy() ? 1U : 0U;
    data[offset++] = s_host_failsafe_armed ? 1U : 0U;
    HostProtocol_WriteU32(&data[offset], s_host_failsafe_trip_count);
    offset += 4U;
    HostProtocol_WriteU32(&data[offset], s_host_protocol_error_count);
    offset += 4U;
    HostProtocol_WriteU32(&data[offset], s_motor_protocol_error_count);
    offset += 4U;
    offset += HostProtocol_WriteSafety(&data[offset]);
    return HostProtocol_QueueFrame(sequence, HOST_RESPONSE_INDEX(HOST_COMMAND_STATUS), data, offset, false);
}

static void HostProtocol_StartSelfTest(void)
{
    /* 与普通全车停车共用同一条清理路径，但不在此处清除急停锁/底盘故障锁。 */
    (void)HostProtocol_StopAll();
    s_test_state = SELF_TEST_RUNNING;
    s_test_motor_mask = 0U;
    s_test_index = 0U;
    s_test_attempts = 0U;
    s_test_waiting = false;
    s_test_clock_started = false;
    s_safety_fault = 0U;
    s_safety_fault_address = 0U;
    s_safety_event_pending = true;
    /* 清除旧回复，避免把上次检测的残留字节当成本次应答。 */
    BSP_UART_DiscardRxData(BSP_UART_PORT_MOTOR);
    s_motor_frame_length = 0U;
#if (APP_CONFIG_IMU_ENABLED == 1U)
    s_observed_imu_samples = IMU_GetSampleCount();
    s_test_imu_seen = false;
#endif
}

static void HostProtocol_SafetyStop(uint8_t reason)
{
    s_test_state = SELF_TEST_FAILED;
    s_safety_fault = reason;
    s_safety_event_pending = true;
    (void)HostProtocol_StopAll();
}

static uint8_t HostProtocol_AddressMask(uint8_t address)
{
    /* 地址0表示全部四轮；其他地址只能匹配配置表，不用address-1推导车轮位。 */
    uint8_t index;
    if (address == 0U) { return 0x0FU; }
    for (index = 0U; index < 4U; ++index)
    {
        if (address == s_test_addresses[index]) { return (uint8_t)(1U << index); }
    }
    return 0U;
}

static void HostProtocol_AuthorizeMotion(uint8_t sequence)
{
    /* 合并原“布防+授权”两个相邻步骤：首次布防才初始化巡检，续期保留巡检进度。 */
    if (!s_host_failsafe_armed)
    {
        s_health_index = 0U;
        s_health_attempts = 0U;
        s_health_waiting = false;
        s_health_next_tick = xTaskGetTickCount() + pdMS_TO_TICKS(APP_CONFIG_MOTOR_HEALTH_PERIOD_MS);
    }
    s_host_failsafe_armed = true;
    s_motion_sequence = sequence;
    s_last_motion_tick = xTaskGetTickCount();
}

static void HostProtocol_EndMotion(void)
{
    /* 仅撤销软件动作/授权，不发送停车、不清除任何故障锁。 */
    Mecanum_Cancel();
    s_host_failsafe_armed = false;
    s_health_waiting = false;
    s_control_mode = CONTROL_IDLE;
    s_active_motor_mask = 0U;
    s_staged_motor_mask = 0U;
}

static Project_Status_t HostProtocol_StopAll(void)
{
    /* 普通停车、自检开始、故障停车共用；发送失败保留请求，下周期继续尝试。 */
    Project_Status_t status;
    HostProtocol_EndMotion();
    status = Emm_V5_Stop_Now(0U, false);
    s_stop_retry_pending = status != PROJECT_STATUS_OK;
    s_stop_retry_is_failsafe = false;
    return status;
}

static void HostProtocol_HandleSelfTestMotor(const uint8_t *frame, uint32_t length)
{
    /* 先检查故障标志，再推进当前地址的自检/巡检；异常回复不能被计作健康通过。 */
    /* A valid FLAG reply can still describe a stalled/protected or disabled moving motor. */
    if ((length == 4U) && frame[1] == 0x3AU && frame[3] == HOST_MOTOR_FRAME_END &&
        ((s_test_state == SELF_TEST_RUNNING) || (s_test_state == SELF_TEST_PASSED)))
    {
        uint8_t mask = HostProtocol_AddressMask(frame[0]);
        if ((frame[0] != 0U) && mask &&
            (((frame[2] & 0xFCU) != 0U) ||
             (((s_active_motor_mask & mask) != 0U) && (frame[2] & 1U) == 0U)))
        {
            s_safety_fault_address = frame[0];
            HostProtocol_SafetyStop(SAFETY_FAULT_MOTOR_STATUS);
            return;
        }
    }
    if ((s_test_state == SELF_TEST_PASSED) && s_health_waiting &&
        (length == 4U) && (frame[0] == s_test_addresses[s_health_index]) &&
        (frame[1] == 0x3AU) && (frame[3] == HOST_MOTOR_FRAME_END))
    {
        s_health_waiting = false;
        s_health_attempts = 0U;
        ++s_health_index;
        if (s_health_index >= 4U)
        {
            s_health_index = 0U;
            s_health_next_tick = xTaskGetTickCount() +
                pdMS_TO_TICKS(APP_CONFIG_MOTOR_HEALTH_PERIOD_MS);
        }
    }
    if ((s_test_state == SELF_TEST_RUNNING) && s_test_waiting &&
        (s_test_index < 4U) && (length == 4U) &&
        (frame[0] == s_test_addresses[s_test_index]) && (frame[1] == 0x3AU) &&
        (frame[3] == HOST_MOTOR_FRAME_END))
    {
        s_test_motor_mask |= (uint8_t)(1U << s_test_index);
        ++s_test_index;
        s_test_attempts = 0U;
        s_test_waiting = false;
    }
}

static uint8_t HostProtocol_WriteSafety(uint8_t *data)
{
    /* 14字节布局与 STATUS 的尾部/E3 共用，顺序是外部协议，不能随变量排序调整。 */
    data[0] = s_test_state;
    data[1] = s_test_motor_mask; /* bit0/1/2/3 = 左前/右前/左后/右后。 */
#if (APP_CONFIG_IMU_ENABLED == 1U)
    data[2] = s_test_imu_seen ? 1U : 0U;
#else
    data[2] = 2U; /* IMU 功能未编译，不属于必检项。 */
#endif
    data[3] = s_safety_fault;
    data[4] = s_safety_fault_address;
    data[5] = (s_stop_retry_pending || Mecanum_StopPending()) ? 1U : 0U;
    HostProtocol_WriteU32(&data[6], Mecanum_GetFaultCount());
    data[10] = s_estop_latched ? 1U : 0U;
    data[11] = s_control_mode;
    data[12] = s_host_failsafe_armed ? 1U : 0U;
    data[13] = s_motion_sequence;
    return 14U;
}

static void HostProtocol_CheckMotionExpiry(void)
{
    TickType_t now = xTaskGetTickCount();
    /* 无符号 tick 差值支持计数器回绕；必须在新帧刷新链路时间之前调用。 */
    if (s_host_failsafe_armed &&
        (((TickType_t)(now - s_last_valid_host_tick) >= pdMS_TO_TICKS(APP_CONFIG_HOST_LINK_TIMEOUT_MS)) ||
         ((TickType_t)(now - s_last_motion_tick) >= pdMS_TO_TICKS(APP_CONFIG_MOTION_LEASE_MS))))
    {
        uint8_t reason = ((TickType_t)(now - s_last_valid_host_tick) >=
                          pdMS_TO_TICKS(APP_CONFIG_HOST_LINK_TIMEOUT_MS)) ?
                             SAFETY_FAULT_LINK : SAFETY_FAULT_LEASE;
        HostProtocol_SafetyStop(reason);
        if (s_stop_retry_pending) { s_stop_retry_is_failsafe = true; }
        else { ++s_host_failsafe_trip_count; }
    }
}

static void HostProtocol_ProcessSafety(void)
{
    /* 非阻塞推进：先故障/超时，再自检/巡检，最后尝试入队E4/E3安全事件。 */
    TickType_t now = xTaskGetTickCount();
    uint8_t data[14];
    bool imu_ready = true;
    uint32_t faults = Mecanum_GetFaultCount();

    /* Also rechecked per host frame, so a long receive batch cannot hide expiry. */
    HostProtocol_CheckMotionExpiry();

    if (faults != s_observed_motor_faults)
    {
        s_observed_motor_faults = faults;
        s_safety_fault_address = Mecanum_GetFaultAddress();
        HostProtocol_SafetyStop(SAFETY_FAULT_MOTOR);
    }
#if (APP_CONFIG_IMU_ENABLED == 1U)
    {
        uint32_t samples = IMU_GetSampleCount();
        if (samples != s_observed_imu_samples)
        {
            s_observed_imu_samples = samples;
            s_last_imu_tick = now;
            s_test_imu_seen = true;
        }
        imu_ready = s_test_imu_seen &&
            ((TickType_t)(now - s_last_imu_tick) < pdMS_TO_TICKS(APP_CONFIG_IMU_STALE_TIMEOUT_MS));
        if ((s_test_state == SELF_TEST_PASSED) && !imu_ready)
        {
            s_test_imu_seen = false;
            HostProtocol_SafetyStop(SAFETY_FAULT_IMU);
        }
    }
#endif
    if (s_test_state == SELF_TEST_RUNNING)
    {
        if (!s_test_clock_started)
        {
            s_test_clock_started = true;
            s_test_start_tick = now;
        }
        if ((s_test_index >= 4U) && (s_test_motor_mask == 0x0FU) && imu_ready &&
            !s_stop_retry_pending && !Mecanum_StopPending())
        {
            s_test_state = SELF_TEST_PASSED;
            Mecanum_ClearFault();
            s_estop_latched = false; /* Only an explicit restart can reach this state after ESTOP. */
            s_safety_event_pending = true;
        }
        else if ((TickType_t)(now - s_test_start_tick) >=
                 pdMS_TO_TICKS(APP_CONFIG_SELF_TEST_TIMEOUT_MS))
        {
            HostProtocol_SafetyStop(SAFETY_FAULT_SELF_TEST);
        }
        else if (!s_stop_retry_pending && !Mecanum_StopPending() && (s_test_index < 4U))
        {
            if (s_test_waiting && ((TickType_t)(now - s_test_probe_tick) >=
                pdMS_TO_TICKS(APP_CONFIG_SELF_TEST_MOTOR_TIMEOUT_MS)))
            {
                s_test_waiting = false;
                if (s_test_attempts >= APP_CONFIG_SELF_TEST_MOTOR_ATTEMPTS)
                {
                    if (s_safety_fault_address == 0U)
                    {
                        s_safety_fault_address = s_test_addresses[s_test_index];
                    }
                    ++s_test_index;
                    s_test_attempts = 0U;
                }
            }
            if (!s_test_waiting && (s_test_index < 4U) &&
                (Emm_V5_Read_Sys_Params(s_test_addresses[s_test_index], S_FLAG) == PROJECT_STATUS_OK))
            {
                ++s_test_attempts;
                s_test_waiting = true;
                s_test_probe_tick = now;
            }
        }
    }
    if ((s_test_state == SELF_TEST_PASSED) && s_host_failsafe_armed && !Mecanum_IsBusy())
    {
        if (s_health_waiting && ((TickType_t)(now - s_health_tick) >=
            pdMS_TO_TICKS(APP_CONFIG_SELF_TEST_MOTOR_TIMEOUT_MS)))
        {
            s_health_waiting = false;
            if (s_health_attempts >= APP_CONFIG_SELF_TEST_MOTOR_ATTEMPTS)
            {
                s_test_motor_mask &= (uint8_t)~(1U << s_health_index);
                s_safety_fault_address = s_test_addresses[s_health_index];
                HostProtocol_SafetyStop(SAFETY_FAULT_MOTOR);
            }
        }
        if ((s_test_state == SELF_TEST_PASSED) && !s_health_waiting &&
            ((int32_t)(now - s_health_next_tick) >= 0) &&
            (Emm_V5_Read_Sys_Params(s_test_addresses[s_health_index], S_FLAG) == PROJECT_STATUS_OK))
        {
            ++s_health_attempts;
            s_health_waiting = true;
            s_health_tick = now;
        }
    }
    if (s_chassis_result_pending && !Mecanum_IsBusy())
    {
        data[0] = s_chassis_sequence;
        data[1] = Mecanum_GetResult();
        data[2] = data[1] == MECANUM_RESULT_FAILED ? Mecanum_GetFaultAddress() : 0U;
        if (HostProtocol_SendEvent(HOST_PROTOCOL_EVENT_CHASSIS_RESULT, data, 3U) == PROJECT_STATUS_OK)
        {
            s_chassis_result_pending = false;
        }
    }
    if (s_safety_event_pending)
    {
        uint8_t length = HostProtocol_WriteSafety(data);
        if (HostProtocol_SendEvent(HOST_PROTOCOL_EVENT_SELF_TEST, data, length) == PROJECT_STATUS_OK)
        {
            s_safety_event_pending = false;
        }
    }
}

static Project_Status_t HostProtocol_SendAck(uint8_t sequence, uint8_t request_command,
                                              uint8_t status)
{
    return HostProtocol_QueueFrame(sequence, HOST_RESPONSE_INDEX(request_command), &status, 1U, false);
}

static Project_Status_t HostProtocol_QueueFrame(uint8_t sequence, uint8_t command,
                                                const uint8_t *data, uint8_t length,
                                                bool replace_same_command)
{
    uint8_t frame[HOST_FRAME_MAX_SIZE];
    uint16_t crc;
    uint16_t total_length;
    uint32_t previous_primask;
    uint8_t index;

    if ((length > HOST_FRAME_MAX_PAYLOAD_LENGTH) || ((data == NULL) && (length != 0U)))
    {
        return PROJECT_STATUS_INVALID_ARGUMENT;
    }

    frame[0] = HOST_FRAME_SYNC;
    frame[1] = (uint8_t)(length + HOST_FRAME_MIN_SIZE);
    frame[2] = HOST_DESTINATION_HOST;
    frame[3] = sequence;
    frame[4] = command;
    if (length != 0U)
    {
        memcpy(&frame[5], data, length);
    }
    crc = HostProtocol_Crc16(&frame[1], (uint16_t)length + 4U);
    frame[5U + length] = (uint8_t)crc;
    frame[6U + length] = (uint8_t)(crc >> 8U);
    total_length = (uint16_t)length + HOST_FRAME_MIN_SIZE;

    previous_primask = __get_PRIMASK();
    __disable_irq();

    if (replace_same_command)
    {
        /* 在已排队帧中寻找同命令帧并覆盖，队列顺序不变、容量不增加。 */
        for (index = 0U; index < s_tx_queue_count; ++index)
        {
            uint8_t slot = (uint8_t)((s_tx_queue_head + index) % HOST_TX_QUEUE_DEPTH);
            if (s_tx_queue[slot].data[4] == command)
            {
                memcpy(s_tx_queue[slot].data, frame, total_length);
                s_tx_queue[slot].length = (uint8_t)total_length;
                __set_PRIMASK(previous_primask);
                return PROJECT_STATUS_OK;
            }
        }
    }

    if (s_tx_queue_count >= HOST_TX_QUEUE_DEPTH)
    {
        /* 队列已满：返回 BUSY 语义，由调用方稍后重试，绝不覆盖已排队帧。 */
        __set_PRIMASK(previous_primask);
        return PROJECT_STATUS_ERROR;
    }

    index = (uint8_t)((s_tx_queue_head + s_tx_queue_count) % HOST_TX_QUEUE_DEPTH);
    memcpy(s_tx_queue[index].data, frame, total_length);
    s_tx_queue[index].length = (uint8_t)total_length;
    ++s_tx_queue_count;
    __set_PRIMASK(previous_primask);

    return PROJECT_STATUS_OK;
}

static Project_Status_t HostProtocol_FlushPendingOutput(void)
{
    Project_Status_t status;
    uint32_t previous_primask;
    uint8_t frame[HOST_FRAME_MAX_SIZE];
    uint8_t frame_length;

    while (s_tx_queue_count != 0U)
    {
        /* 在临界区内取出队头帧的完整快照，避免与入队/替换操作交错。 */
        previous_primask = __get_PRIMASK();
        __disable_irq();
        if (s_tx_queue_count == 0U)
        {
            __set_PRIMASK(previous_primask);
            break;
        }
        memcpy(frame, s_tx_queue[s_tx_queue_head].data, s_tx_queue[s_tx_queue_head].length);
        frame_length = s_tx_queue[s_tx_queue_head].length;
        __set_PRIMASK(previous_primask);

        status = BSP_UART_Write(BSP_UART_PORT_HOST, frame, frame_length);
        if (status == PROJECT_STATUS_ERROR)
        {
            /* USART1 TX 环形缓冲已满：队头保留，下个周期继续发送。 */
            return PROJECT_STATUS_OK;
        }
        if (status != PROJECT_STATUS_OK)
        {
            return status;
        }

        previous_primask = __get_PRIMASK();
        __disable_irq();
        s_tx_queue_head = (uint8_t)((s_tx_queue_head + 1U) % HOST_TX_QUEUE_DEPTH);
        --s_tx_queue_count;
        __set_PRIMASK(previous_primask);
    }

    return PROJECT_STATUS_OK;
}

static void HostProtocol_ResetReceiver(uint8_t latest_byte)
{
    s_rx_index = 0U;
    if (latest_byte == HOST_FRAME_SYNC)
    {
        s_rx_frame[0] = latest_byte;
        s_rx_state = HOST_RX_WAIT_LENGTH;
    }
    else
    {
        s_rx_state = HOST_RX_WAIT_SYNC;
    }
}

static uint16_t HostProtocol_Crc16(const uint8_t *data, uint16_t length)
{
    uint16_t crc = 0xFFFFU;
    uint16_t index;
    uint8_t bit;

    for (index = 0U; index < length; ++index)
    {
        crc ^= data[index];
        for (bit = 0U; bit < 8U; ++bit)
        {
            crc = (crc & 1U) ? (uint16_t)((crc >> 1U) ^ 0xA001U) : (uint16_t)(crc >> 1U);
        }
    }
    return crc;
}

static uint16_t HostProtocol_ReadU16(const uint8_t *data)
{
    return ((uint16_t)data[0] << 8U) | data[1];
}

static uint32_t HostProtocol_ReadU32(const uint8_t *data)
{
    return ((uint32_t)data[0] << 24U) | ((uint32_t)data[1] << 16U) |
           ((uint32_t)data[2] << 8U) | data[3];
}

static int16_t HostProtocol_ReadI16(const uint8_t *data)
{
    return (int16_t)HostProtocol_ReadU16(data);
}

static void HostProtocol_WriteU32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)(value >> 24U);
    data[1] = (uint8_t)(value >> 16U);
    data[2] = (uint8_t)(value >> 8U);
    data[3] = (uint8_t)value;
}

static bool HostProtocol_ParseReadItem(uint8_t value, SysParams_t *parameter)
{
    switch (value)
    {
    case S_VER:
    case S_VBUS:
    case S_CPHA:
    case S_VEL:
    case S_CPOS:
    case S_PERR:
    case S_FLAG:
    case S_State:
    case S_ORG:
        *parameter = (SysParams_t)value;
        return true;
    default:
        return false;
    }
}

static uint8_t HostProtocol_MapMotorStatus(Project_Status_t status)
{
    if (status == PROJECT_STATUS_OK)
    {
        return HOST_ACK_OK;
    }
    if (status == PROJECT_STATUS_INVALID_ARGUMENT)
    {
        return HOST_ACK_INVALID_ARGUMENT;
    }
    if (status == PROJECT_STATUS_ERROR)
    {
        return HOST_ACK_BUSY;
    }
    return HOST_ACK_MOTOR_ERROR;
}
