# Arduino IDE 使用说明

本文件夹是一个完整的标准 Arduino 草图。用 Arduino IDE 直接打开
`helloWorld_test_Arduino.ino`，同目录的 `.cpp` 和 `.h` 模块会自动参与编译。

## 1. 安装开发板支持

在 Arduino IDE 的“开发板管理器”中安装：

- `esp32 by Espressif Systems`，建议版本 `2.0.17`

本程序使用 ESP32 Arduino Core 2.x 的 I2S 和 LEDC API；使用 3.x 版本可能因 API
变化而编译失败。

## 2. 安装依赖库

在“库管理器”中安装：

- `MPU6050` by Electronic Cats，版本 `1.4.4`
- `Adafruit GFX Library`，版本 `1.12.6`
- `Adafruit ST7735 and ST7789 Library`，版本 `1.11.0`

如果 Arduino IDE 提示安装关联依赖，请同时安装 `Adafruit BusIO`。
`Wire`、`SPI` 和 `Ticker` 已包含在 ESP32 开发板支持包中，无需单独安装。

## 3. Arduino IDE 设置

- 开发板：`ESP32S3 Dev Module`
- Flash Size：`8MB (64Mb)`
- CPU Frequency：`240MHz (WiFi)`
- Upload Speed：可保持默认；上传不稳定时选 `460800`
- 串口监视器波特率：`115200`

选择开发板实际连接的端口，点击“验证”或“上传”即可。

## 文件说明

- `helloWorld_test_Arduino.ino`：主程序，包含 `setup()` 和 `loop()`
- `module_tests/`：IMU、屏幕、按键和电机的四个独立测试程序，详见其中的 `README.md`
- `arm_servo.*`：机械臂舵机驱动
- `button.*`：按键消抖
- `i2s_speaker.*`：I2S 扬声器
- `imu.*`：MPU6050 姿态数据
- `motor_driver.*`：电机驱动
- `serial_receive.*` / `serial_sender.*`：串口协议
- `speed_pid.*`：速度 PID
- `wheel_encoder.*`：轮式编码器

上级目录中的 `platformio.ini` 也已同步适配新的文件布局，仍可使用 PlatformIO 构建。
