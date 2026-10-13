#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "Emm_V5.h"
#include "FreeRTOS.h"
#include "bsp_uart.h"
#include "host_protocol.h"
#include "mecanum_chassis.h"

static TickType_t s_tick;
static uint8_t s_motor_rx[4];
static uint32_t s_motor_index = 4U;
static uint8_t s_probe;
static uint32_t s_stops;
static uint32_t s_velocities;
static uint32_t s_syncs;

#define CHECK(condition) do { if (!(condition)) { \
    printf("integration FAIL line %u: %s\n", (unsigned)__LINE__, #condition); return 1; \
} } while (0)

TickType_t xTaskGetTickCount(void) { return s_tick; }
bool BSP_UART_IsTxIdle(BSP_UartPort_t port) { (void)port; return true; }
bool BSP_UART_TryReadByte(BSP_UartPort_t port, uint8_t *data)
{
    if (port != BSP_UART_PORT_MOTOR || s_motor_index >= 4U) { return false; }
    *data = s_motor_rx[s_motor_index++];
    return true;
}
void BSP_UART_DiscardRxData(BSP_UartPort_t port) { (void)port; s_motor_index = 4U; }
Project_Status_t BSP_UART_Write(BSP_UartPort_t port, const uint8_t *data, uint32_t length)
{ (void)port; (void)data; (void)length; return PROJECT_STATUS_OK; }
uint32_t BSP_UART_GetDroppedByteCount(BSP_UartPort_t port) { (void)port; return 0U; }
uint32_t BSP_UART_GetDmaErrorCount(BSP_UartPort_t port) { (void)port; return 0U; }
Project_Status_t Emm_V5_Read_Sys_Params(uint8_t address, SysParams_t item)
{ (void)item; s_probe = address; return PROJECT_STATUS_OK; }
Project_Status_t Emm_V5_Stop_Now(uint8_t address, bool synchronous)
{ if (address == 0U && !synchronous) { ++s_stops; } return PROJECT_STATUS_OK; }
Project_Status_t Emm_V5_Vel_Control(uint8_t address, uint8_t direction, uint16_t rpm,
                                    uint8_t acceleration, bool synchronous)
{ (void)address; (void)direction; (void)rpm; (void)acceleration; (void)synchronous;
  ++s_velocities; return PROJECT_STATUS_OK; }
Project_Status_t Emm_V5_Synchronous_motion(uint8_t address)
{ (void)address; ++s_syncs; return PROJECT_STATUS_OK; }
Project_Status_t Emm_V5_En_Control(uint8_t address, bool enable, bool synchronous)
{ (void)address; (void)enable; (void)synchronous; return PROJECT_STATUS_OK; }
Project_Status_t Emm_V5_Pos_Control(uint8_t address, uint8_t direction, uint16_t rpm,
    uint8_t acceleration, uint32_t pulses, bool absolute, bool synchronous)
{ (void)address; (void)direction; (void)rpm; (void)acceleration; (void)pulses;
  (void)absolute; (void)synchronous; return PROJECT_STATUS_OK; }
Project_Status_t Emm_V5_Reset_CurPos_To_Zero(uint8_t address)
{ (void)address; return PROJECT_STATUS_OK; }
Project_Status_t Emm_V5_Origin_Trigger_Return(uint8_t address, uint8_t mode, bool synchronous)
{ (void)address; (void)mode; (void)synchronous; return PROJECT_STATUS_OK; }

static void Reply(uint8_t address, uint8_t function, uint8_t status)
{
    s_motor_rx[0] = address; s_motor_rx[1] = function;
    s_motor_rx[2] = status; s_motor_rx[3] = 0x6BU; s_motor_index = 0U;
}

int main(void)
{
    uint32_t index;
    CHECK(HostProtocol_Init() == PROJECT_STATUS_OK);
    CHECK(s_stops == 1U);
    CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    for (index = 0U; index < 4U; ++index)
    {
        Reply(s_probe, 0x3AU, 0U);
        CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    }
    CHECK(Mecanum_Drive(1000, 0, 0, 100U, 10U) == PROJECT_STATUS_OK);
    CHECK(Mecanum_Process() == PROJECT_STATUS_OK);
    s_tick = 10U;
    CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    CHECK(Mecanum_Process() == PROJECT_STATUS_OK);
    s_tick = 20U;
    CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    CHECK(Mecanum_Process() == PROJECT_STATUS_OK);
    CHECK(s_velocities == 2U && s_syncs == 0U && s_stops >= 2U);
    CHECK(Mecanum_HasFault());
    CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    CHECK(Mecanum_Drive(1000, 0, 0, 100U, 10U) == PROJECT_STATUS_ERROR);
    Reply(2U, 0xF6U, 2U); /* A late reply cannot restart the failed command. */
    CHECK(HostProtocol_Process() == PROJECT_STATUS_OK);
    CHECK(Mecanum_Process() == PROJECT_STATUS_OK);
    CHECK(s_syncs == 0U && !Mecanum_IsBusy());
    puts("safety integration tests passed");
    return 0;
}
