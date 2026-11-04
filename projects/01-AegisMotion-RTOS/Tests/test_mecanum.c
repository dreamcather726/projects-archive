#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "Emm_V5.h"
#include "FreeRTOS.h"
#include "app_config.h"
#include "bsp_uart.h"
#include "mecanum_chassis.h"

#define TEST_MAX_CALLS (16U)

typedef enum
{
    TEST_CALL_VELOCITY = 0,
    TEST_CALL_STOP,
    TEST_CALL_SYNC
} Test_CallType_t;

typedef struct
{
    Test_CallType_t type;
    uint8_t address;
    uint8_t direction;
    uint16_t rpm;
    uint8_t acceleration;
    bool synchronous;
} Test_MotorCall_t;

static TickType_t s_fake_tick;
static Test_MotorCall_t s_calls[TEST_MAX_CALLS];
static uint32_t s_call_count;
static uint32_t s_stop_failures;
static bool s_tx_idle = true;
static bool s_velocity_busy;

bool BSP_UART_IsTxIdle(BSP_UartPort_t port) { (void)port; return s_tx_idle; }

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

Project_Status_t Emm_V5_Vel_Control(uint8_t address, uint8_t direction, uint16_t rpm,
                                    uint8_t acceleration, bool synchronous)
{
    TEST_CHECK(s_call_count < TEST_MAX_CALLS);
    s_calls[s_call_count++] = (Test_MotorCall_t){
        TEST_CALL_VELOCITY, address, direction, rpm, acceleration, synchronous};
    if (s_velocity_busy) { return PROJECT_STATUS_ERROR; }
    return PROJECT_STATUS_OK;
}

Project_Status_t Emm_V5_Stop_Now(uint8_t address, bool synchronous)
{
    TEST_CHECK(s_call_count < TEST_MAX_CALLS);
    s_calls[s_call_count++] =
        (Test_MotorCall_t){TEST_CALL_STOP, address, 0U, 0U, 0U, synchronous};
    if (s_stop_failures != 0U)
    {
        --s_stop_failures;
        return PROJECT_STATUS_ERROR;
    }
    return PROJECT_STATUS_OK;
}

Project_Status_t Emm_V5_Synchronous_motion(uint8_t address)
{
    TEST_CHECK(s_call_count < TEST_MAX_CALLS);
    s_calls[s_call_count++] =
        (Test_MotorCall_t){TEST_CALL_SYNC, address, 0U, 0U, 0U, false};
    return PROJECT_STATUS_OK;
}

static void Test_Reset(void)
{
    s_fake_tick = 0U;
    s_call_count = 0U;
    s_stop_failures = 0U;
    s_tx_idle = true;
    s_velocity_busy = false;
    Mecanum_Init();
}

static int Test_Kinematics(void)
{
    Mecanum_WheelRpm_t rpm;

    TEST_CHECK(Mecanum_CalculateWheelRpm(1000, 0, 0, 100U, &rpm) == PROJECT_STATUS_OK);
    TEST_CHECK(rpm.left_front_rpm == 100);
    TEST_CHECK(rpm.right_front_rpm == 100);
    TEST_CHECK(rpm.left_rear_rpm == 100);
    TEST_CHECK(rpm.right_rear_rpm == 100);

    TEST_CHECK(Mecanum_CalculateWheelRpm(0, 1000, 0, 100U, &rpm) == PROJECT_STATUS_OK);
    TEST_CHECK(rpm.left_front_rpm == -100);
    TEST_CHECK(rpm.right_front_rpm == 100);
    TEST_CHECK(rpm.left_rear_rpm == 100);
    TEST_CHECK(rpm.right_rear_rpm == -100);

    TEST_CHECK(Mecanum_CalculateWheelRpm(1000, 1000, 1000, 100U, &rpm) ==
               PROJECT_STATUS_OK);
    TEST_CHECK(rpm.left_front_rpm == -33);
    TEST_CHECK(rpm.right_front_rpm == 100);
    TEST_CHECK(rpm.left_rear_rpm == 33);
    TEST_CHECK(rpm.right_rear_rpm == 33);

    TEST_CHECK(Mecanum_CalculateWheelRpm(1001, 0, 0, 100U, &rpm) ==
               PROJECT_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Mecanum_CalculateWheelRpm(0, 0, 0, APP_CONFIG_MOTOR_MAX_RPM + 1U, &rpm) ==
               PROJECT_STATUS_INVALID_ARGUMENT);
    TEST_CHECK(Mecanum_CalculateWheelRpm(0, 0, 0, 100U, NULL) ==
               PROJECT_STATUS_INVALID_ARGUMENT);
    return 0;
}

static int Test_SynchronizedSequence(void)
{
    static const uint8_t addresses[] = {2U, 1U, 3U, 4U};
    static const uint8_t directions[] = {0U, 1U, 0U, 1U};
    uint8_t response[4];
    uint32_t index;

    Test_Reset();
    TEST_CHECK(Mecanum_Drive(1000, 0, 0, 100U, 10U) == PROJECT_STATUS_OK);

    for (index = 0U; index < 4U; ++index)
    {
        TEST_CHECK(Mecanum_Process() == PROJECT_STATUS_OK);
        TEST_CHECK(s_call_count == (index + 1U));
        TEST_CHECK(s_calls[index].type == TEST_CALL_VELOCITY);
        TEST_CHECK(s_calls[index].address == addresses[index]);
        TEST_CHECK(s_calls[index].direction == directions[index]);
        TEST_CHECK(s_calls[index].rpm == 100U);
        TEST_CHECK(s_calls[index].acceleration == 10U);
        TEST_CHECK(s_calls[index].synchronous);

        response[0] = addresses[index];
        response[1] = 0xF6U;
        response[2] = 0x02U;
        response[3] = 0x6BU;
        Mecanum_HandleMotorResponse(response, sizeof(response));
    }

    TEST_CHECK(Mecanum_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_call_count == 5U);
    TEST_CHECK(s_calls[4].type == TEST_CALL_SYNC);
    TEST_CHECK(s_calls[4].address == 0U);
    TEST_CHECK(!Mecanum_IsBusy());
    TEST_CHECK(Mecanum_GetResult() == MECANUM_RESULT_SYNC_SENT);
    return 0;
}

static int Test_ResponseTimeout(void)
{
    Test_Reset();
    TEST_CHECK(Mecanum_Drive(1000, 0, 0, 100U, 10U) == PROJECT_STATUS_OK);
    TEST_CHECK(Mecanum_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_call_count == 1U);

    s_fake_tick = 10U;
    TEST_CHECK(Mecanum_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_call_count == 2U);
    TEST_CHECK(Mecanum_IsBusy());

    s_fake_tick = 20U;
    TEST_CHECK(Mecanum_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_call_count == 3U);
    TEST_CHECK(s_calls[2].type == TEST_CALL_STOP);
    TEST_CHECK(s_calls[2].address == 0U && !s_calls[2].synchronous);
    TEST_CHECK(!Mecanum_IsBusy());
    TEST_CHECK(Mecanum_HasFault());
    TEST_CHECK(Mecanum_GetFaultCount() == 1U);
    TEST_CHECK(Mecanum_GetFaultAddress() == 2U);
    TEST_CHECK(Mecanum_GetResult() == MECANUM_RESULT_FAILED);
    TEST_CHECK(Mecanum_Drive(1000, 0, 0, 100U, 10U) == PROJECT_STATUS_ERROR);
    Mecanum_Cancel();
    TEST_CHECK(Mecanum_HasFault());
    return 0;
}

static int Test_ErrorResponseStopsAndRetriesFullQueue(void)
{
    static const uint8_t failure[] = {2U, 0xF6U, 0xE2U, 0x6BU};
    Test_Reset();
    TEST_CHECK(Mecanum_Drive(1000, 0, 0, 100U, 10U) == PROJECT_STATUS_OK);
    TEST_CHECK(Mecanum_Process() == PROJECT_STATUS_OK);
    s_stop_failures = 2U;
    Mecanum_HandleMotorResponse(failure, sizeof(failure));
    TEST_CHECK(Mecanum_StopPending() && Mecanum_HasFault());
    Mecanum_ClearFault();
    TEST_CHECK(Mecanum_HasFault());
    TEST_CHECK(Mecanum_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(Mecanum_StopPending());
    TEST_CHECK(Mecanum_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(!Mecanum_StopPending());
    TEST_CHECK(s_call_count == 4U);
    Mecanum_ClearFault();
    TEST_CHECK(!Mecanum_HasFault());
    return 0;
}

static int Test_BusyAndMalformedResponse(void)
{
    static const uint8_t malformed[] = {2U, 0xF6U, 0x02U, 0x00U};
    Test_Reset();
    TEST_CHECK(Mecanum_Drive(1000, 0, 0, 100U, 10U) == PROJECT_STATUS_OK);
    TEST_CHECK(Mecanum_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(Mecanum_Drive(0, 0, 0, 100U, 10U) == PROJECT_STATUS_ERROR);
    Mecanum_HandleMotorResponse(malformed, sizeof(malformed));
    TEST_CHECK(Mecanum_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_call_count == 1U);
    return 0;
}

static int Test_TxDrainAndTransactionDeadline(void)
{
    Test_Reset();
    s_tx_idle = false;
    TEST_CHECK(Mecanum_Drive(1000, 0, 0, 100U, 10U) == PROJECT_STATUS_OK);
    TEST_CHECK(Mecanum_Process() == PROJECT_STATUS_OK);
    s_fake_tick = 100U;
    TEST_CHECK(Mecanum_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_call_count == 1U && Mecanum_IsBusy());
    s_tx_idle = true;
    TEST_CHECK(Mecanum_Process() == PROJECT_STATUS_OK);
    s_fake_tick = 109U;
    TEST_CHECK(Mecanum_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_call_count == 1U);
    s_fake_tick = 110U;
    TEST_CHECK(Mecanum_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(s_call_count == 2U);

    Test_Reset();
    s_velocity_busy = true;
    TEST_CHECK(Mecanum_Drive(1000, 0, 0, 100U, 10U) == PROJECT_STATUS_OK);
    TEST_CHECK(Mecanum_Process() == PROJECT_STATUS_OK);
    s_fake_tick = APP_CONFIG_MECANUM_TRANSACTION_TIMEOUT_MS;
    TEST_CHECK(Mecanum_Process() == PROJECT_STATUS_OK);
    TEST_CHECK(Mecanum_HasFault() && !Mecanum_IsBusy());
    TEST_CHECK(s_calls[s_call_count - 1U].type == TEST_CALL_STOP);
    return 0;
}

int main(void)
{
    if ((Test_Kinematics() != 0) || (Test_SynchronizedSequence() != 0) ||
        (Test_ResponseTimeout() != 0) || (Test_ErrorResponseStopsAndRetriesFullQueue() != 0) ||
        (Test_BusyAndMalformedResponse() != 0) || (Test_TxDrainAndTransactionDeadline() != 0))
    {
        return 1;
    }

    puts("mecanum tests passed");
    return 0;
}
