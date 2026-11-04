/**
 * @file app.h
 * @brief 应用层公共接口。
 */

#ifndef APP_H_
#define APP_H_

#include "project_status.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /** 应用运行状态。 */
    typedef enum
    {
        APP_STATE_UNINITIALIZED = 0,
        APP_STATE_READY,
        APP_STATE_FAULT
    } App_State_t;

    /**
     * @brief 初始化应用及其依赖模块。
     * @return PROJECT_STATUS_OK 表示初始化成功，其他值表示失败原因。
     */
    Project_Status_t App_Init(void);

    /** 执行一次 IMU 解析和文本输出处理，由 IMU 任务周期调用。 */
    Project_Status_t App_ProcessData(void);

    /** 执行一次上位机控制帧和电机回复处理，由控制任务周期调用。 */
    Project_Status_t App_ProcessControl(void);

    /**
     * @brief 获取当前应用状态。
     * @return 当前应用状态。
     */
    App_State_t App_GetState(void);

    /** 获取最近一次致命错误，便于在调试器中观察。 */
    Project_Status_t App_GetLastFault(void);

    /** 记录致命错误、关闭中断并停机，正常情况下不会返回。 */
    void App_EnterFault(Project_Status_t status);

#ifdef __cplusplus
}
#endif

#endif /* APP_H_ */
