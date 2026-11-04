/**
 * @file project_status.h
 * @brief 工程统一状态码定义。
 */

#ifndef PROJECT_STATUS_H_
#define PROJECT_STATUS_H_

#ifdef __cplusplus
extern "C"
{
#endif

    /** 所有工程模块共用的函数返回状态。 */
    typedef enum
    {
        PROJECT_STATUS_OK = 0,//< 函数执行成功
        PROJECT_STATUS_ERROR,//< 函数执行失败
        PROJECT_STATUS_INVALID_ARGUMENT,//< 函数参数无效
        PROJECT_STATUS_NOT_INITIALIZED,//< 函数调用时模块尚未初始化
        PROJECT_STATUS_NOT_SUPPORTED,//< 函数不支持
        PROJECT_STATUS_TIMEOUT//< 函数执行超时
    } Project_Status_t;

#ifdef __cplusplus
}
#endif

#endif /* PROJECT_STATUS_H_ */
