# STM32F4 标准外设库

本目录保存 STM32F4 Standard Peripheral Library V1.8.0 中当前工程实际使用的驱动：

- `stm32f4xx_rcc`：外设时钟控制；
- `stm32f4xx_gpio`：GPIO 与复用功能配置；
- `stm32f4xx_usart`：USART 初始化、收发与中断控制；
- NVIC 属于 Cortex-M4 内核，工程通过 CMSIS 内核接口配置，不编译 SPL 的 `misc.c`。

代码来源为 ST 的 `STSW-STM32065` 软件包，第三方库文件保持原始内容，许可文件见
`LICENSE.pdf`。以后增加其他外设时，应从同一版本的软件包添加对应的 `.h` 和 `.c`
文件，不混用 HAL、LL 或其他版本的 SPL。
