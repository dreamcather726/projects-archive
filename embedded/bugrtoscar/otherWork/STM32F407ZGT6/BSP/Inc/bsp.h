/**
 * @file bsp.h
 * @brief 开发板级支持包公共接口。
 */

#ifndef BSP_H_
#define BSP_H_

#include <stdint.h>

#include "project_status.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief 初始化开发板使用的全部硬件资源。
     * @return PROJECT_STATUS_OK 表示成功，其他值表示初始化失败原因。
     */
    Project_Status_t BSP_Init(void);

    /**
     * @brief 启动 STM32 独立看门狗。
     * @param timeout_ms 标称复位时间，范围 100~32000 ms。
     * @return 配置成功返回 PROJECT_STATUS_OK。
     * @note IWDG 启动后只能由复位停止；连接调试器时会冻结计数器。
     */
    Project_Status_t BSP_Watchdog_Init(uint32_t timeout_ms);

    /** 刷新已经启动的独立看门狗。未启动时调用不会产生影响。 */
    void BSP_Watchdog_Refresh(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_H_ */
