# lib：模块实现目录

这里存放项目模块的 `.cpp` 实现，主程序通过 `include/` 中对应的头文件调用它们。

| 文件 | 作用 |
|---|---|
| `uart.cpp` | UART0、CRC16-Modbus、标准组帧和线程安全发送 |
| `motor.cpp` | 两路电机方向与 LEDC PWM 控制 |
| `LED.cpp` | LED GPIO 控制 |

ESP-IDF 不会因为文件位于 `lib/` 就自动编译。每个实现文件都必须加入 [`main/CMakeLists.txt`](../main/CMakeLists.txt)：

```cmake
SRCS
    "main.cpp"
    "../lib/LED.cpp"
    "../lib/motor.cpp"
    "../lib/uart.cpp"
```

调用方只包含 `.hpp`，不要 `#include` `.cpp`。串口模块的完整接口见 [UART_PROTOCOL.md](../docs/UART_PROTOCOL.md)。
