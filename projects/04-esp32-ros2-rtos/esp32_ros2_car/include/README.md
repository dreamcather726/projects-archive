# include：公共头文件目录

这里存放模块对外公开的常量、数据结构和函数声明。主程序只需包含头文件，不应直接包含 `lib` 中的 `.cpp`。

| 文件 | 公开内容 |
|---|---|
| `uart.hpp` | UART0 测试配置、协议常量、CRC、组帧和发送接口 |
| `motor.hpp` | 两路电机引脚和 PWM 控制接口 |
| `LED.hpp` | LED 初始化及控制接口 |

例如：

```cpp
#include "uart.hpp"

extern "C" void app_main(void)
{
    ESP_ERROR_CHECK(uart_protocol_init());
}
```

之所以可以直接写 `#include "uart.hpp"`，是因为 [`main/CMakeLists.txt`](../main/CMakeLists.txt) 已设置：

```cmake
INCLUDE_DIRS "." "../include"
```

新增模块时，在这里添加同名 `.hpp` 声明，在 `lib/` 添加 `.cpp` 实现，再把 `.cpp` 加入 `main/CMakeLists.txt` 的 `SRCS`。
