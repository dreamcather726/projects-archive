/*
  MPU6050_Sensor.h
  Version: v1.1.1-Wire-Compatible

  说明：
  - 保留新版自封库的函数名称和 MPU6050_Data 数据结构
  - SDA 默认使用 GPIO8，SCL 默认使用 GPIO9
  - 底层统一使用 Arduino Wire，便于 MPU6050 和 OLED 共用一条 I2C 总线
  - 本库只负责 MPU6050，不包含任何 OLED 函数
*/

#ifndef MPU6050_SENSOR_H
#define MPU6050_SENSOR_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MPU6050_LIBRARY_VERSION "1.1.1-Wire-Compatible"

#define MPU6050_DEFAULT_ADDRESS 0x68
#define MPU6050_ALTERNATE_ADDRESS 0x69

#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_REG_PWR_MGMT_1 0x6B
#define MPU6050_REG_WHO_AM_I 0x75

typedef struct {
  int16_t accel_x;
  int16_t accel_y;
  int16_t accel_z;

  int16_t temperature_raw;
  float temperature_c;

  int16_t gyro_x;
  int16_t gyro_y;
  int16_t gyro_z;
} MPU6050_Data;

bool mpu6050_init(  // init 是 initialize（初始化）的缩写；这是自封装 MPU6050_Sensor 库函数，用于启动 I2C、检测设备、读取身份并唤醒 MPU6050
  int sda_pin,
  int scl_pin,
  uint8_t device_address,
  uint32_t clock_hz,
  uint8_t *who_am_i
);

void mpu6050_deinit(void);  // deinit 是 deinitialize（取消初始化）的缩写；这是自封装 MPU6050_Sensor 库函数，用于清除本库的初始化状态但不关闭共享 I2C 总线

bool mpu6050_probe(void);  // probe 是“探测”；这是自封装 MPU6050_Sensor 库函数，用于向 MPU6050 地址发送请求并检查设备是否返回 ACK
bool mpu6050_wake(void);  // wake 是“唤醒”；这是自封装 MPU6050_Sensor 库函数，用于写入电源管理寄存器，使 MPU6050 退出睡眠状态
bool mpu6050_read_who_am_i(uint8_t *value);  // read 是“读取”，WHO_AM_I 是 Who Am I（我是谁）身份寄存器；这是自封装 MPU6050_Sensor 库函数，用于读取芯片身份值

bool mpu6050_write_register(  // write register 是“写入寄存器”；这是自封装 MPU6050_Sensor 库函数，用于向 MPU6050 的指定寄存器写入一个字节
  uint8_t register_address,
  uint8_t value
);

bool mpu6050_read_register(  // read register 是“读取寄存器”；这是自封装 MPU6050_Sensor 库函数，用于读取 MPU6050 指定寄存器中的一个字节
  uint8_t register_address,
  uint8_t *value
);

bool mpu6050_read_bytes(  // read bytes 是“读取多个字节”；这是自封装 MPU6050_Sensor 库函数，用于从指定起始寄存器连续读取一组数据
  uint8_t start_register,
  uint8_t *buffer,
  uint8_t length
);

bool mpu6050_read_data(MPU6050_Data *data);  // read data 是“读取数据”；这是自封装 MPU6050_Sensor 库函数，用于读取并解析加速度、温度和陀螺仪的 14 字节数据

int16_t mpu6050_combine_bytes(  // combine bytes 是“合并字节”；这是自封装 MPU6050_Sensor 库函数，用于把高字节和低字节组合成一个 16 位有符号数
  uint8_t high_byte,
  uint8_t low_byte
);

#ifdef __cplusplus
}
#endif

#endif