# ESP32 ROS2 Car

> ESP32-S3 小车的 ESP-IDF 工程。目前包含自定义二进制串口协议、两路电机 PWM 和 LED 基础模块。

## 目录一览

| 路径 | 用途 |
|---|---|
| `main/` | `app_main()` 程序入口和组件构建配置 |
| `include/` | 模块的公共声明、结构体和常量 |
| `lib/` | UART、电机、LED 的具体实现 |
| `docs/` | 协议说明和 ESP-IDF API 速查表 |
| `build/` | ESP-IDF 自动生成的编译产物，不要手动修改 |

## 当前程序做什么

启动后，程序会：

1. 临时测试模式下复用 UART0 现有控制台引脚，串口参数为 115200 8N1。
2. 使用 CRC16-Modbus 生成 `AA ... BB` 标准二进制帧。
3. 每 200 ms 发送一帧 `FUNC=0x01` 数据。
4. Payload 是一个不断递增的四字节小端 `uint32_t` 计数器。

当前串口模块不包含接收器、速度命令、PID、编码器或 MPU6050 数据结构。完整测试帧说明见 [UART_PROTOCOL.md](docs/UART_PROTOCOL.md)。

## main 如何使用 include 和 lib

```text
main/main.cpp
    │  #include "uart.hpp"
    ▼
include/uart.hpp
    │  声明协议接口、帧结构和常量
    ▼
lib/uart.cpp
       实现 UART0、CRC、组帧和发送函数
```

[`main/CMakeLists.txt`](main/CMakeLists.txt) 中的两项配置把它们连接起来：

```cmake
idf_component_register(
    SRCS
        "main.cpp"
        "../lib/uart.cpp"
    INCLUDE_DIRS
        "."
        "../include"
)
```

- `INCLUDE_DIRS` 告诉编译器去哪里寻找 `.hpp`。
- `SRCS` 告诉 ESP-IDF 编译哪些 `.cpp` 实现。
- `main.cpp` 只包含头文件，不要直接包含 `.cpp`。

## 编译、烧录和监视日志

在配置好的 ESP-IDF 终端中运行：

```powershell
idf.py build
idf.py -p COM端口 flash monitor
```

当前是 UART0 临时测试模式，ESP-IDF 日志和二进制协议会混用同一串口。退出监视器使用 `Ctrl+]`。正式使用时再切换回 UART1 GPIO17/18。

## 相关文档

- [UART1 二进制协议](docs/UART_PROTOCOL.md)
- [GPIO、PWM、I2C 和常用 API](docs/ESP_IDF_API_QUICK_REFERENCE.md)
- [main 入口说明](main/README.md)
- [include 头文件说明](include/README.md)
- [lib 实现文件说明](lib/README.md)
