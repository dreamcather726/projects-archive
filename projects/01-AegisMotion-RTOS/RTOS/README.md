# FreeRTOS 内核说明

本目录内置 FreeRTOS Kernel V11.3.1，来源为官方
`FreeRTOS/FreeRTOS-Kernel` 仓库的 `V11.3.1` 标签（提交 `3a22924`）。许可证见
`FreeRTOS-Kernel/LICENSE.md`。

工程只加入了 STM32F407 所需的核心源码和 `portable/GCC/ARM_CM4F` 端口。FreeRTOS
官方说明该 GCC 端口同样支持 ArmClang，因此可直接由本工程的 Arm Compiler 6 编译。

日常开发通常不修改这里。应用任务放在 `App/Src/app_tasks.c`，可调配置放在
`Config/Inc/FreeRTOSConfig.h`。这样以后升级内核时，不会把项目业务代码混入第三方源码。
