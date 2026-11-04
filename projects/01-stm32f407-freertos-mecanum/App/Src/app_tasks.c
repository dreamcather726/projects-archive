/**
 * @file app_tasks.c
 * @brief FreeRTOS 应用任务定义。
 *
 * 这里只负责“何时运行”和“任务之间如何分工”；具体协议仍放在 App、
 * Libraries 和 BSP 中。这样的分层能避免把业务代码写进 RTOS 内核文件。
 */

#include "app_tasks.h"

#include "FreeRTOS.h"
#include "app.h"
#include "app_config.h"
#include "bsp.h"
#include "task.h"

/* StackType_t 在 Cortex-M4 上是 32 位，所以 384 words = 1536 bytes。 */
#define APP_TASK_STACK_DEPTH_WORDS (384U)

/* 当前内核为 1 kHz：数据任务 2 ms 一次，兼顾响应速度和 CPU 空闲时间。 */
#define APP_DATA_TASK_PERIOD_MS (2U)
#define APP_CONTROL_TASK_PERIOD_MS (2U)

#define APP_DATA_TASK_PRIORITY (tskIDLE_PRIORITY + 2U)
#define APP_CONTROL_TASK_PRIORITY (tskIDLE_PRIORITY + 3U)
#define APP_WATCHDOG_TASK_PRIORITY (tskIDLE_PRIORITY + 4U)

#define APP_DATA_TASK_NAME "IMU"
#define APP_WATCHDOG_TASK_NAME "Watchdog"

#if (APP_CONFIG_IMU_ENABLED == 1U)
static StaticTask_t s_data_task_control_block;
static StackType_t s_data_task_stack[APP_TASK_STACK_DEPTH_WORDS];
#endif

#if (APP_CONFIG_HOST_CONTROL_ENABLED == 1U)
static StaticTask_t s_control_task_control_block;
static StackType_t s_control_task_stack[APP_TASK_STACK_DEPTH_WORDS];
#endif

#if (APP_CONFIG_IWDG_ENABLED == 1U)
#define APP_TASK_ALIVE_DATA (1UL << 0U)
#define APP_TASK_ALIVE_CONTROL (1UL << 1U)

static StaticTask_t s_watchdog_task_control_block;
static StackType_t s_watchdog_task_stack[configMINIMAL_STACK_SIZE];
static volatile uint32_t s_task_alive_mask;

#if (APP_CONFIG_IMU_ENABLED == 1U)
#define APP_TASK_EXPECTED_DATA APP_TASK_ALIVE_DATA
#else
#define APP_TASK_EXPECTED_DATA (0UL)
#endif

#if (APP_CONFIG_HOST_CONTROL_ENABLED == 1U)
#define APP_TASK_EXPECTED_CONTROL APP_TASK_ALIVE_CONTROL
#else
#define APP_TASK_EXPECTED_CONTROL (0UL)
#endif

#define APP_TASK_EXPECTED_ALIVE_MASK (APP_TASK_EXPECTED_DATA | APP_TASK_EXPECTED_CONTROL)
#endif

#if (APP_CONFIG_IMU_ENABLED == 1U)
static void AppTasks_DataTask(void *argument);
#endif

#if (APP_CONFIG_HOST_CONTROL_ENABLED == 1U)
static void AppTasks_ControlTask(void *argument);
#endif

#if (APP_CONFIG_IWDG_ENABLED == 1U)
static void AppTasks_WatchdogTask(void *argument);
static void AppTasks_ReportAlive(uint32_t task_mask);
#endif

Project_Status_t AppTasks_StartScheduler(void)
{
#if ((APP_CONFIG_IMU_ENABLED == 1U) || (APP_CONFIG_HOST_CONTROL_ENABLED == 1U) ||               \
     (APP_CONFIG_IWDG_ENABLED == 1U))
    TaskHandle_t task_handle;
#endif
#if (APP_CONFIG_IWDG_ENABLED == 1U)
    Project_Status_t status;
#endif

    if (App_GetState() != APP_STATE_READY)
    {
        return PROJECT_STATUS_NOT_INITIALIZED;
    }

#if (APP_CONFIG_IMU_ENABLED == 1U)
    task_handle =
        xTaskCreateStatic(AppTasks_DataTask, APP_DATA_TASK_NAME, APP_TASK_STACK_DEPTH_WORDS, NULL,
                          APP_DATA_TASK_PRIORITY, s_data_task_stack, &s_data_task_control_block);
    if (task_handle == NULL)
    {
        return PROJECT_STATUS_ERROR;
    }
#endif

#if (APP_CONFIG_HOST_CONTROL_ENABLED == 1U)
    task_handle = xTaskCreateStatic(AppTasks_ControlTask, "Control", APP_TASK_STACK_DEPTH_WORDS,
                                    NULL, APP_CONTROL_TASK_PRIORITY, s_control_task_stack,
                                    &s_control_task_control_block);
    if (task_handle == NULL)
    {
        return PROJECT_STATUS_ERROR;
    }
#endif

#if (APP_CONFIG_IWDG_ENABLED == 1U)
    task_handle = xTaskCreateStatic(
        AppTasks_WatchdogTask, APP_WATCHDOG_TASK_NAME, configMINIMAL_STACK_SIZE, NULL,
        APP_WATCHDOG_TASK_PRIORITY, s_watchdog_task_stack, &s_watchdog_task_control_block);
    if (task_handle == NULL)
    {
        return PROJECT_STATUS_ERROR;
    }

    s_task_alive_mask = 0U;
    status = BSP_Watchdog_Init(APP_CONFIG_IWDG_TIMEOUT_MS);
    if (status != PROJECT_STATUS_OK)
    {
        return status;
    }
#endif

    vTaskStartScheduler();

    /* 调度器正常启动后永远不会走到这里。 */
    return PROJECT_STATUS_ERROR;
}

#if (APP_CONFIG_IMU_ENABLED == 1U)
static void AppTasks_DataTask(void *argument)
{
    TickType_t last_wake_time = xTaskGetTickCount();
    Project_Status_t status;

    (void)argument;

    for (;;)
    {
        status = App_ProcessData();
        if (status != PROJECT_STATUS_OK)
        {
            App_EnterFault(status);
        }

#if (APP_CONFIG_IWDG_ENABLED == 1U)
        AppTasks_ReportAlive(APP_TASK_ALIVE_DATA);
#endif

        /* DelayUntil 使用绝对周期，任务执行时间不会逐圈累积成时间漂移。 */
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(APP_DATA_TASK_PERIOD_MS));
    }
}
#endif

#if (APP_CONFIG_HOST_CONTROL_ENABLED == 1U)
static void AppTasks_ControlTask(void *argument)
{
    TickType_t last_wake_time = xTaskGetTickCount();
    Project_Status_t status;

    (void)argument;

    for (;;)
    {
        status = App_ProcessControl();
        if (status != PROJECT_STATUS_OK)
        {
            App_EnterFault(status);
        }

#if (APP_CONFIG_IWDG_ENABLED == 1U)
        AppTasks_ReportAlive(APP_TASK_ALIVE_CONTROL);
#endif

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(APP_CONTROL_TASK_PERIOD_MS));
    }
}
#endif

#if (APP_CONFIG_IWDG_ENABLED == 1U)
static void AppTasks_WatchdogTask(void *argument)
{
    TickType_t last_wake_time = xTaskGetTickCount();
    uint32_t alive_mask;

    (void)argument;

    for (;;)
    {
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(APP_CONFIG_IWDG_CHECK_PERIOD_MS));

        taskENTER_CRITICAL();
        alive_mask = s_task_alive_mask;
        s_task_alive_mask = 0U;
        taskEXIT_CRITICAL();

        /*
         * 只有所有已启用的应用任务都在本检查周期内运行过，才允许刷新 IWDG。
         * 任一任务卡死后，不再喂狗，由硬件复位恢复系统。
         */
        if ((alive_mask & APP_TASK_EXPECTED_ALIVE_MASK) == APP_TASK_EXPECTED_ALIVE_MASK)
        {
            BSP_Watchdog_Refresh();
        }
    }
}

static void AppTasks_ReportAlive(uint32_t task_mask)
{
    taskENTER_CRITICAL();
    s_task_alive_mask |= task_mask;
    taskEXIT_CRITICAL();
}
#endif
