/**
 * @file bsp.c
 * @brief 开发板级硬件的统一初始化入口。
 */

#include "bsp.h"

#include <stdbool.h>

#include "bsp_uart.h"
#include "stm32f407xx.h"

#define BSP_IWDG_LSI_FREQUENCY_HZ (32000UL)
#define BSP_IWDG_MAX_RELOAD (0x0FFFUL)
#define BSP_IWDG_WRITE_ACCESS_KEY (0x5555U)
#define BSP_IWDG_RELOAD_KEY (0xAAAAU)
#define BSP_IWDG_START_KEY (0xCCCCU)

static bool s_watchdog_started;

Project_Status_t BSP_Init(void)
{
    /* 后续增加定时器、I2C、SPI 等板级资源时，也在这里按依赖顺序初始化。 */
    return BSP_UART_Init();
}

Project_Status_t BSP_Watchdog_Init(uint32_t timeout_ms)
{
    static const uint16_t prescaler_divisors[] = {4U, 8U, 16U, 32U, 64U, 128U, 256U};
    uint32_t reload;
    uint32_t prescaler_index;

    if ((timeout_ms < 100U) || (timeout_ms > 32000U))
    {
        return PROJECT_STATUS_INVALID_ARGUMENT;
    }
    if (s_watchdog_started)
    {
        return PROJECT_STATUS_OK;
    }

    for (prescaler_index = 0U;
         prescaler_index < (sizeof(prescaler_divisors) / sizeof(prescaler_divisors[0]));
         ++prescaler_index)
    {
        /* 向上取整，避免量化后的标称超时时间短于配置值。 */
        reload = ((timeout_ms * (BSP_IWDG_LSI_FREQUENCY_HZ / 1000U)) +
                  prescaler_divisors[prescaler_index] - 1U) /
                 prescaler_divisors[prescaler_index];
        if ((reload > 0U) && (reload <= (BSP_IWDG_MAX_RELOAD + 1U)))
        {
            --reload;
            break;
        }
    }

    if (prescaler_index >= (sizeof(prescaler_divisors) / sizeof(prescaler_divisors[0])))
    {
        return PROJECT_STATUS_INVALID_ARGUMENT;
    }

    /* 调试暂停时冻结 IWDG，避免单步调试导致意外复位。 */
    DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_IWDG_STOP;

    IWDG->KR = BSP_IWDG_WRITE_ACCESS_KEY;
    IWDG->PR = prescaler_index;
    IWDG->RLR = reload;
    IWDG->KR = BSP_IWDG_RELOAD_KEY;
    IWDG->KR = BSP_IWDG_START_KEY;
    s_watchdog_started = true;
    return PROJECT_STATUS_OK;
}

void BSP_Watchdog_Refresh(void)
{
    if (s_watchdog_started)
    {
        IWDG->KR = BSP_IWDG_RELOAD_KEY;
    }
}
