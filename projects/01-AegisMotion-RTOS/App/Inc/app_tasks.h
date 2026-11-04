/**
 * @file app_tasks.h
 * @brief 应用任务创建和 FreeRTOS 调度器启动接口。
 */

#ifndef APP_TASKS_H_
#define APP_TASKS_H_

#include "project_status.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief 静态创建应用任务并启动 FreeRTOS 调度器。
     * @return 正常情况下不会返回；创建失败或调度器异常退出时返回错误。
     * @note 调用前必须先成功执行 App_Init()。
     */
    Project_Status_t AppTasks_StartScheduler(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_TASKS_H_ */
