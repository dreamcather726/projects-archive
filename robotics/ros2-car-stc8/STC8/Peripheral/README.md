# STC8 外设库与四电机适配层

本目录与 Firmware 相邻，构建不依赖原始库文件夹。原 STC8G/H 库的 32 个源文件和头文件保留原版权说明与 GBK 编码。

```text
Peripheral/
├─ Include/    STC8xxxx.H、config.h、motor_hal.h、通用外设头文件
└─ Source/     motor_hal.c 及原通用外设实现
```

V4 工程编译 **motor_hal.c**：依据 STC 的 GPIO、PWM、UART、Timer、Exti 示例，集中完成当前 STC8H1K08 的初始化、PWM 更新、串口发送。中断入口包括 UART1、Timer0；四个编码器中断入口直接定义在 Firmware/Source/encoder.c，减少中断层的调用开销。

原 GPIO.c、PWM.C、UART.C、timer.c、Exti.c，以及 ADC、EEPROM、I2C、SPI、PCA 等通用模块仍可查看或用于后续项目，当前未加入构建。通用库和适配层含相同硬件中断入口，不能同时把重复入口加入当前工程。

Include/config.h 选择 STC8H 系列和 24 MHz 时钟；Firmware/Include/board_config.h 定义本板可用 DIR GPIO、PWM 路由和固定的 UART/编码器资源，适配层按该配置设置引脚模式。运行参数由 Firmware/Source/parameters.c 管理。

V4 电机适配层不再访问 EEPROM；运行配置仅保存在 RAM，通用 EEPROM 库仍留在旁边供其他项目使用。

寄存器及外设依据 [STC 官方资料](https://www.stcmicro.com/datasheet/STC8H1K08_Features.pdf)。工程入口、接线与操作见 [使用说明](../docs/usage.md)。
