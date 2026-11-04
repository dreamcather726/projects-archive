/**
 * @file freertos_hooks.c
 * @brief FreeRTOS 静态内存回调和调试故障钩子。
 */

#include "FreeRTOS.h"
#include "app.h"
#include "task.h"

/* Idle 任务也需要一个 TCB 和栈；静态分配时由应用提供。 */
static StaticTask_t s_idle_task_control_block;
static StackType_t s_idle_task_stack[configMINIMAL_STACK_SIZE];

/* 保留最近一次内核断言位置，即使编译器优化后也能在调试器中查看。 */
static const char *volatile s_assert_file;
static volatile uint32_t s_assert_line;

void vApplicationGetIdleTaskMemory(StaticTask_t **task_control_block, StackType_t **stack_buffer,
                                   configSTACK_DEPTH_TYPE *stack_size)
{
    *task_control_block = &s_idle_task_control_block;
    *stack_buffer = s_idle_task_stack;
    *stack_size = configMINIMAL_STACK_SIZE;
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *task_name)
{
    (void)task;
    (void)task_name;
    App_EnterFault(PROJECT_STATUS_ERROR);
}

void vApplicationAssertHook(const char *file, uint32_t line)
{
    s_assert_file = file;
    s_assert_line = line;
    App_EnterFault(PROJECT_STATUS_ERROR);
}
