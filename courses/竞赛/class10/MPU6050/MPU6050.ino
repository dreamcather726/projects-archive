#include "MPU6050_Sensor.h"                  // 引入本封装，获得数据结构和函数声明
MPU6050_Data data; 
  const int MPU_SDA_PIN = 8;                  // 设置 MPU6050 的 SDA 接 GPIO17
  const int MPU_SCL_PIN = 9;                  // 设置 MPU6050 的 SCL 接 GPIO18
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
    // MPU6050_Data data;                         // 创建结构体，保存一次完整测量结果

    if (!mpu6050_read_data(&data)) {           // 读取 14 字节数据并判断是否成功
      Serial.println("读取MPU6050失败");       // 读取失败时显示提示
      delay(500);                              // 暂停 500 毫秒后再尝试
      return;                                  // 结束本轮 loop()，随后会重新进入 loop()
    }                                          // 读取失败处理结束
    int ax=data.accel_x;
    Serial.print("加速度：");                 // 输出加速度标题，不换行
    Serial.print("X=");                       // 输出 X 轴标签
    Serial.print(ax);                // 输出 X 轴加速度原始值
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
