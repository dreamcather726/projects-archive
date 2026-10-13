/**
 * @file main.c
 * @brief 程序入口与顶层故障处理。
 */

#include "app.h"
#include "app_tasks.h"
#include "stm32f407xx.h"

int main(void)
{
    Project_Status_t status;

    SystemCoreClockUpdate();

    /* FreeRTOS 要求所有已实现的优先级位都用于抢占优先级，不使用子优先级。 */
    NVIC_SetPriorityGrouping(0U);

    status = App_Init();
    if (status != PROJECT_STATUS_OK)
    {
        App_EnterFault(status);
    }

    status = AppTasks_StartScheduler();
    if (status != PROJECT_STATUS_OK)
    {
        App_EnterFault(status);
    }

    /* 防御性兜底：调度器不应该正常返回。 */
    for (;;)
    {
        __NOP();
    }
}
