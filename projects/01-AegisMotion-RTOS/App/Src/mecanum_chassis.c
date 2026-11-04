/**
 * @file mecanum_chassis.c
 * @brief X 型麦克纳姆轮逆运动学与 Emm_V5 四轮同步控制。
 * @note 状态只由Control任务访问；上位机协议负责门禁/授权，本模块负责四轮事务。
 */

#include "mecanum_chassis.h"

#include <stddef.h>

#include "Emm_V5.h"
#include "FreeRTOS.h"
#include "app_config.h"
#include "bsp_uart.h"
#include "task.h"

#define MECANUM_COMMAND_LIMIT (1000)
#define MECANUM_MOTOR_COUNT (4U)
#define MECANUM_RESPONSE_TIMEOUT_MS (10U)
#define MECANUM_MAX_RETRY_COUNT (1U)
#define MECANUM_RESPONSE_FRAME_LENGTH (4U)
#define MECANUM_VELOCITY_FUNCTION_CODE (0xF6U)
#define MECANUM_STOP_FUNCTION_CODE (0xFEU)
#define MECANUM_COMMAND_SUCCESS (0x02U)
#define MECANUM_INVALID_FUNCTION_CODE (0x00U)
#define MECANUM_INVALID_COMMAND_STATUS (0xEEU)

static const uint8_t s_addresses[MECANUM_MOTOR_COUNT] = {
    APP_CONFIG_MECANUM_LEFT_FRONT_ADDRESS,
    APP_CONFIG_MECANUM_RIGHT_FRONT_ADDRESS,
    APP_CONFIG_MECANUM_LEFT_REAR_ADDRESS,
    APP_CONFIG_MECANUM_RIGHT_REAR_ADDRESS,
};
static const uint8_t s_forward_directions[MECANUM_MOTOR_COUNT] = {
    APP_CONFIG_MECANUM_LEFT_FRONT_FORWARD_DIR,
    APP_CONFIG_MECANUM_RIGHT_FRONT_FORWARD_DIR,
    APP_CONFIG_MECANUM_LEFT_REAR_FORWARD_DIR,
    APP_CONFIG_MECANUM_RIGHT_REAR_FORWARD_DIR,
};

/* 按左前/右前/左后/右后保存当前事务目标，四轮应答后才入队广播同步帧。 */
static uint16_t s_target_rpm[MECANUM_MOTOR_COUNT];
static uint8_t s_target_directions[MECANUM_MOTOR_COUNT];
static uint8_t s_target_acceleration;
static uint8_t s_next_motor_index;
static uint8_t s_retry_count;
static bool s_motion_active;
static bool s_waiting_response;
static uint8_t s_expected_function_code;
static TickType_t s_response_deadline; /* 本轮发送完成后的10ms回复期限。 */
static TickType_t s_transaction_start; /* 整个事务200ms期限，包含排队与重试。 */
static bool s_waiting_tx_complete;     /* 排队成功不等于TC完成，不能立即计回复超时。 */
static bool s_fault_latched;
static bool s_fault_stop_pending;
static uint32_t s_fault_count;
static uint8_t s_fault_address;
static uint8_t s_result;

static uint32_t Mecanum_Absolute(int32_t value);
static int16_t Mecanum_ScaleWheelRpm(int32_t value, uint32_t divisor, uint16_t maximum_rpm);
static uint8_t Mecanum_GetDriverDirection(int16_t wheel_rpm, uint8_t forward_direction);
static void Mecanum_RetryOrCancel(void);
static void Mecanum_FailStop(void);

void Mecanum_Init(void)
{
    Mecanum_Cancel();
    s_fault_latched = false;
    s_fault_stop_pending = false;
    s_fault_count = 0U;
    s_fault_address = 0U;
    s_result = MECANUM_RESULT_IDLE;
}

bool Mecanum_HasFault(void) { return s_fault_latched; }
bool Mecanum_StopPending(void) { return s_fault_stop_pending; }
uint32_t Mecanum_GetFaultCount(void) { return s_fault_count; }
uint8_t Mecanum_GetFaultAddress(void) { return s_fault_address; }
uint8_t Mecanum_GetResult(void) { return s_result; }
void Mecanum_ClearFault(void)
{
    if (!s_fault_stop_pending && !s_motion_active)
    {
        s_fault_latched = false;
    }
}

Project_Status_t Mecanum_CalculateWheelRpm(int16_t forward, int16_t left, int16_t counterclockwise,
                                           uint16_t maximum_rpm, Mecanum_WheelRpm_t *wheel_rpm)
{
    int32_t raw_rpm[MECANUM_MOTOR_COUNT];
    uint32_t divisor = MECANUM_COMMAND_LIMIT;
    uint32_t index;

    if ((wheel_rpm == NULL) || (forward < -MECANUM_COMMAND_LIMIT) ||
        (forward > MECANUM_COMMAND_LIMIT) || (left < -MECANUM_COMMAND_LIMIT) ||
        (left > MECANUM_COMMAND_LIMIT) || (counterclockwise < -MECANUM_COMMAND_LIMIT) ||
        (counterclockwise > MECANUM_COMMAND_LIMIT) ||
        (maximum_rpm > APP_CONFIG_MOTOR_MAX_RPM))
    {
        return PROJECT_STATUS_INVALID_ARGUMENT;
    }

    /* X 型轮组：车体坐标 X 向前、Y 向左、Z 逆时针为正。 */
    raw_rpm[0] = (int32_t)forward - (int32_t)left - (int32_t)counterclockwise;
    raw_rpm[1] = (int32_t)forward + (int32_t)left + (int32_t)counterclockwise;
    raw_rpm[2] = (int32_t)forward + (int32_t)left - (int32_t)counterclockwise;
    raw_rpm[3] = (int32_t)forward - (int32_t)left + (int32_t)counterclockwise;

    /* 统一归一化，避免某一轮饱和后改变合成运动方向。 */
    for (index = 0U; index < MECANUM_MOTOR_COUNT; ++index)
    {
        uint32_t magnitude = Mecanum_Absolute(raw_rpm[index]);

        if (magnitude > divisor)
        {
            divisor = magnitude;
        }
    }

    wheel_rpm->left_front_rpm = Mecanum_ScaleWheelRpm(raw_rpm[0], divisor, maximum_rpm);
    wheel_rpm->right_front_rpm = Mecanum_ScaleWheelRpm(raw_rpm[1], divisor, maximum_rpm);
    wheel_rpm->left_rear_rpm = Mecanum_ScaleWheelRpm(raw_rpm[2], divisor, maximum_rpm);
    wheel_rpm->right_rear_rpm = Mecanum_ScaleWheelRpm(raw_rpm[3], divisor, maximum_rpm);

    return PROJECT_STATUS_OK;
}

Project_Status_t Mecanum_Drive(int16_t forward, int16_t left, int16_t counterclockwise,
                               uint16_t maximum_rpm, uint8_t acceleration)
{
    Mecanum_WheelRpm_t calculated;
    int16_t signed_rpm[MECANUM_MOTOR_COUNT];
    uint32_t index;
    Project_Status_t status;

    status = Mecanum_CalculateWheelRpm(forward, left, counterclockwise, maximum_rpm, &calculated);
    if (s_fault_latched || s_fault_stop_pending || s_motion_active)
    {
        return PROJECT_STATUS_ERROR;
    }
    if (status != PROJECT_STATUS_OK)
    {
        return status;
    }

    signed_rpm[0] = calculated.left_front_rpm;
    signed_rpm[1] = calculated.right_front_rpm;
    signed_rpm[2] = calculated.left_rear_rpm;
    signed_rpm[3] = calculated.right_rear_rpm;

    for (index = 0U; index < MECANUM_MOTOR_COUNT; ++index)
    {
        s_target_directions[index] =
            Mecanum_GetDriverDirection(signed_rpm[index], s_forward_directions[index]);
        s_target_rpm[index] = (uint16_t)Mecanum_Absolute(signed_rpm[index]);
    }

    /*
     * 不把多条帧连续塞入 USART。每次只发一个地址，只有收到成功回复才进入
     * 下一台；超时重试一次仍失败则广播停车并锁定，四台确认后才同步触发。
     */
    s_target_acceleration = acceleration;
    s_next_motor_index = 0U;
    s_retry_count = 0U;
    s_waiting_response = false;
    s_motion_active = true;
    s_transaction_start = xTaskGetTickCount();
    s_waiting_tx_complete = false;
    s_result = MECANUM_RESULT_PENDING;
    return PROJECT_STATUS_OK;
}

Project_Status_t Mecanum_Process(void)
{
    Project_Status_t status;
    TickType_t now;

    if (s_fault_stop_pending)
    {
        status = Emm_V5_Stop_Now(0U, false);
        if (status == PROJECT_STATUS_OK)
        {
            s_fault_stop_pending = false;
        }
        /* 停车失败仍保留请求，不能因退出控制任务而失去重试机会。 */
        return PROJECT_STATUS_OK;
    }
    if (!s_motion_active)
    {
        return PROJECT_STATUS_OK;
    }

    now = xTaskGetTickCount();
    if ((TickType_t)(now - s_transaction_start) >=
        pdMS_TO_TICKS(APP_CONFIG_MECANUM_TRANSACTION_TIMEOUT_MS))
    {
        Mecanum_FailStop();
        return PROJECT_STATUS_OK;
    }
    if (s_waiting_response)
    {
        if (s_waiting_tx_complete)
        {
            if (!BSP_UART_IsTxIdle(BSP_UART_PORT_MOTOR)) { return PROJECT_STATUS_OK; }
            s_waiting_tx_complete = false;
            s_response_deadline = now + pdMS_TO_TICKS(MECANUM_RESPONSE_TIMEOUT_MS);
        }
        if ((int32_t)(now - s_response_deadline) < 0)
        {
            return PROJECT_STATUS_OK;
        }

        Mecanum_RetryOrCancel();
        if (!s_motion_active)
        {
            return PROJECT_STATUS_OK;
        }
    }

    if (s_next_motor_index < MECANUM_MOTOR_COUNT)
    {
        if (s_target_rpm[s_next_motor_index] == 0U)
        {
            /* 斜移时部分车轮目标为零，使用同步停止帧避免部分固件拒绝 0 RPM。 */
            status = Emm_V5_Stop_Now(s_addresses[s_next_motor_index], true);
            s_expected_function_code = MECANUM_STOP_FUNCTION_CODE;
        }
        else
        {
            status = Emm_V5_Vel_Control(
                s_addresses[s_next_motor_index], s_target_directions[s_next_motor_index],
                s_target_rpm[s_next_motor_index], s_target_acceleration, true);
            s_expected_function_code = MECANUM_VELOCITY_FUNCTION_CODE;
        }
        if (status == PROJECT_STATUS_ERROR)
        {
            /* USART6 发送队列暂满，保留当前索引并在下个控制周期重试。 */
            return PROJECT_STATUS_OK;
        }
        if (status != PROJECT_STATUS_OK)
        {
            Mecanum_FailStop();
            return PROJECT_STATUS_OK;
        }

        s_waiting_response = true;
        s_waiting_tx_complete = !BSP_UART_IsTxIdle(BSP_UART_PORT_MOTOR);
        s_response_deadline = now + pdMS_TO_TICKS(MECANUM_RESPONSE_TIMEOUT_MS);
        return PROJECT_STATUS_OK;
    }

    status = Emm_V5_Synchronous_motion(0U);
    if (status == PROJECT_STATUS_ERROR)
    {
        return PROJECT_STATUS_OK;
    }
    if (status != PROJECT_STATUS_OK)
    {
        Mecanum_FailStop();
        return PROJECT_STATUS_OK;
    }

    Mecanum_Cancel();
    s_result = MECANUM_RESULT_SYNC_SENT;
    return PROJECT_STATUS_OK;
}

void Mecanum_HandleMotorResponse(const uint8_t *frame, uint32_t length)
{
    if (!s_motion_active || !s_waiting_response || (frame == NULL) ||
        (length != MECANUM_RESPONSE_FRAME_LENGTH) || (frame[3] != 0x6BU) ||
        (frame[0] != s_addresses[s_next_motor_index]))
    {
        return;
    }

    if ((frame[1] == s_expected_function_code) && (frame[2] == MECANUM_COMMAND_SUCCESS))
    {
        s_waiting_response = false;
        ++s_next_motor_index;
        s_retry_count = 0U;
        return;
    }

    /* 错误命令帧可能由瞬时总线冲突造成，仅允许原地址重试一次。 */
    if ((frame[1] == MECANUM_INVALID_FUNCTION_CODE) && (frame[2] == MECANUM_INVALID_COMMAND_STATUS))
    {
        Mecanum_RetryOrCancel();
        return;
    }

    /* 未使能或保护等错误必须停止全车，不只是取消本轮的软件流程。 */
    if (frame[1] == s_expected_function_code)
    {
        Mecanum_FailStop();
    }
}

void Mecanum_Cancel(void)
{
    if (s_motion_active)
    {
        s_result = MECANUM_RESULT_CANCELLED;
    }
    s_motion_active = false;
    s_waiting_response = false;
    s_waiting_tx_complete = false;
    s_next_motor_index = 0U;
    s_retry_count = 0U;
}

bool Mecanum_IsBusy(void) { return s_motion_active; }

static uint32_t Mecanum_Absolute(int32_t value)
{
    return (value < 0) ? (uint32_t)(-value) : (uint32_t)value;
}

static int16_t Mecanum_ScaleWheelRpm(int32_t value, uint32_t divisor, uint16_t maximum_rpm)
{
    uint32_t magnitude = Mecanum_Absolute(value);
    uint32_t scaled = ((magnitude * maximum_rpm) + (divisor / 2U)) / divisor;

    return (value < 0) ? (int16_t)(-(int32_t)scaled) : (int16_t)scaled;
}

static uint8_t Mecanum_GetDriverDirection(int16_t wheel_rpm, uint8_t forward_direction)
{
    if (wheel_rpm < 0)
    {
        return (uint8_t)(forward_direction ^ 1U);
    }
    return forward_direction;
}

static void Mecanum_RetryOrCancel(void)
{
    s_waiting_response = false;
    if (s_retry_count < MECANUM_MAX_RETRY_COUNT)
    {
        ++s_retry_count;
        return;
    }

    Mecanum_FailStop();
}

static void Mecanum_FailStop(void)
{
    /* 先保存故障地址再Cancel（Cancel会重置索引）；停车入队失败则周期重试。 */
    s_fault_address = s_next_motor_index < MECANUM_MOTOR_COUNT ?
                          s_addresses[s_next_motor_index] : 0U;
    Mecanum_Cancel();
    s_result = MECANUM_RESULT_FAILED;
    s_fault_latched = true;
    ++s_fault_count;
    s_fault_stop_pending = Emm_V5_Stop_Now(0U, false) != PROJECT_STATUS_OK;
}
