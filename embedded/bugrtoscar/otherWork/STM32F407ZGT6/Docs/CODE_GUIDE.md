# 文件结构与代码阅读说明

本轮只整理现有STM32程序和PC人工调试工具：补注释、消除重复状态/转发、校准文档；不新增香橙派程序，不改变协议、保护条件、超时参数或按钮动作。

## 1. 文件职责

```text
App/
  Inc/                 应用/协议/底盘的公开接口
  Src/main.c           初始化与启动调度器
  Src/app.c            应用初始化、IMU文本、Control/Data入口、硬故障入口
  Src/app_tasks.c      静态任务及看门狗健康检查
  Src/host_protocol.c  主控帧、电机回复、自检/巡检/授权与发送队列
  Src/mecanum_chassis.c  四轮分解、逐轮应答、同步事务/失败停车
BSP/
  Inc/bsp_uart.h       应用层使用的串口公开接口
  Src/bsp_uart.c       GPIO/USART/DMA/中断实现
  Src/bsp_uart_tx.h    私有纯TX环形算法，原生测试共用
Config/Inc/            功能开关、地址、方向、超时、FreeRTOS配置
Common/Inc/            公共状态码
Libraries/IMU/         JY61P接收/配置/样本与Yaw修正
Libraries/StepperMotor/  Emm_V5命令编码及保留的原库文件
Libraries/STM32F4xx_StdPeriph_Driver/  标准外设库，保持原布局
RTOS/ 与 CMSIS/        内核/芯片支持，保持厂商布局
PC_Tools/              当前人工调试程序、依赖、使用说明
Tests/                 主机端C/Python测试、硬件替身、编译检查
Docs/                  协议、接线调试、代码解释、安全验收
Objects/ 与 Listings/  Keil产物，不是源码；Tests/build/是测试产物
```

`Inc`用于公开头，`Src`允许包含实现私有头。TX辅助头从BSP/Inc移到BSP/Src后，驱动通过同目录包含；原生测试显式引用该私有目录。Keil工程文件、Python入口、App公开函数及主要搜索路径保持不变。

原电机库的fifo/dianji_uart仍保留，且原Keil工程列出了其源文件；现有电机收发主路径是Emm_V5命令编码 + BSP_UART，不再添加第二个USART中断实现。此次没有删除厂商文件或改动工程引用，避免把整理变成驱动替换。

## 2. 建议阅读顺序

1. Config/Inc/app_config.h：先看四轮地址、方向、速率与所有超时。
2. App/Src/main.c、app.c、app_tasks.c：看启动顺序和任务边界。
3. host_protocol.c：看Control周期、命令门禁、自检/巡检/授权与队列。
4. mecanum_chassis.c：看四轮速度分解和逐轮事务。
5. BSP/Inc/bsp_uart.h、BSP/Src/bsp_uart.c：看DMA接收、整帧TX和优先停车。
6. Libraries/IMU与StepperMotor：看设备帧与命令编码。
7. PC_Tools/jy61p_motor_test.py：常量/编码 → 自检 → 串口线程 → GUI → 按钮回调。

## 3. 时序与状态归属

启动：BSP初始化 → HostProtocol初始化并请求全车停车/启动自检 → IMU上电配置 → READY → 启动调度器 → 非阻塞自检推进。APP_STATE_READY不是SELF_TEST_PASSED，只有后者才允许使能/运动。

Control每2ms：刷新待发上报 → 处理电机回复 → 安全/自检/授权检查 → 主控请求 → 停车重试 → 推进Mecanum。每个合法主控帧更新链路时间前再检查一次过期，防止长批次输入覆盖超时。

Data每2ms：解析IMU字节，收齐51/52/53后发布完整样本并格式化E1。2ms是轮询周期，不是IMU输出频率。看门狗每250ms核对所有已启用任务的存活标志，全部正常才刷新。

| 状态/数据 | 所有者 | 不能混淆的概念 |
|---|---|---|
| 主控解析、自检/巡检、授权、底盘事务 | Control任务 | 自检通过≠电机已使能；取消≠停车 |
| IMU解析与发布样本 | Data任务 | 单帧VALID≠新鲜完整样本 |
| HostProtocol发送队列 | Control/Data入队，Control出队 | 入队≠送达；E1可覆盖旧快照 |
| UART软件TX/帧边界 | 任务临界区与TXE中断 | 队列空≠硬件TC完成 |
| PC待应答/授权意图与Tk控件 | Tk主线程 | PC清变量≠物理停车 |
| PC串口及发送队列/邮箱 | 后台线程和受锁保护的入队接口 | 旧PASSED≠急停恢复许可 |

## 4. 保留的独立保护

急停锁、底盘故障锁、待停车、当前控制模式分别表达不同约束，不能用一个busy变量代替。链路与运动分别计时，PING只能刷新链路；续期不能重置设备巡检。

底盘事务包含总200ms期限、TX完成等待和各轮10ms回复期限；先计排队成功会误触发回复超时。故障锁必须重新自检清除，Mecanum_Cancel只撤销软件序列。App_EnterFault已经关闭中断，只能用有界轮询尝试停车，不可只向TX队列写入。

PC保留本地急停发送门禁，同时下位机独立拒绝故障中的运动。两层保护不是重复变量，不应删除其中一层。

## 5. 此次精简及等价依据

- 去掉HostProtocol_SendFrame纯转发层，ACK/STATUS直接进入原QueueFrame，普通帧仍FIFO，IMU仍仅替换同类快照。
- 原ArmFailsafe与AuthorizeMotion相邻调用合并；首次布防初始化巡检、之后只更新动作SEQ/时间，原调用次序和计时位置保留。
- 去掉两行赋值的DisarmFailsafe包装，在原两处直接清布防和巡检等待；没有用EndMotion替代局部解除，避免额外改变模式/结果。
- 自检开始与故障停车共用原StopAll；锁定状态的设置顺序和停车失败重试语义不变。
- RX预期长度直接读帧内LEN；电机frame_ready降为单次调用局部状态；队列尾槽由head/count推导。只删可推导/不需跨调用保存的数据。
- PC恢复的两个布尔量合并为0/1/2阶段；原“发SELF_TEST且观察RUNNING才接受PASSED”保持。重复按钮包装改用partial，原编码与描述不变。
- PC入队与实际写入共用同一运动门禁常量，避免两处指令清单漂移；未改变门禁内容。

不为减少行数合并RX/TX缓冲、超时、故障计数或IMU工作/发布样本；这些数据有不同并发/生命周期。也不拆出新的转发类或重排厂商目录。

## 6. 文档与验证入口

| 文档 | 唯一职责 |
|---|---|
| [项目总览](../README.md) | 编译/测试入口 |
| [PC调试说明](../PC_Tools/README.md) | 当前GUI安装、按钮、授权、日志 |
| [协议说明](HOST_PROTOCOL.md) | 完整协议字段、命令、CRC示例与兼容性 |
| [串口调试](UART_DEBUG_GUIDE.md) | 接线、调试顺序、故障排查 |
| [安全评估](SAFETY_REVIEW.md) | 上一轮功能修复评估及硬件验收清单 |

协议表不在调试文档重复维护，避免出现多个不同版本的STATUS长度/急停说明。

整理前先验证现有回归，并新增FIFO绕回/满队列/快照替换测试；整理后复跑C和Python、PC恢复状态等价测试、八种功能组合、Keil完整编译与Tk界面构建/按钮回调冒烟测试。测试不连接真实串口、不烧录，不以模拟结果代替硬件验收。

本轮实际验证结果（2026-10-04）：

- C回归、37项Python测试通过，其中625条恢复事件序列与原布尔状态逻辑比对一致。
- IMU/HOST/IWDG八种组合编译检查通过；Keil完整编译0错误、0警告，日志位于 `Tests/build/structure_cleanup_build.log`。
- Tk构建、默认授权关闭、急停红色及使能/失能/PING按钮编码冒烟验证通过，未连接真实串口。
- 协议文件私有函数30→27、PC顶层函数/类方法73→69；减少3个C全局状态及1个PC持久状态。测试辅助函数不计入生产代码精简数量。
- 此构建下Code从27322→26938字节、ZI-data从11944→11936字节；产物变小不等于硬件验收通过。
