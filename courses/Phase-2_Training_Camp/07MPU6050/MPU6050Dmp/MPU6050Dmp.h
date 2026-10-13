// 防止头文件被重复包含。Arduino 编译时可能从多个文件引用本头文件，
// 如果没有这组保护宏，函数和类型可能被重复声明。
#ifndef MPU6050_DMP_WRAPPER_H
#define MPU6050_DMP_WRAPPER_H

// 引入 Arduino 常用数据类型，例如 uint8_t、uint32_t。
#include <Arduino.h>

// 头文件只告诉主程序“有哪些函数可以使用”，具体代码写在 .cpp 中。

// 初始化 I2C、MPU6050 和 DMP，并完成静止校准。
// sdaPin、sclPin：I2C 使用的两个 GPIO。
// i2cClock：I2C 时钟频率，默认 400000 Hz，也就是 400 kHz。
// calibrationLoops：校准轮数，默认使用课堂程序中的 6。
// 返回 true 表示连接、DMP 初始化和校准全部完成。
// 返回 false 表示连接失败或 DMP 初始化失败。
// 注意：执行本函数期间，模块必须静止并水平放置。
bool mpuDmpBegin(int sdaPin = 8,
                 int sclPin = 9,
                 uint32_t i2cClock = 400000,
                 uint8_t calibrationLoops = 6);

// 尝试从 FIFO 读取并解算一包新姿态数据。
// 返回 true：Yaw、Pitch、Roll 已经更新，可以调用三个 get 函数。
// 返回 false：本次没有完整数据包，三个角度保留上一次有效结果。
bool mpuDmpUpdate();

// 查询基础 I2C 连接测试是否通过。
bool mpuDmpIsConnected();

// 查询 DMP 是否已经成功开启，可以开始读取姿态数据。
bool mpuDmpIsReady();

// 获取 dmpInitialize() 的状态码，0 表示成功，非 0 表示失败。
uint8_t mpuDmpGetStatus();

// 获取最近一次有效的偏航角，单位为度。
float mpuDmpGetYaw();

// 获取最近一次有效的俯仰角，单位为度。
float mpuDmpGetPitch();

// 获取最近一次有效的横滚角，单位为度。
float mpuDmpGetRoll();

// 通过串口输出当前校准偏移量。
// 调用前必须先执行 Serial.begin()，否则串口监视器无法正常显示。
void mpuDmpPrintOffsets();

// 结束头文件保护范围。
#endif
