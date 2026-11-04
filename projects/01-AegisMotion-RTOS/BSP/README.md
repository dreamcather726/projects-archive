# BSP 板级驱动

本目录用于存放与开发板硬件直接相关的驱动：

- `Inc/`：只放应用层可引用的公开接口。
- `Src/`：实现源文件及实现私有头文件。
- GPIO、LED、按键、串口、定时器等驱动放在这里。

建议文件采用 `bsp_功能名.c/.h` 的命名方式，例如：

```text
BSP/Inc/bsp_led.h
BSP/Src/bsp_led.c
```

当前串口公开接口是 `Inc/bsp_uart.h`。`Src/bsp_uart_tx.h` 是驱动内部的纯TX环形算法，
只由 `bsp_uart.c` 和原生测试引用；不要让应用层依赖其结构体或直接改变读写指针。
普通TX整段入队不阻塞，急停删除未开始帧但保留当前帧尾，故障路径才使用有界寄存器轮询。

配置与调用层次见 [代码说明](../Docs/CODE_GUIDE.md)，接线与调试见 [串口说明](../Docs/UART_DEBUG_GUIDE.md)。
