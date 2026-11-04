/**
 * @file FreeRTOSConfig.h
 * @brief STM32F407ZGT6 的 FreeRTOS 内核配置。
 *
 * 本工程刻意采用静态内存：任务、栈和内核对象都由编译器在启动前分配，
 * 运行期间不使用 malloc。这样 RAM 占用可预测，也更适合第一次学习 RTOS。
 */

#ifndef FREERTOS_CONFIG_H_
#define FREERTOS_CONFIG_H_

#include <stdint.h>

extern uint32_t SystemCoreClock;
void vApplicationAssertHook(const char *file, uint32_t line);

/* 调度器：1 kHz 节拍，即 1 tick = 1 ms；使用抢占式调度。 */
#define configCPU_CLOCK_HZ ((uint32_t)SystemCoreClock)
#define configTICK_RATE_HZ ((TickType_t)1000U)
#define configUSE_PREEMPTION (1)
#define configUSE_TIME_SLICING (1)
#define configUSE_PORT_OPTIMISED_TASK_SELECTION (1)
#define configUSE_TICKLESS_IDLE (0)
#define configMAX_PRIORITIES (5U)
#define configMINIMAL_STACK_SIZE (128U)
#define configMAX_TASK_NAME_LEN (16U)
#define configTICK_TYPE_WIDTH_IN_BITS TICK_TYPE_WIDTH_32_BITS
#define configIDLE_SHOULD_YIELD (1)

/* 只允许静态创建，避免运行期堆分配。 */
#define configSUPPORT_STATIC_ALLOCATION (1)
#define configSUPPORT_DYNAMIC_ALLOCATION (0)
#define configKERNEL_PROVIDED_STATIC_MEMORY (0)

/* 当前会用到的内核功能；队列/信号量已保留给后续扩展。 */
#define configUSE_TASK_NOTIFICATIONS (1)
#define configUSE_MUTEXES (1)
#define configUSE_RECURSIVE_MUTEXES (0)
#define configUSE_COUNTING_SEMAPHORES (1)
#define configUSE_QUEUE_SETS (0)
#define configUSE_TIMERS (0)
#define configUSE_EVENT_GROUPS (1)
#define configUSE_STREAM_BUFFERS (1)
#define configUSE_CO_ROUTINES (0)
#define configMAX_CO_ROUTINE_PRIORITIES (1)

/* 调试保护：检测栈溢出，并在调试器中停在断言位置。 */
#define configCHECK_FOR_STACK_OVERFLOW (2)
#define configUSE_IDLE_HOOK (0)
#define configUSE_TICK_HOOK (0)
#define configUSE_MALLOC_FAILED_HOOK (0)
#define configUSE_DAEMON_TASK_STARTUP_HOOK (0)
#define configUSE_TRACE_FACILITY (0)
#define configUSE_STATS_FORMATTING_FUNCTIONS (0)
#define configGENERATE_RUN_TIME_STATS (0)
#define configQUEUE_REGISTRY_SIZE (4U)
#define configASSERT(x)                                                                            \
    do                                                                                             \
    {                                                                                              \
        if ((x) == 0)                                                                              \
        {                                                                                          \
            vApplicationAssertHook(__FILE__, (uint32_t)__LINE__);                                  \
        }                                                                                          \
    } while (0)

/* Cortex-M4 有 4 个有效优先级位。数值越小，硬件中断优先级越高。 */
#define configPRIO_BITS (4U)
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY (15U)
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY (5U)
#define configKERNEL_INTERRUPT_PRIORITY                                                            \
    (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8U - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY                                                       \
    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8U - configPRIO_BITS))

/* 启动文件中的三个 Cortex-M 异常入口直接交给 FreeRTOS 端口层。 */
#define vPortSVCHandler SVC_Handler
#define xPortPendSVHandler PendSV_Handler
#define xPortSysTickHandler SysTick_Handler
#define configCHECK_HANDLER_INSTALLATION (1)

/* 本项目使用的任务 API。 */
#define INCLUDE_vTaskPrioritySet (0)
#define INCLUDE_uxTaskPriorityGet (0)
#define INCLUDE_vTaskDelete (0)
#define INCLUDE_vTaskSuspend (0)
#define INCLUDE_xTaskDelayUntil (1)
#define INCLUDE_vTaskDelay (1)
#define INCLUDE_xTaskGetSchedulerState (0)
#define INCLUDE_xTaskGetCurrentTaskHandle (1)
#define INCLUDE_uxTaskGetStackHighWaterMark (1)
#define INCLUDE_xTaskGetIdleTaskHandle (0)
#define INCLUDE_eTaskGetState (0)
#define INCLUDE_xTimerPendFunctionCall (0)
#define INCLUDE_xTaskAbortDelay (0)
#define INCLUDE_xTaskGetHandle (0)
#define INCLUDE_xTaskResumeFromISR (0)

#endif /* FREERTOS_CONFIG_H_ */
