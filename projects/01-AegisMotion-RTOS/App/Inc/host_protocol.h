/**
 * @file host_protocol.h
 * @brief 上位机二进制帧协议及 Emm_V5 控制接口。
 */

#ifndef HOST_PROTOCOL_H_
#define HOST_PROTOCOL_H_

#include <stdint.h>

#include "project_status.h"

#ifdef __cplusplus
extern "C"
{
#endif

    enum
    {
        HOST_PROTOCOL_EVENT_READY = 0xE0U,
        HOST_PROTOCOL_EVENT_IMU_TEXT = 0xE1U,
        HOST_PROTOCOL_EVENT_MOTOR_REPLY = 0xE2U,
        HOST_PROTOCOL_EVENT_SELF_TEST = 0xE3U,
        HOST_PROTOCOL_EVENT_CHASSIS_RESULT = 0xE4U
    };

    /** 初始化解析器/队列，先请求全车停车并启动自检；OK不代表自检已经通过。 */
    Project_Status_t HostProtocol_Init(void);

    /** Control任务独占调用：非阻塞处理回复、安全检查、请求与停车重试。 */
    Project_Status_t HostProtocol_Process(void);

    /**
     * @brief 发送无需请求序号的异步事件帧。
     * @param command 事件命令码，建议使用 0xE0~0xEF。
     * @param data 事件数据；length 非零时不能为 NULL。
     * @param length 数据长度，最大 248 字节。
     * @return OK表示完整帧已入队；ERROR表示队列满，不代表已送达上位机。
     * @note 可由Control/Data任务调用；内部保护共享队列，IMU事件只保留最新待发帧。
     */
    Project_Status_t HostProtocol_SendEvent(uint8_t command, const uint8_t *data, uint8_t length);

#ifdef __cplusplus
}
#endif

#endif /* HOST_PROTOCOL_H_ */
