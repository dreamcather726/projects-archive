/*
  MPU6050_Sensor.h
  Version: v1.0.1

  总体说明：
  - 本文件用于声明 MPU6050 数据结构和可调用函数
  - MPU6050_Sensor.c 负责真正的 ESP32 底层 I2C 通信
  - 主程序只需要包含本头文件，不需要直接操作底层 I2C 命令
  - 默认接线：SDA=GPIO17、SCL=GPIO18、AD0=GND、地址=0x68

  ============================================================
  完整 Arduino 调用示例
  每一行示例代码后面都带有对应说明
  ============================================================

  #include "MPU6050_Sensor.h"                  // 引入本封装，获得数据结构和函数声明

  const int MPU_SDA_PIN = 17;                  // 设置 MPU6050 的 SDA 接 GPIO17
  const int MPU_SCL_PIN = 18;                  // 设置 MPU6050 的 SCL 接 GPIO18

  void setup() {                               // setup() 在开发板启动后只执行一次
    Serial.begin(115200);                      // 启动串口，波特率设为 115200
    delay(1500);                               // 等待串口和 MPU6050 上电稳定

    uint8_t whoAmI = 0;                        // 创建变量保存 WHO_AM_I 身份寄存器结果

    bool success = mpu6050_init(               // 初始化 I2C、检查设备、读取身份并唤醒 MPU6050
      MPU_SDA_PIN,                             // 传入 SDA 引脚编号
      MPU_SCL_PIN,                             // 传入 SCL 引脚编号
      MPU6050_DEFAULT_ADDRESS,                 // 使用默认地址 0x68
      100000,                                  // 设置 I2C 通信速度为 100kHz
      &whoAmI                                  // 把 WHO_AM_I 结果写入 whoAmI 变量
    );                                         // 初始化函数调用结束

    if (!success) {                            // 如果 success 为 false，说明初始化失败
      Serial.println("MPU6050初始化失败");     // 在串口监视器显示错误信息

      while (true) {                           // 进入无限循环，阻止程序继续读取错误数据
        delay(1000);                           // 每次暂停 1 秒，避免空循环占用过多资源
      }                                        // 无限循环结束位置，正常情况下不会离开
    }                                          // 初始化失败判断结束

    Serial.print("WHO_AM_I = 0x");            // 输出身份寄存器提示文字，但暂不换行

    if (whoAmI < 0x10) {                       // 小于 0x10 时，需要在前面补一个 0
      Serial.print("0");                      // 例如把 0x8 显示为 0x08
    }                                          // 补零判断结束

    Serial.println(whoAmI, HEX);               // 按十六进制输出身份值并换行
    Serial.println("MPU6050初始化成功");       // 提示传感器已经可以工作
  }                                            // setup() 结束

  void loop() {                                // loop() 会在 setup() 后不断重复执行
    MPU6050_Data data;                         // 创建结构体，保存一次完整测量结果

    if (!mpu6050_read_data(&data)) {           // 读取 14 字节数据并判断是否成功
      Serial.println("读取MPU6050失败");       // 读取失败时显示提示
      delay(500);                              // 暂停 500 毫秒后再尝试
      return;                                  // 结束本轮 loop()，随后会重新进入 loop()
    }                                          // 读取失败处理结束

    Serial.print("加速度：");                 // 输出加速度标题，不换行
    Serial.print("X=");                       // 输出 X 轴标签
    Serial.print(data.accel_x);                // 输出 X 轴加速度原始值
    Serial.print(" Y=");                      // 输出 Y 轴标签
    Serial.print(data.accel_y);                // 输出 Y 轴加速度原始值
    Serial.print(" Z=");                      // 输出 Z 轴标签
    Serial.print(data.accel_z);                // 输出 Z 轴加速度原始值

    Serial.print(" | 温度=");                 // 输出温度标题
    Serial.print(data.temperature_c, 1);       // 输出摄氏温度，并保留 1 位小数
    Serial.print("℃");                        // 输出摄氏度单位

    Serial.print(" | 陀螺仪：");              // 输出陀螺仪标题
    Serial.print("X=");                       // 输出 X 轴标签
    Serial.print(data.gyro_x);                 // 输出 X 轴陀螺仪原始值
    Serial.print(" Y=");                      // 输出 Y 轴标签
    Serial.print(data.gyro_y);                 // 输出 Y 轴陀螺仪原始值
    Serial.print(" Z=");                      // 输出 Z 轴标签
    Serial.println(data.gyro_z);               // 输出 Z 轴陀螺仪原始值并换行

    delay(500);                                // 每隔 500 毫秒读取并显示一次
  }                                            // loop() 结束后自动重新开始

  ============================================================
  拓展用法示例
  以下内容是注释中的伪代码，不会参与程序编译
  ============================================================

  1. Serial.print() 与 Serial.println()

  Serial.print("A");                          // 输出 A，但输出后不自动换行
  Serial.println("A");                        // 输出 A，并在结尾自动换行
  Serial.print(123);                           // 按十进制输出整数 123
  Serial.println(255, HEX);                    // 按十六进制输出 FF，然后换行
  Serial.print(3.14159, 2);                    // 输出浮点数并保留 2 位小数，结果约为 3.14

  2. delay() 与 delayMicroseconds()

  delay(500);                                  // 暂停 500 毫秒，适合控制读取间隔
  delayMicroseconds(10);                       // 暂停 10 微秒，适合产生很短的硬件时序

  3. 单寄存器读取与连续读取

  uint8_t value = 0;                           // 创建一个字节变量保存寄存器结果
  mpu6050_read_register(0x75, &value);         // 读取 WHO_AM_I 寄存器的 1 个字节

  uint8_t buffer[14];                          // 创建 14 字节数组保存连续数据
  mpu6050_read_bytes(0x3B, buffer, 14);        // 从 0x3B 开始读取 14 个连续字节

  4. 写寄存器的不同用途

  mpu6050_write_register(0x6B, 0x00);          // 唤醒 MPU6050，关闭默认休眠状态
  mpu6050_write_register(0x1C, 0x00);          // 示例：把加速度量程设置为默认范围
  mpu6050_write_register(0x1B, 0x00);          // 示例：把陀螺仪量程设置为默认范围

  5. bool 返回值的不同判断方式

  bool ok = mpu6050_probe();                   // 检查当前地址是否有设备回应
  if (ok) {                                    // ok 为 true 时进入成功分支
    Serial.println("设备在线");               // 输出设备在线提示
  } else {                                     // ok 为 false 时进入失败分支
    Serial.println("设备离线");               // 输出设备离线提示
  }                                            // 判断结束

  if (!mpu6050_read_data(&data)) {             // 使用 ! 直接判断“读取失败”
    Serial.println("读取失败");               // 输出失败提示
  }                                            // 失败判断结束

  6. 原始值与换算值

  Serial.println(data.accel_x);                // 输出 X 轴加速度原始值
  Serial.println(data.temperature_raw);        // 输出温度传感器原始值
  Serial.println(data.temperature_c);          // 输出已经换算好的摄氏温度

  7. 使用另一个 I2C 地址

  mpu6050_init(17, 18, 0x69, 100000, NULL);    // 当 AD0 接高电平时，可以尝试地址 0x69

  8. 不需要 WHO_AM_I 返回值

  mpu6050_init(17, 18, 0x68, 100000, NULL);    // 最后一个参数传 NULL，表示不保存身份值

  注意：
  - .c 文件使用 ESP32 的 I2C_NUM_0 底层驱动
  - 同一组 SDA、SCL 不要再同时调用 Wire.begin()
  - VCC 建议连接 3.3V，GND 必须与 ESP32-S3 共地
  - AD0 接 GND 时通常使用 0x68，接高电平时通常使用 0x69
*/

#ifndef MPU6050_SENSOR_H                         // 如果本头文件尚未被包含，开始定义
#define MPU6050_SENSOR_H                         // 定义头文件保护标记，避免重复声明

#include <stdint.h>                              // 提供 uint8_t、int16_t、uint32_t 等固定宽度类型
#include <stdbool.h>                             // 提供 C 语言中的 bool、true 和 false

#ifdef __cplusplus                               // 如果当前文件由 C++ 编译器处理
extern "C" {                                    // 使用 C 语言链接方式，保证 .ino 能调用 .c 函数
#endif                                           // C++ 条件判断结束

#define MPU6050_LIBRARY_VERSION "1.0.1"          // 当前封装版本号

#define MPU6050_DEFAULT_ADDRESS 0x68              // AD0 接 GND 时常用的 I2C 地址
#define MPU6050_ALTERNATE_ADDRESS 0x69            // AD0 接高电平时常用的 I2C 地址

#define MPU6050_REG_ACCEL_XOUT_H 0x3B             // 加速度 X 轴高字节，也是 14 字节连续数据起点
#define MPU6050_REG_PWR_MGMT_1 0x6B               // 电源管理寄存器，用于唤醒传感器
#define MPU6050_REG_WHO_AM_I 0x75                 // 身份寄存器，用于确认设备是否为 MPU6050

typedef struct {                                  // 定义一次完整测量结果的数据结构
    int16_t accel_x;                              // X 轴加速度原始值
    int16_t accel_y;                              // Y 轴加速度原始值
    int16_t accel_z;                              // Z 轴加速度原始值

    int16_t temperature_raw;                      // 温度传感器原始值
    float temperature_c;                         // 按公式换算后的摄氏温度

    int16_t gyro_x;                               // X 轴陀螺仪原始值
    int16_t gyro_y;                               // Y 轴陀螺仪原始值
    int16_t gyro_z;                               // Z 轴陀螺仪原始值
} MPU6050_Data;                                   // 数据结构类型名称

bool mpu6050_init(                                // 初始化 I2C、检查设备、读取身份并唤醒 MPU6050
    int sda_pin,                                  // SDA 所连接的 GPIO 编号
    int scl_pin,                                  // SCL 所连接的 GPIO 编号
    uint8_t device_address,                       // MPU6050 的 I2C 地址
    uint32_t clock_hz,                            // I2C 时钟频率，推荐 100000
    uint8_t *who_am_i                             // 保存身份值的指针，不需要时可传 NULL
);                                                // 初始化函数声明结束

void mpu6050_deinit(void);                        // 删除本封装安装的 I2C 驱动并释放总线

bool mpu6050_probe(void);                         // 检查当前 MPU6050 地址是否有设备回应

bool mpu6050_wake(void);                          // 向电源管理寄存器写入 0x00，唤醒 MPU6050

bool mpu6050_read_who_am_i(uint8_t *value);       // 读取 WHO_AM_I 身份寄存器

bool mpu6050_write_register(                      // 向指定寄存器写入一个字节
    uint8_t register_address,                     // 目标寄存器地址
    uint8_t value                                 // 要写入寄存器的数据
);                                                // 写寄存器函数声明结束

bool mpu6050_read_register(                       // 从指定寄存器读取一个字节
    uint8_t register_address,                     // 目标寄存器地址
    uint8_t *value                                // 保存读取结果的变量地址
);                                                // 读单个寄存器函数声明结束

bool mpu6050_read_bytes(                          // 从起始寄存器连续读取多个字节
    uint8_t start_register,                       // 连续读取的起始寄存器
    uint8_t *buffer,                              // 保存读取数据的数组地址
    uint8_t length                                // 需要读取的字节数量
);                                                // 连续读取函数声明结束

bool mpu6050_read_data(MPU6050_Data *data);       // 一次读取并解析加速度、温度和陀螺仪数据

int16_t mpu6050_combine_bytes(                    // 把高低两个字节组合为有符号 16 位数据
    uint8_t high_byte,                            // 原始数据的高 8 位
    uint8_t low_byte                              // 原始数据的低 8 位
);                                                // 字节组合函数声明结束

#ifdef __cplusplus                               // 如果前面启用了 C 语言链接方式
}                                                // 结束 extern "C" 代码区域
#endif                                           // C++ 条件判断结束

#endif                                           // MPU6050_SENSOR_H 头文件保护结束
