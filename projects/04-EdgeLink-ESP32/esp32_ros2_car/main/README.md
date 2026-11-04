# main：程序入口目录

这里负责启动并周期发送测试帧。

| 文件 | 作用 |
|---|---|
| `main.cpp` | 初始化 UART0，每 200 ms 发送一帧 `FUNC=0x01` 数据 |
| `CMakeLists.txt` | 声明源码、头文件目录和 ESP-IDF 组件依赖 |

当前调用链：

```text
ESP-IDF main_task
    └─ app_main()
        ├─ uart_protocol_init()
        └─ 每 200 ms
            ├─ 把递增计数器打包为四字节小端 Payload
            └─ uart_protocol_send_frame(0x01, payload, 4, 100)
```

周期控制使用 `xTaskDelayUntil()`，任务在等待期间会主动让出 CPU，不会触发空循环导致的任务看门狗问题。

帧格式和测试数据见 [UART_PROTOCOL.md](../docs/UART_PROTOCOL.md)。
