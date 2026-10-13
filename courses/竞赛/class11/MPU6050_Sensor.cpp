/*
  MPU6050_Sensor.cpp
  Version: v1.1.1-Wire-Compatible

  说明：
  - 保留用户提供的新库函数接口
  - 底层改为 Arduino Wire
  - OLED 仍由主程序直接调用 Adafruit_SSD1306
  - 本文件没有包含或调用任何 OLED 库
*/

#include "MPU6050_Sensor.h"

#include <Arduino.h>
#include <Wire.h>

static uint8_t g_mpu6050_address =
  MPU6050_DEFAULT_ADDRESS;

static bool g_i2c_initialized = false;

int16_t mpu6050_combine_bytes(  // combine bytes 是“合并字节”；这是自封装 MPU6050_Sensor 库函数，用于把高字节和低字节组合成一个 16 位有符号数
  uint8_t high_byte,
  uint8_t low_byte
) {
  uint16_t value =
    ((uint16_t)high_byte << 8) |
    (uint16_t)low_byte;

  return (int16_t)value;
}

bool mpu6050_probe(void) {  // probe 是“探测”；这是自封装 MPU6050_Sensor 库函数，用于向 MPU6050 地址发送请求并检查设备是否返回 ACK
  if (!g_i2c_initialized)
    return false;

  Wire.beginTransmission(  // begin transmission 是“开始传输”；Arduino Wire 库函数，用于指定目标 I2C 从设备地址并开始准备发送数据
    g_mpu6050_address
  );

  return Wire.endTransmission() == 0;  // end transmission 是“结束传输”；Arduino Wire 库函数，用于真正发送缓冲区数据，并通过返回值判断设备是否 ACK（Acknowledge，应答）
}

bool mpu6050_write_register(  // write register 是“写入寄存器”；这是自封装 MPU6050_Sensor 库函数，用于向 MPU6050 的指定寄存器写入一个字节
  uint8_t register_address,
  uint8_t value
) {
  if (!g_i2c_initialized)
    return false;

  Wire.beginTransmission(  // begin transmission 是“开始传输”；Arduino Wire 库函数，用于指定目标 I2C 从设备地址并开始准备发送数据
    g_mpu6050_address
  );

  Wire.write(register_address);  // write 是“写入”；Arduino Wire 库函数，用于把一个字节或一组字节加入 I2C 发送缓冲区
  Wire.write(value);  // write 是“写入”；Arduino Wire 库函数，用于把一个字节或一组字节加入 I2C 发送缓冲区

  return Wire.endTransmission() == 0;  // end transmission 是“结束传输”；Arduino Wire 库函数，用于真正发送缓冲区数据，并通过返回值判断设备是否 ACK（Acknowledge，应答）
}

bool mpu6050_read_bytes(  // read bytes 是“读取多个字节”；这是自封装 MPU6050_Sensor 库函数，用于从指定起始寄存器连续读取一组数据
  uint8_t start_register,
  uint8_t *buffer,
  uint8_t length
) {
  if (
    !g_i2c_initialized ||
    buffer == NULL ||
    length == 0
  ) {
    return false;
  }

  Wire.beginTransmission(  // begin transmission 是“开始传输”；Arduino Wire 库函数，用于指定目标 I2C 从设备地址并开始准备发送数据
    g_mpu6050_address
  );

  Wire.write(start_register);  // write 是“写入”；Arduino Wire 库函数，用于把一个字节或一组字节加入 I2C 发送缓冲区

  if (Wire.endTransmission(false) != 0)  // end transmission 是“结束传输”；Arduino Wire 库函数，用于真正发送缓冲区数据，并通过返回值判断设备是否 ACK（Acknowledge，应答）
    return false;

  uint8_t received = Wire.requestFrom(  // request from 是“向……请求数据”；Arduino Wire 库函数，用于向指定 I2C 从设备请求一定数量的字节
    g_mpu6050_address,
    length
  );

  if (received != length)
    return false;

  for (uint8_t i = 0; i < length; i++) {
    if (!Wire.available())  // available 是“可用的”；Arduino Wire 库函数，用于返回 I2C 接收缓冲区中当前可读取的字节数
      return false;

    buffer[i] = (uint8_t)Wire.read();  // read 是“读取”；Arduino Wire 库函数，用于从 I2C 接收缓冲区读取一个字节
  }

  return true;
}

bool mpu6050_read_register(  // read register 是“读取寄存器”；这是自封装 MPU6050_Sensor 库函数，用于读取 MPU6050 指定寄存器中的一个字节
  uint8_t register_address,
  uint8_t *value
) {
  if (value == NULL)
    return false;

  return mpu6050_read_bytes(  // read bytes 是“读取多个字节”；这是自封装 MPU6050_Sensor 库函数，用于从指定起始寄存器连续读取一组数据
    register_address,
    value,
    1
  );
}

bool mpu6050_read_who_am_i(  // read 是“读取”，WHO_AM_I 是 Who Am I（我是谁）身份寄存器；这是自封装 MPU6050_Sensor 库函数，用于读取芯片身份值
  uint8_t *value
) {
  return mpu6050_read_register(  // read register 是“读取寄存器”；这是自封装 MPU6050_Sensor 库函数，用于读取 MPU6050 指定寄存器中的一个字节
    MPU6050_REG_WHO_AM_I,
    value
  );
}

bool mpu6050_wake(void) {  // wake 是“唤醒”；这是自封装 MPU6050_Sensor 库函数，用于写入电源管理寄存器，使 MPU6050 退出睡眠状态
  return mpu6050_write_register(  // write register 是“写入寄存器”；这是自封装 MPU6050_Sensor 库函数，用于向 MPU6050 的指定寄存器写入一个字节
    MPU6050_REG_PWR_MGMT_1,
    0x00
  );
}

bool mpu6050_read_data(  // read data 是“读取数据”；这是自封装 MPU6050_Sensor 库函数，用于读取并解析加速度、温度和陀螺仪的 14 字节数据
  MPU6050_Data *data
) {
  if (data == NULL)
    return false;

  uint8_t raw_data[14];

  if (!mpu6050_read_bytes(  // read bytes 是“读取多个字节”；这是自封装 MPU6050_Sensor 库函数，用于从指定起始寄存器连续读取一组数据
        MPU6050_REG_ACCEL_XOUT_H,
        raw_data,
        sizeof(raw_data)  // size of 是“……的大小”；这是 C/C++ 运算符，用于取得变量、类型或数组占用的字节数
      )) {
    return false;
  }

  data->accel_x =
    mpu6050_combine_bytes(  // combine bytes 是“合并字节”；这是自封装 MPU6050_Sensor 库函数，用于把高字节和低字节组合成一个 16 位有符号数
      raw_data[0],
      raw_data[1]
    );

  data->accel_y =
    mpu6050_combine_bytes(  // combine bytes 是“合并字节”；这是自封装 MPU6050_Sensor 库函数，用于把高字节和低字节组合成一个 16 位有符号数
      raw_data[2],
      raw_data[3]
    );

  data->accel_z =
    mpu6050_combine_bytes(  // combine bytes 是“合并字节”；这是自封装 MPU6050_Sensor 库函数，用于把高字节和低字节组合成一个 16 位有符号数
      raw_data[4],
      raw_data[5]
    );

  data->temperature_raw =
    mpu6050_combine_bytes(  // combine bytes 是“合并字节”；这是自封装 MPU6050_Sensor 库函数，用于把高字节和低字节组合成一个 16 位有符号数
      raw_data[6],
      raw_data[7]
    );

  data->gyro_x =
    mpu6050_combine_bytes(  // combine bytes 是“合并字节”；这是自封装 MPU6050_Sensor 库函数，用于把高字节和低字节组合成一个 16 位有符号数
      raw_data[8],
      raw_data[9]
    );

  data->gyro_y =
    mpu6050_combine_bytes(  // combine bytes 是“合并字节”；这是自封装 MPU6050_Sensor 库函数，用于把高字节和低字节组合成一个 16 位有符号数
      raw_data[10],
      raw_data[11]
    );

  data->gyro_z =
    mpu6050_combine_bytes(  // combine bytes 是“合并字节”；这是自封装 MPU6050_Sensor 库函数，用于把高字节和低字节组合成一个 16 位有符号数
      raw_data[12],
      raw_data[13]
    );

  data->temperature_c =
    data->temperature_raw /
    340.0F +
    36.53F;

  return true;
}

bool mpu6050_init(  // init 是 initialize（初始化）的缩写；这是自封装 MPU6050_Sensor 库函数，用于启动 I2C、检测设备、读取身份并唤醒 MPU6050
  int sda_pin,
  int scl_pin,
  uint8_t device_address,
  uint32_t clock_hz,
  uint8_t *who_am_i
) {
  if (
    sda_pin < 0 ||
    scl_pin < 0 ||
    clock_hz == 0 ||
    device_address < 0x03 ||
    device_address > 0x77
  ) {
    return false;
  }

  if (!Wire.begin(  // Wire 表示 I2C 连线通信，begin 是“开始”；Arduino Wire 库函数，用于启动 I2C（Inter-Integrated Circuit，集成电路总线）并指定 SDA、SCL 引脚
        sda_pin,
        scl_pin
      )) {
    g_i2c_initialized = false;
    return false;
  }

  Wire.setClock(clock_hz);  // set 是“设置”，clock 是“时钟”；Arduino Wire 库函数，用于设置 I2C 总线的时钟频率

  g_mpu6050_address =
    device_address;

  g_i2c_initialized = true;

  delay(200);  // delay 是“延时”；Arduino 核心函数，参数单位为 ms（millisecond，毫秒），执行期间程序会暂停

  if (!mpu6050_probe()) {  // probe 是“探测”；这是自封装 MPU6050_Sensor 库函数，用于向 MPU6050 地址发送请求并检查设备是否返回 ACK
    mpu6050_deinit();  // deinit 是 deinitialize（取消初始化）的缩写；这是自封装 MPU6050_Sensor 库函数，用于清除本库的初始化状态但不关闭共享 I2C 总线
    return false;
  }

  uint8_t identity = 0;

  if (!mpu6050_read_who_am_i(  // read 是“读取”，WHO_AM_I 是 Who Am I（我是谁）身份寄存器；这是自封装 MPU6050_Sensor 库函数，用于读取芯片身份值
        &identity
      )) {
    mpu6050_deinit();  // deinit 是 deinitialize（取消初始化）的缩写；这是自封装 MPU6050_Sensor 库函数，用于清除本库的初始化状态但不关闭共享 I2C 总线
    return false;
  }

  if (who_am_i != NULL)
    *who_am_i = identity;

  if (!mpu6050_wake()) {  // wake 是“唤醒”；这是自封装 MPU6050_Sensor 库函数，用于写入电源管理寄存器，使 MPU6050 退出睡眠状态
    mpu6050_deinit();  // deinit 是 deinitialize（取消初始化）的缩写；这是自封装 MPU6050_Sensor 库函数，用于清除本库的初始化状态但不关闭共享 I2C 总线
    return false;
  }

  delay(100);  // delay 是“延时”；Arduino 核心函数，参数单位为 ms（millisecond，毫秒），执行期间程序会暂停
  return true;
}

void mpu6050_deinit(void) {  // deinit 是 deinitialize（取消初始化）的缩写；这是自封装 MPU6050_Sensor 库函数，用于清除本库的初始化状态但不关闭共享 I2C 总线
  /*
    MPU6050和OLED共用GPIO8、GPIO9上的Wire总线
    这里只清除本库状态，不关闭共享I2C总线
  */
  g_i2c_initialized = false;
}