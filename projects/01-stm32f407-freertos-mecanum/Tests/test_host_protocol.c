#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "Emm_V5.h"
#include "FreeRTOS.h"
#include "app_config.h"
#include "bsp_uart.h"
#include "host_protocol.h"
#include "mecanum_chassis.h"

#define TEST_BUFFER_SIZE (512U)
#define TEST_FRAME_SYNC (0x5AU)
#define TEST_DESTINATION_CONTROLLER (0x01U)
#define TEST_COMMAND_PING (0x01U)
#define TEST_COMMAND_VELOCITY (0x11U)

static TickType_t s_fake_tick;
static TickType_t s_tick_on_host_read;
static uint8_t s_host_rx[TEST_BUFFER_SIZE];
static uint32_t s_host_rx_length;
static uint32_t s_host_rx_index;
static uint8_t s_motor_rx[TEST_BUFFER_SIZE];
static uint32_t s_motor_rx_length;
static uint32_t s_motor_rx_index;
static uint32_t s_broadcast_stop_count;
static uint32_t s_broadcast_stop_attempt_count;
static uint32_t s_stop_failures_remaining;
static uint32_t s_mecanum_cancel_count;
static uint32_t s_velocity_count;
static uint32_t s_position_count;
static uint8_t s_read_address;
static uint32_t s_read_count;
static uint8_t s_last_tx[255];
static uint32_t s_last_tx_length;
static uint32_t s_imu_samples;
static uint32_t s_motor_faults;
static bool s_motor_fault;
static bool s_chassis_busy;
static uint8_t s_chassis_result;
/* Capture only the host TX boundary; the production queue remains unmodified. */
static bool s_host_tx_busy;
static uint8_t s_sent_frames[16][255];
static uint32_t s_sent_frame_count;

#define TEST_CHECK(condition)                                                                  \
    do                                                                                         \
    {                                                                                          \
        if (!(condition))                                                                      \
        {                                                                                      \
            printf("FAIL line %u: %s\n", (unsigned)__LINE__, #condition);                     \
            return 1;                                                                          \
        }                                                                                      \
    } while (0)

TickType_t xTaskGetTickCount(void) { return s_fake_tick; }
uint32_t IMU_GetSampleCount(void) { return s_imu_samples; }
uint32_t IMU_GetValidFrameCount(void) { return s_imu_samples * 3U; }
uint32_t IMU_GetChecksumErrorCount(void) { return 0U; }
uint32_t IMU_GetBaudRate(void) { return 9600U; }
bool IMU_IsYawStationaryLocked(void) { return false; }

static uint16_t Test_Crc16(const uint8_t *data, uint32_t length)
{
    uint16_t crc = 0xFFFFU;
    uint32_t index;
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

static uint32_t Test_BuildFrame(uint8_t sequence, uint8_t command, const uint8_t *payload,
                                uint8_t payload_length, uint8_t *frame)
{
    uint16_t crc;
    uint32_t length = (uint32_t)payload_length + 7U;

    frame[0] = TEST_FRAME_SYNC;
    frame[1] = (uint8_t)length;
    frame[2] = TEST_DESTINATION_CONTROLLER;
    frame[3] = sequence;
    frame[4] = command;
    if (payload_length != 0U)
    {
        memcpy(&frame[5], payload, payload_length);
    }
    crc = Test_Crc16(&frame[1], (uint32_t)payload_length + 4U);
    frame[5U + payload_length] = (uint8_t)crc;
    frame[6U + payload_length] = (uint8_t)(crc >> 8U);
    return length;
}

static void Test_QueueHostFrame(uint8_t command, const uint8_t *payload, uint8_t payload_length)
{
    s_host_rx_length =
        Test_BuildFrame(1U, command, payload, payload_length, s_host_rx);
    s_host_rx_index = 0U;
}

static void Test_Reset(void)
{
    s_fake_tick = 0U;
    s_tick_on_host_read = 0U;
    s_host_rx_length = 0U;
    s_host_rx_index = 0U;
    s_motor_rx_length = 0U;
    s_motor_rx_index = 0U;
    s_broadcast_stop_count = 0U;
    s_broadcast_stop_attempt_count = 0U;
    s_stop_failures_remaining = 0U;
    s_mecanum_cancel_count = 0U;
    s_velocity_count = 0U;
    s_position_count = 0U;
    s_read_count = 0U;
    s_last_tx_length = 0U;
    s_imu_samples = 0U;
    s_host_tx_busy = false;
    s_sent_frame_count = 0U;
    (void)HostProtocol_Init();
}

bool BSP_UART_TryReadByte(BSP_UartPort_t port, uint8_t *data)
{
    if (port == BSP_UART_PORT_HOST)
    {
        if (s_host_rx_index >= s_host_rx_length)
        {
            return false;
        }
        if (s_host_rx_index == 0U && s_tick_on_host_read != 0U)
        {
            s_fake_tick = s_tick_on_host_read;
            s_tick_on_host_read = 0U;
        }
        *data = s_host_rx[s_host_rx_index++];
        return true;
    }
    if (port == BSP_UART_PORT_MOTOR)
    {
        if (s_motor_rx_index >= s_motor_rx_length)
        {
            return false;
        }
        *data = s_motor_rx[s_motor_rx_index++];
        return true;
    }
    return false;
}

Project_Status_t BSP_UART_Write(BSP_UartPort_t port, const uint8_t *data, uint32_t length)
{
    if (port == BSP_UART_PORT_HOST)
    {
        if (s_host_tx_busy) { return PROJECT_STATUS_ERROR; }
        if (s_sent_frame_count < 16U)
        {
            memcpy(s_sent_frames[s_sent_frame_count], data, length);
        }
        ++s_sent_frame_count;
        memcpy(s_last_tx, data, length);
        s_last_tx_length = length;
    }
    return PROJECT_STATUS_OK;
}

void BSP_UART_DiscardRxData(BSP_UartPort_t port)
{
    if (port == BSP_UART_PORT_HOST) { s_host_rx_index = s_host_rx_length; }
    if (port == BSP_UART_PORT_MOTOR)
    {
        s_motor_rx_index = s_motor_rx_length;
    }
}

uint32_t BSP_UART_GetDroppedByteCount(BSP_UartPort_t port)
{
    (void)port;
    return 0U;
}

uint32_t BSP_UART_GetDmaErrorCount(BSP_UartPort_t port)
{
    (void)port;
    return 0U;
}

Project_Status_t Emm_V5_En_Control(uint8_t address, bool state, bool synchronous)
{
    (void)address;
    (void)state;
    (void)synchronous;
    return PROJECT_STATUS_OK;
}

Project_Status_t Emm_V5_Vel_Control(uint8_t address, uint8_t direction, uint16_t rpm,
                                    uint8_t acceleration, bool synchronous)
{
    (void)address;
    (void)direction;
    (void)rpm;
    (void)acceleration;
    (void)synchronous;
    ++s_velocity_count;
    return PROJECT_STATUS_OK;
}

Project_Status_t Emm_V5_Pos_Control(uint8_t address, uint8_t direction, uint16_t rpm,
                                    uint8_t acceleration, uint32_t pulses, bool absolute,
                                    bool synchronous)
{
    (void)address;
    (void)direction;
    (void)rpm;
    (void)acceleration;
    (void)pulses;
    (void)absolute;
    (void)synchronous;
    ++s_position_count;
    return PROJECT_STATUS_OK;
}

Project_Status_t Emm_V5_Stop_Now(uint8_t address, bool synchronous)
{
    if ((address == 0U) && !synchronous)
    {
        ++s_broadcast_stop_attempt_count;
        if (s_stop_failures_remaining != 0U)
        {
            --s_stop_failures_remaining;
            return PROJECT_STATUS_ERROR;
        }
        ++s_broadcast_stop_count;
    }
    return PROJECT_STATUS_OK;
}

Project_Status_t Emm_V5_Synchronous_motion(uint8_t address)
{
    (void)address;
    return PROJECT_STATUS_OK;
}

Project_Status_t Emm_V5_Reset_CurPos_To_Zero(uint8_t address)
{
    (void)address;
    return PROJECT_STATUS_OK;
}

Project_Status_t Emm_V5_Read_Sys_Params(uint8_t address, SysParams_t parameter)
{
    (void)parameter;
    s_read_address = address;
    ++s_read_count;
    return PROJECT_STATUS_OK;
}

Project_Status_t Emm_V5_Origin_Trigger_Return(uint8_t address, uint8_t mode, bool synchronous)
{
    (void)address;
    (void)mode;
    (void)synchronous;
    return PROJECT_STATUS_OK;
}

Project_Status_t Mecanum_Drive(int16_t forward, int16_t left, int16_t counterclockwise,
                               uint16_t maximum_rpm, uint8_t acceleration)
{
    (void)forward;
    (void)left;
    (void)counterclockwise;
    (void)maximum_rpm;
    (void)acceleration;
    s_chassis_busy = true;
    s_chassis_result = MECANUM_RESULT_PENDING;
    return PROJECT_STATUS_OK;
}

void Mecanum_HandleMotorResponse(const uint8_t *frame, uint32_t length)
{
    (void)frame;
    (void)length;
}

void Mecanum_Cancel(void)
{
    ++s_mecanum_cancel_count;
    s_chassis_busy = false;
}

void Mecanum_Init(void)
{
    s_motor_faults = 0U;
    s_motor_fault = false;
    s_chassis_busy = false;
    s_chassis_result = MECANUM_RESULT_IDLE;
}
bool Mecanum_HasFault(void) { return s_motor_fault; }
bool Mecanum_StopPending(void) { return false; }
void Mecanum_ClearFault(void) { s_motor_fault = false; }
uint32_t Mecanum_GetFaultCount(void) { return s_motor_faults; }
uint8_t Mecanum_GetFaultAddress(void) { return 2U; }
uint8_t Mecanum_GetResult(void) { return s_chassis_result; }

bool Mecanum_IsBusy(void) { return s_chassis_busy; }

static void Test_MotorFlag(uint8_t address)
{
    const uint8_t response[] = {address, 0x3AU, 0U, 0x6BU};
    memcpy(s_motor_rx, response, sizeof(response));
    s_motor_rx_length = sizeof(response);
    s_motor_rx_index = 0U;
}

static int Test_CompleteSelfTest(void)
{
    uint32_t index;
    ++s_imu_samples;
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    for (index = 0U; index < 4U; ++index)
    {
        Test_MotorFlag(s_read_address);
        TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    }
    s_broadcast_stop_count = 0U;
    s_broadcast_stop_attempt_count = 0U;
    return 0;
}

static int Test_LinkTimeoutStopsMotors(void)
{
    static const uint8_t velocity_payload[] = {1U, 0U, 0U, 100U, 10U, 0U};

    Test_Reset();
    TEST_CHECK(Test_CompleteSelfTest() == 0);
    Test_QueueHostFrame(TEST_COMMAND_VELOCITY, velocity_payload, sizeof(velocity_payload));
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_broadcast_stop_count == 0U);

    s_fake_tick = 499U;
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_broadcast_stop_count == 0U);

    s_fake_tick = 500U;
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_broadcast_stop_count == 1U);
    TEST_CHECK(s_mecanum_cancel_count >= 2U);

    s_fake_tick = 1000U;
    ++s_imu_samples;
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_broadcast_stop_count == 1U);
    return 0;
}

static int Test_HeartbeatDoesNotExtendMotionLease(void)
{
    static const uint8_t velocity_payload[] = {1U, 0U, 0U, 100U, 10U, 0U};

    Test_Reset();
    TEST_CHECK(Test_CompleteSelfTest() == 0);
    Test_QueueHostFrame(TEST_COMMAND_VELOCITY, velocity_payload, sizeof(velocity_payload));
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);

    s_fake_tick = 400U;
    Test_QueueHostFrame(TEST_COMMAND_PING, NULL, 0U);
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);

    s_fake_tick = 499U;
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_broadcast_stop_count == 0U);

    s_fake_tick = 500U;
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_broadcast_stop_count == 1U);
    return 0;
}

static int Test_FailsafeRetriesWhenMotorQueueIsBusy(void)
{
    static const uint8_t velocity_payload[] = {1U, 0U, 0U, 100U, 10U, 0U};

    Test_Reset();
    TEST_CHECK(Test_CompleteSelfTest() == 0);
    Test_QueueHostFrame(TEST_COMMAND_VELOCITY, velocity_payload, sizeof(velocity_payload));
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);

    s_stop_failures_remaining = 2U;
    s_fake_tick = 500U;
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_broadcast_stop_attempt_count == 2U);
    TEST_CHECK(s_broadcast_stop_count == 0U);

    s_fake_tick = 502U;
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_broadcast_stop_attempt_count == 3U);
    TEST_CHECK(s_broadcast_stop_count == 1U);
    return 0;
}

static int Test_OversizeMotorFrameRecoversLocally(void)
{
    uint32_t index;

    Test_Reset();
    s_motor_rx_length = 65U;
    for (index = 0U; index < s_motor_rx_length; ++index)
    {
        s_motor_rx[index] = 0x55U;
    }
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);

    s_motor_rx[0] = 0x6BU;
    s_motor_rx_length = 1U;
    s_motor_rx_index = 0U;
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);

    Test_QueueHostFrame(TEST_COMMAND_PING, NULL, 0U);
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    return 0;
}

static int Test_BootInterlockAndEmergencyStop(void)
{
    static const uint8_t velocity[] = {1U, 0U, 0U, 100U, 10U, 0U};
    Test_Reset();
    TEST_CHECK(s_broadcast_stop_count == 1U); /* 不等 IMU 初始化，先请求停车。 */
    Test_QueueHostFrame(TEST_COMMAND_VELOCITY, velocity, sizeof(velocity));
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_velocity_count == 0U);
    TEST_CHECK(s_last_tx[4] == 0x91U && s_last_tx[5] == 7U);
    Test_QueueHostFrame(0x14U, NULL, 0U);
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_broadcast_stop_count == 2U);
    return 0;
}

static int Test_BootStopBusyKeepsMotionBlocked(void)
{
    static const uint8_t velocity[] = {1U, 0U, 0U, 100U, 10U, 0U};
    Test_Reset();
    s_broadcast_stop_count = 0U;
    s_stop_failures_remaining = 2U;
    s_read_count = 0U;
    TEST_CHECK(HostProtocol_Init() == PROJECT_STATUS_OK);
    Test_QueueHostFrame(TEST_COMMAND_VELOCITY, velocity, sizeof(velocity));
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_broadcast_stop_count == 0U && s_read_count == 0U && s_velocity_count == 0U);
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_broadcast_stop_count == 1U && s_read_count == 0U);
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_read_count == 1U);
    return 0;
}

static int Test_MissingMotorRetryAndExplicitRecovery(void)
{
    uint32_t index;
    static const uint8_t velocity[] = {1U, 0U, 0U, 100U, 10U, 0U};
    Test_Reset();
    ++s_imu_samples;
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_read_count == 1U && s_read_address == 2U);
    Test_MotorFlag(1U); /* 非当前地址不能通过自检。 */
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_read_count == 1U);
    s_fake_tick = 500U;
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_read_count == 2U && s_read_address == 2U);
    s_fake_tick = 1000U;
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_read_count == 3U && s_read_address == 1U);
    for (index = 0U; index < 3U; ++index)
    {
        Test_MotorFlag(s_read_address);
        TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    }
    s_fake_tick = 5000U;
    ++s_imu_samples;
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    Test_QueueHostFrame(0x02U, NULL, 0U);
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_last_tx_length == 65U);
    TEST_CHECK(s_last_tx[49] == 3U && s_last_tx[50] == 0x0EU);
    TEST_CHECK(s_last_tx[53] == 2U);
    Test_QueueHostFrame(TEST_COMMAND_VELOCITY, velocity, sizeof(velocity));
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_velocity_count == 0U);
    Test_QueueHostFrame(0x03U, NULL, 0U);
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(Test_CompleteSelfTest() == 0);
    Test_QueueHostFrame(TEST_COMMAND_VELOCITY, velocity, sizeof(velocity));
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_velocity_count == 1U);
    return 0;
}

static int Test_RuntimeMotorFaultStopsAndLocks(void)
{
    static const uint8_t velocity[] = {1U, 0U, 0U, 100U, 10U, 0U};
    Test_Reset();
    TEST_CHECK(Test_CompleteSelfTest() == 0);
    ++s_motor_faults;
    s_motor_fault = true;
    Test_QueueHostFrame(TEST_COMMAND_VELOCITY, velocity, sizeof(velocity));
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_velocity_count == 0U && s_broadcast_stop_count == 1U);
    Test_QueueHostFrame(0x03U, NULL, 0U);
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(Test_CompleteSelfTest() == 0);
    TEST_CHECK(!s_motor_fault);
    return 0;
}

static int Test_ReadDoesNotCancelChassis(void)
{
    static const uint8_t read[] = {1U, 13U};
    uint32_t cancels;
    uint32_t reads;
    Test_Reset();
    TEST_CHECK(Test_CompleteSelfTest() == 0);
    cancels = s_mecanum_cancel_count;
    reads = s_read_count;
    s_chassis_busy = true;
    Test_QueueHostFrame(0x17U, read, sizeof(read));
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_mecanum_cancel_count == cancels);
    TEST_CHECK(s_read_count == reads && s_chassis_busy);
    return 0;
}

static int Test_OnlineMotorDisconnectStopsDespiteHeartbeat(void)
{
    static const uint8_t velocity[] = {1U, 0U, 0U, 100U, 10U, 0U};
    uint32_t tick;
    Test_Reset();
    TEST_CHECK(Test_CompleteSelfTest() == 0);
    Test_QueueHostFrame(TEST_COMMAND_VELOCITY, velocity, sizeof(velocity));
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    for (tick = 200U; tick <= 1800U; tick += 200U)
    {
        s_fake_tick = tick;
        ++s_imu_samples;
        { const uint8_t origin[] = {1U}; Test_QueueHostFrame(0x04U, origin, 1U); }
        TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    }
    TEST_CHECK(s_broadcast_stop_count == 1U);
    Test_QueueHostFrame(TEST_COMMAND_VELOCITY, velocity, sizeof(velocity));
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_velocity_count == 1U);
    return 0;
}

#if (APP_CONFIG_IMU_ENABLED == 1U)
static int Test_ImuMustBeFreshAndStaleImuStops(void)
{
    uint32_t index;
    static const uint8_t velocity[] = {1U, 0U, 0U, 100U, 10U, 0U};
    Test_Reset();
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    for (index = 0U; index < 4U; ++index)
    {
        Test_MotorFlag(s_read_address);
        TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    }
    Test_QueueHostFrame(TEST_COMMAND_VELOCITY, velocity, sizeof(velocity));
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_velocity_count == 0U); /* 无完整实时 IMU 样本，不允许运动。 */
    ++s_imu_samples;
    Test_QueueHostFrame(TEST_COMMAND_VELOCITY, velocity, sizeof(velocity));
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_velocity_count == 1U);
    s_broadcast_stop_count = 0U;
    s_fake_tick = APP_CONFIG_IMU_STALE_TIMEOUT_MS;
    Test_QueueHostFrame(TEST_COMMAND_PING, NULL, 0U); /* 心跳不能掩盖 IMU 停更。 */
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_broadcast_stop_count == 1U);
    Test_QueueHostFrame(TEST_COMMAND_VELOCITY, velocity, sizeof(velocity));
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_velocity_count == 1U);
    return 0;
}

static int Test_RecheckCannotReuseHistoricalImu(void)
{
    uint32_t index;
    static const uint8_t velocity[] = {1U, 0U, 0U, 100U, 10U, 0U};
    Test_Reset();
    TEST_CHECK(Test_CompleteSelfTest() == 0);
    Test_QueueHostFrame(0x03U, NULL, 0U);
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    for (index = 0U; index < 4U; ++index)
    {
        Test_MotorFlag(s_read_address);
        TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    }
    Test_QueueHostFrame(TEST_COMMAND_VELOCITY, velocity, sizeof(velocity));
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_velocity_count == 0U);
    ++s_imu_samples;
    Test_QueueHostFrame(TEST_COMMAND_VELOCITY, velocity, sizeof(velocity));
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_velocity_count == 1U);
    return 0;
}
#endif

static int Test_EstopLatchAndBufferedCommands(void)
{
    const uint8_t velocity[] = {1U, 0U, 0U, 100U, 10U, 0U};
    uint32_t first;
    Test_Reset();
    TEST_CHECK(Test_CompleteSelfTest() == 0);
    first = Test_BuildFrame(2U, 0x14U, NULL, 0U, s_host_rx);
    s_host_rx_length = first + Test_BuildFrame(3U, 0x11U, velocity, sizeof(velocity), &s_host_rx[first]);
    s_host_rx_index = 0U;
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_velocity_count == 0U && s_broadcast_stop_count == 1U);
    Test_QueueHostFrame(0x11U, velocity, sizeof(velocity));
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_last_tx[5] == 7U && s_velocity_count == 0U);
    Test_QueueHostFrame(0x03U, NULL, 0U);
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(Test_CompleteSelfTest() == 0);
    Test_QueueHostFrame(0x11U, velocity, sizeof(velocity));
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_last_tx[5] == 0U && s_velocity_count == 1U);
    return 0;
}

static int Test_RenewSequenceAndExpiry(void)
{
    const uint8_t velocity[] = {1U, 0U, 0U, 100U, 10U, 0U};
    const uint8_t good[] = {1U}, wrong[] = {2U};
    Test_Reset();
    TEST_CHECK(Test_CompleteSelfTest() == 0);
    Test_QueueHostFrame(0x11U, velocity, sizeof(velocity));
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    s_fake_tick = 200U;
    Test_QueueHostFrame(0x04U, wrong, 1U);
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_last_tx[5] == 4U);
    s_fake_tick = 400U;
    Test_QueueHostFrame(0x04U, good, 1U);
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_last_tx[5] == 0U);
    s_fake_tick = 800U;
    ++s_imu_samples;
    Test_QueueHostFrame(0x01U, NULL, 0U);
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_broadcast_stop_count == 0U);
    s_fake_tick = 900U;
    Test_QueueHostFrame(0x04U, good, 1U);
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_broadcast_stop_count == 1U && s_last_tx[5] == 7U);
    return 0;
}

static int Test_ControlModesAndFlagFault(void)
{
    const uint8_t chassis[] = {3U, 0xE8U, 0U, 0U, 0U, 0U, 0U, 100U, 10U};
    const uint8_t sync[] = {0U};
    const uint8_t flag[] = {2U, 0x3AU, 4U, 0x6BU};
    uint32_t cancels;
    Test_Reset();
    TEST_CHECK(Test_CompleteSelfTest() == 0);
    Test_QueueHostFrame(0x20U, chassis, sizeof(chassis));
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_chassis_busy);
    cancels = s_mecanum_cancel_count;
    Test_QueueHostFrame(0x15U, sync, sizeof(sync));
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_last_tx[5] == 5U && s_chassis_busy && s_mecanum_cancel_count == cancels);
    memcpy(s_motor_rx, flag, sizeof(flag));
    s_motor_rx_length = sizeof(flag);
    s_motor_rx_index = 0U;
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_broadcast_stop_count == 1U && !s_chassis_busy);
    return 0;
}

static int Test_BinaryMotorPayloadLength(void)
{
    const uint8_t frames[][8] = {
        {1U, 0x1FU, 0x6BU, 2U, 0x6BU},
        {1U, 0x35U, 0U, 0x6BU, 2U, 0x6BU},
        {1U, 0x36U, 0U, 0x6BU, 2U, 3U, 4U, 0x6BU},
        {1U, 0x43U, 8U, 0x6BU, 2U, 3U, 4U, 0x6BU}
    };
    const uint8_t lengths[] = {5U, 6U, 8U, 8U};
    uint32_t index;
    Test_Reset();
    TEST_CHECK(Test_CompleteSelfTest() == 0);
    for (index = 0U; index < 4U; ++index)
    {
        memcpy(s_motor_rx, frames[index], lengths[index]);
        s_motor_rx_length = lengths[index];
        s_motor_rx_index = 0U;
        TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
        TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
        TEST_CHECK(s_last_tx[4] == 0xE2U && s_last_tx_length == (uint32_t)lengths[index] + 7U);
        TEST_CHECK(memcmp(&s_last_tx[5], frames[index], lengths[index]) == 0);
    }
    return 0;
}

static int Test_AbsoluteZeroLeaseAndActionDedup(void)
{
    uint8_t position[] = {1U, 0U, 0U, 100U, 10U, 0U, 0U, 0U, 0U, 1U, 0U};
    Test_Reset();
    TEST_CHECK(Test_CompleteSelfTest() == 0);
    Test_QueueHostFrame(0x12U, position, sizeof(position));
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_position_count == 1U);
    s_fake_tick = 200U;
    Test_QueueHostFrame(0x12U, position, sizeof(position));
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_position_count == 1U && s_last_tx[5] == 0U);
    position[8] = 1U;
    Test_QueueHostFrame(0x12U, position, sizeof(position));
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_position_count == 1U && s_last_tx[5] == 4U);
    s_fake_tick = 500U;
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_broadcast_stop_count == 1U);
    return 0;
}

static int Test_ExpiryCrossingDuringHostInput(void)
{
    const uint8_t velocity[] = {1U, 0U, 0U, 100U, 10U, 0U};
    Test_Reset();
    TEST_CHECK(Test_CompleteSelfTest() == 0);
    Test_QueueHostFrame(0x11U, velocity, sizeof(velocity));
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    s_fake_tick = 499U; /* Safety checked here; the new command arrives one tick later. */
    s_tick_on_host_read = 500U;
    Test_QueueHostFrame(0x11U, velocity, sizeof(velocity));
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_velocity_count == 1U && s_broadcast_stop_count == 1U && s_last_tx[5] == 7U);
    return 0;
}

static int Test_MotorFlagFaultVariants(void)
{
    const uint8_t velocity[] = {1U, 0U, 0U, 100U, 10U, 0U};
    const uint8_t bad_flags[] = {0x04U, 0x08U, 0x80U, 0U};
    uint32_t index;
    for (index = 0U; index < sizeof(bad_flags); ++index)
    {
        Test_Reset();
        TEST_CHECK(Test_CompleteSelfTest() == 0);
        Test_QueueHostFrame(0x11U, velocity, sizeof(velocity));
        TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
        Test_MotorFlag(1U);
        s_motor_rx[2] = bad_flags[index];
        TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
        TEST_CHECK(s_broadcast_stop_count == 1U);
        Test_QueueHostFrame(0x11U, velocity, sizeof(velocity));
        TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
        TEST_CHECK(s_velocity_count == 1U);
    }
    return 0;
}

static int Test_HostOutputQueueWrapAndSnapshotReplacement(void)
{
    uint32_t round, index;
    uint8_t first_sequence = 0U;
    uint8_t value;
    Test_Reset();
    TEST_CHECK(Test_CompleteSelfTest() == 0);
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    /* Fill/drain beyond the ring capacity so head wraps repeatedly. */
    for (round = 0U; round < 12U; ++round)
    {
        s_sent_frame_count = 0U;
        s_host_tx_busy = true;
        for (index = 0U; index < 8U; ++index)
        {
            value = (uint8_t)index;
            TEST_CHECK(HostProtocol_SendEvent(HOST_PROTOCOL_EVENT_READY, &value, 1U) == PROJECT_STATUS_OK);
        }
        TEST_CHECK(HostProtocol_SendEvent(HOST_PROTOCOL_EVENT_READY, NULL, 0U) == PROJECT_STATUS_ERROR);
        TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
        TEST_CHECK(s_sent_frame_count == 0U);
        s_host_tx_busy = false;
        TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
        TEST_CHECK(s_sent_frame_count == 8U);
        first_sequence = s_sent_frames[0][3];
        for (index = 0U; index < 8U; ++index)
        {
            TEST_CHECK(s_sent_frames[index][3] == (uint8_t)(first_sequence + index));
            TEST_CHECK(s_sent_frames[index][4] == HOST_PROTOCOL_EVENT_READY);
            TEST_CHECK(s_sent_frames[index][5] == (uint8_t)index);
            TEST_CHECK(Test_Crc16(&s_sent_frames[index][1], 5U) ==
                       ((uint16_t)s_sent_frames[index][6] | ((uint16_t)s_sent_frames[index][7] << 8U)));
        }
    }
    s_sent_frame_count = 0U;
    value = 1U;
    TEST_CHECK(HostProtocol_SendEvent(HOST_PROTOCOL_EVENT_IMU_TEXT, &value, 1U) == PROJECT_STATUS_OK);
    TEST_CHECK(HostProtocol_SendEvent(HOST_PROTOCOL_EVENT_READY, NULL, 0U) == PROJECT_STATUS_OK);
    value = 2U;
    TEST_CHECK(HostProtocol_SendEvent(HOST_PROTOCOL_EVENT_IMU_TEXT, &value, 1U) == PROJECT_STATUS_OK);
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_sent_frame_count == 2U);
    TEST_CHECK(s_sent_frames[0][4] == HOST_PROTOCOL_EVENT_IMU_TEXT && s_sent_frames[0][5] == 2U);
    TEST_CHECK(s_sent_frames[1][4] == HOST_PROTOCOL_EVENT_READY);
    return 0;
}

static int Test_MaximumHostLengthAfterReceiverSimplification(void)
{
    uint8_t payload[248];
    uint32_t index;
    Test_Reset();
    TEST_CHECK(Test_CompleteSelfTest() == 0);
    for (index = 0U; index < sizeof(payload); ++index) { payload[index] = (uint8_t)index; }
    /* Unsupported command verifies full255-byte assembly/CRC without moving motors. */
    Test_QueueHostFrame(0x7FU, payload, sizeof(payload));
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_last_tx[4] == 0xFFU && s_last_tx[5] == 3U);
    TEST_CHECK(s_velocity_count == 0U && s_position_count == 0U);
    return 0;
}

int main(void)
{
    if ((Test_LinkTimeoutStopsMotors() != 0) || (Test_HeartbeatDoesNotExtendMotionLease() != 0) ||
        (Test_FailsafeRetriesWhenMotorQueueIsBusy() != 0) ||
        (Test_OversizeMotorFrameRecoversLocally() != 0) ||
        (Test_BootInterlockAndEmergencyStop() != 0) ||
        (Test_BootStopBusyKeepsMotionBlocked() != 0) ||
        (Test_MissingMotorRetryAndExplicitRecovery() != 0) ||
        (Test_RuntimeMotorFaultStopsAndLocks() != 0) ||
        (Test_ReadDoesNotCancelChassis() != 0) ||
        (Test_OnlineMotorDisconnectStopsDespiteHeartbeat() != 0) ||
        (Test_EstopLatchAndBufferedCommands() != 0) || (Test_RenewSequenceAndExpiry() != 0) ||
        (Test_ControlModesAndFlagFault() != 0) || (Test_BinaryMotorPayloadLength() != 0) ||
        (Test_AbsoluteZeroLeaseAndActionDedup() != 0) || (Test_ExpiryCrossingDuringHostInput() != 0) ||
        (Test_MotorFlagFaultVariants() != 0) || (Test_HostOutputQueueWrapAndSnapshotReplacement() != 0) ||
        (Test_MaximumHostLengthAfterReceiverSimplification() != 0))
    {
        return 1;
    }
#if (APP_CONFIG_IMU_ENABLED == 1U)
    if ((Test_ImuMustBeFreshAndStaleImuStops() != 0) || (Test_RecheckCannotReuseHistoricalImu() != 0))
    {
        return 1;
    }
#endif

    puts("host protocol tests passed");
    return 0;
}
