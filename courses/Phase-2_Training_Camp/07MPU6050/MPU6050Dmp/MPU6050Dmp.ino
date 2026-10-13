/*
  程序功能：使用 MPU6050Dmp 函数库读取 Yaw、Pitch、Roll。
  使用方法：SDA 接 GPIO 8、SCL 接 GPIO 9；上电初始化期间保持模块静止。
  运行结果：串口每次取得新数据包后输出三个姿态角。
*/

// 引入同一工程目录中的函数声明。
// 主程序只需认识这些公开函数，不需要处理 FIFO 和四元数变量。
#include "MPU6050Dmp.h"

// 本实验统一使用 GPIO 8 作为 SDA、GPIO 9 作为 SCL。
#define PIN_SDA 8
#define PIN_SCL 9

void setup() {
  // 启动 USB 串口，用于显示初始化状态、错误信息和姿态角。
  Serial.begin(115200);
  // 下一行提示必须写在初始化函数前，因为函数内部会立即开始校准。
  Serial.println("正在初始化和校准，请保持模块静止、水平放置...");

  // mpuDmpBegin() 依次完成：
  // I2C 启动 -> MPU6050 连接测试 -> DMP 初始化 -> 静止校准 -> 开启 DMP。
  // 这里只传入 SDA 和 SCL，I2C 时钟与校准轮数使用头文件中的默认值。
  if (!mpuDmpBegin(PIN_SDA, PIN_SCL)) {
    // 初始化失败后先判断是底层连接失败，还是上层 DMP 初始化失败。
    if (!mpuDmpIsConnected()) {
      Serial.println("MPU6050 连接失败，请检查供电、共地和 SDA/SCL");
    } else {
      // 能连接 MPU6050 但 DMP 未成功时，打印状态码帮助排查库和配置。
      Serial.print("DMP 初始化失败，错误代码：");
      Serial.println(mpuDmpGetStatus());
    }

    // 初始化失败后停在这里，防止 loop() 继续读取无效数据。
    while (true) {
      delay(1000);
    }
  }

  // 初始化成功后打印自动校准得到的六个偏移量，便于记录和比较。
  mpuDmpPrintOffsets();
  Serial.println("MPU6050 DMP 初始化完成");
}

void loop() {
  // 每次循环尝试读取一包新的 DMP 数据。
  // 只有返回 true 时，本次 Yaw、Pitch、Roll 才是刚更新的结果。
  if (mpuDmpUpdate()) {
    // 三个 get 函数只返回最近一次有效角度，不会再次访问传感器。
    // Serial.print(value, 1) 中的 1 表示保留一位小数。
    Serial.print("Yaw:");
    Serial.print(mpuDmpGetYaw(), 1);
    Serial.print("  Pitch:");
    Serial.print(mpuDmpGetPitch(), 1);
    Serial.print("  Roll:");
    Serial.println(mpuDmpGetRoll(), 1);
  }

  // 每 100 ms 尝试更新一次，避免串口输出过快而难以观察。
  delay(100);
}
