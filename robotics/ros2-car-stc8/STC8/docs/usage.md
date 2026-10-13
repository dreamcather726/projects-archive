# V4 使用与开发说明

本工程使用 STC8H1K08 TSSOP20、四台直流编码减速电机和两片 TB6612。每台独立 DIR、PWM、编码器 A 相。详细接线见 [wiring.md](wiring.md)，功能对照见 [features.md](features.md)，二进制帧与参数编号见 [protocol.md](protocol.md)。

## 构建和烧录

用 Keil 打开根目录 STC8.uvproj，构建唯一目标 STC8_4Motor_AOnly。也可通过 VS Code Keil Assistant 构建。当前使用 C51 SMALL 内存模型、ROM LARGE、OPTIMIZE(9,SIZE)。这是 C51 支持的代码优化方式，见 [Keil 官方说明](https://www.keil.com/product/brochures/c51_v6.pdf)。

输出 HEX 在 Build/Objects/STC8_4Motor_AOnly/STC8_4Motor_AOnly.hex；构建日志在 Build/Listings/STC8_4Motor_AOnly/build.log。V4 使用独立 DIR/A 相接线，旧共享 DIR 的 AB HEX 不属于当前工程。

STC-ISP 选择 STC8H1K08、内部时钟 24 MHz、P5.4 为普通 GPIO，禁用该引脚的外部复位功能。V4 不读写 EEPROM；每次上电停止全部电机并恢复默认参数。配置通过电脑端 JSON 保存，重新上电后导入并点击“停机并应用”。

V4 必须配套使用本版 Python 上位机。协议主版本和参数格式均为 4；旧 V3 配置文件需按 V4 格式重新导出。版本调整是因为删除了 EEPROM 保存、自动上报及相关参数，并统一了重复控制指令。

## 图形上位机

V4 保留直接串口通信检查：COM9（按实际端口选择）、115200、8N1、无流控，HEX 模式发送 `50 49 4E 47`，应回复 `50 4F 4E 47 0D 0A`；文本模式可发送 `PING`，应回复 `PONG`。需要重新烧录更新后的 HEX。测试前关闭占用串口的图形上位机，烧录前关闭串口助手。

```powershell
python -m pip install -r Host/requirements.txt
python Host/motor_gui.py
```

没有硬件可运行 `python Host/motor_gui.py --demo`。演示数据用于检查界面，不能验证实际电机或闭环性能。

1. 选择实际串口后连接。串口打开成功就显示“已连接”，无须等待固件应答。没有有效二进制反馈时显示“尚无 V4 状态帧”，仍可发送控制、停止等命令；无应答的执行结果不会显示为已确认。收到有效回复后显示固件版本，并在空闲时尝试读取参数。参数未读完整时可点击“停机并读取”重试。
2. 先打开参数页，填写各台输出轴一圈的 A 相下降沿计数 CPR。默认 1 是占位值。修改方向极性、最大 PWM 和 PI 等参数后点击“停机并应用”。
3. 点击“导出 JSON”将配置保存到电脑。修改编辑框不会自动应用；导入 JSON 也只修改编辑区，随后需点击“停机并应用”。芯片掉电后恢复默认。
4. 控制页分别设置四台模式和目标。PWM 使用千分比，例如 150=15%；速度可选择 count/s 或 RPM。负值反转。
5. “应用到此电机”仅更新该台；“将以上四台设置同步应用”使用一帧同时更新四台。各台可混合使用 PWM、速度和停止模式。
6. 曲线页选择 M1～M4，查看最近 30 秒速度目标、反馈和 PWM。仅速度模式有转速目标曲线。可记录含故障位图的 CSV，或将当前事件列表导出 TXT；事件区保留最近约 300 行。
7. 完成后停止并断开。断开和关闭窗口时后台尝试全部停止；串口失败时固件失联保护仍有效。

上位机使用单个后台线程读写串口，界面线程只处理控件。默认查询周期 500 ms（每秒 2 次），可通过 Host/motor_gui.py 中的 STATUS_QUERY_INTERVAL 调整，单位为秒；运动时按失联时间的约三分之一独立发送心跳，心跳保持已接受目标，不自动重发目标或解除故障。不要让多个程序同时控制同一个串口。

应答超时不会自动断开串口；手动断开、关闭窗口、串口打开失败或实际串口读写异常才会结束连接。暂时没有反馈时，运动指令发出后仍发送心跳；设备故障回复会停止这种续期。参数读取和应用仍需要应答来确认结果。

连接栏下方显示发送字节数、接收字节数、有效 V4 帧数和 CRC 错误数。点击“PING 检查”发送文本 PING，收到 PONG 只表示文本收发正常，不代替有效状态帧。日常日志只显示连接、操作结果和故障变化。勾选“串口调试（每秒采样）”，到“曲线与日志”查看 TX/RX 的 HEX：二进制收发分别每秒最多显示一次，文本 PING/PONG 立即显示；采样日志不代表完整抓包。日志保留最近约 300 行，可清空或导出 TXT。调试日志默认关闭，收发统计与状态曲线始终按原频率更新。未匹配的固件/参数版本会提示具体收到的版本并保持串口打开，使用配套固件后可恢复。

接收 0 字节说明目前没有任何数据返回；有原始字节而有效帧为 0 时，查看日志和 CRC 计数。收到 READY/IRQ_READY/WAIT_SEND_S 表示正在收到诊断程序文本，不能当成四电机 V4 状态；收到 PONG 而没有 V4 帧则说明文本通信正常，需继续检查二进制协议与正式固件运行路径。

参数页包含各台 15 个参数和一项全局参数 timeout_ms。写参数和恢复默认需要全部停止；配置立即写入 RAM。所有参数均不掉电保存，恢复默认会将状态设置为默认值。

配置 JSON 格式为 stc8-motor-config-v4，有严格版本、字段和范围检查。完整配置按多个参数命令应用；通信中断时可能只有部分参数应用，应该停机并读取确认。导出保存的是电脑文件，不是芯片 EEPROM。

## 停止、保护和方向

单台立即停止把该台 PWM 置零；如果其他电机仍运行，STBY 保持使能，此时该台为 TB6612 零 PWM 制动。全部立即停止关闭共用 STBY，两片驱动都待机。独立释放单台进入滑行受共用 STBY 硬件限制。

减速停止按该台 PWM 减速步长降至零；换向先零 PWM，再等待 reverse_ms 后反向增加。PWM 输出和速度目标分别有加减速步长，单位见参数表，控制周期 50 ms。

改变曾运行过的电机的方向极性或换向等待参数，会按新等待时间重新暂停输出。停止再启动不会绕过尚未完成的换向等待。

堵转检测适用于 PWM 和速度模式：目标非零、实际 PWM 幅度达到 stall_pwm，且反馈速度幅度不高于 stall_speed，持续 stall_ms 后，锁存故障并全停。stall_pwm=0 关闭检测。该方法不测电流，不替代电流保护。

任一失联、RX 溢出、堵转/无反馈或调度超时都会关闭 STBY。故障后点击全部停止，再清除故障，再由操作者重新启动。仅查询不会延长运动期限。

每个编码器 A 相下降沿计一次。单 A 相不能辨识真实方向，计数符号和速度符号从控制方向推定；外力反转、惯性反转或手动推动可能产生不正确的符号。这里没有精确位置控制、绝对位置或回零功能。

CPR 应采用输出轴每圈实际 A 相下降沿数。若资料给的是编码器电机轴每圈脉冲，需要结合减速比；如果给的是 A/B x4 CPR，不能直接使用。RPM 换算：`RPM = count/s × 60 / CPR`。50 ms 固定窗口测速分辨率约 20 count/s，RPM 输入精度不代表实际测量精度。

## 首次上板

### 已烧录但串口无回复的诊断

最小串口程序 `Build/Diagnostics/Objects/UART_Probe.hex` 已在实板确认 READY、PING 回显和 PONG。先前 V3 初始化探针也已确认 START、HAL_OK、EEPROM_OK、PWM_OK 和连续 IRQ_READY：各初始化阶段以及定时器、UART 中断发送可运行。V4.1 正式固件随后已在实板确认正常二进制收发：上位机显示固件 V4.1、参数格式 V4、接收 9152 字节、143 个有效 64 字节状态帧、CRC 错误 0，故障状态正常。该记录验证了串口与状态反馈，尚未验证电机接线、编码器脉冲及实际闭环。

V4.1 将 UART RX 的 P3.0 改为与已实测最小程序相同的准双向模式，端口锁存值置 1；TX 仍为推挽。UART ISR 先读 SBUF，再清 RI；Serial_Read 使用 SMALL 模型的 data 指针，避免不必要的通用指针转换。GPIO 模式、读取顺序和 data 指针修改一起对齐已知成功路径；V4.1 实板收发已正常，各修改的独立作用未作隔离验证。

Keil 生成的实际 HEX 另用 [emu8051 指令核心](https://github.com/jarikomppa/emu8051) 执行，输入 PING 和查询均得到正确结果；串口/定时器事件由本地模型注入，不能替代 STC 外设、引脚、电平和时序实测。工具及输出在 Build/NativeCheck，与正式固件构建无关。

精简后的正式固件不再进行 EEPROM 初始化。启动汇编在清零 XDATA 前显式选择内部 XRAM；适配层也显式清理中断状态、设置 UART 端口锁存值和 SMOD，避免依赖 ISP 返回时的寄存器状态。这些设置用于确定初始化条件。V4.1 的实际状态收发已由用户截图确认正常，单项修改的独立作用未作隔离验证。

当前 `Build/Diagnostics/InitObjects/Init_Probe.hex` 使用 V4 外设适配层和 RAM 默认参数。以文本发送 S，或 HEX 发送 53，正常依次输出 START、HAL_OK、DEFAULT_OK、PWM_OK，然后每秒 IRQ_READY。此时发送 PING 应回复 PONG。诊断程序保持 STBY 关闭，不提供四电机二进制协议，上位机显示“尚无 V4 状态帧”属于预期行为。完成后重新烧录正式 HEX 再控制电机。

### 完整程序的阶段诊断

`Build/Diagnostics/FullObjects/STC8_Full_Probe.hex` 使用正式主循环、协议和中断，编译时开启 BOARD_COMM_DIAGNOSTICS=1。此目标拒绝运动指令并全停，只用于诊断；正式工程默认关闭此宏，保持正常四电机功能，不产生诊断输出。诊断版 7359 字节代码，正式版 7098 字节。

启动时依次发出 H、D、M：外设初始化完成、RAM 默认参数完成、电机状态归零完成。此后电机停止且发送队列空闲时，定时中断约每秒发送 `DBG S=04 I=0E R=0E C=01 U=50 E=97` 这样的快照。为了在主循环卡住时仍能报告，该行在中断里用轮询发送；这期间可能漏接串口字节，诊断版不能用于实际电机运行。

| 字段 | 含义 |
|---|---|
| S | 主程序阶段：1=HAL 完成，2=默认参数完成，3=停止初始化完成，4=处理串口，5=执行控制，6=本轮主循环完成 |
| I | 接收中断放入缓冲的字节计数 |
| R | 主循环从缓冲取出的字节计数 |
| C | CRC 校验通过并交给命令处理器的帧计数 |
| U | 采样时的 SCON；例如 50=UART 模式 1、允许接收、RI/TI 都为 0 |
| E | 采样时的 IE；正常运行常见 97 |

字段均为两位十六进制，I/R/C 是 8 位计数，会回绕。I 增长而 R 不增长提示接收缓冲后面的主循环路径异常；I/R 增长但 C 不增长提示帧未完成或 CRC 不匹配。阶段值可能因采样时刻变化，不应仅凭一次 S=05 判断控制过程卡住。

架空车轮，先给 M1～M4 分别低 PWM 试转，检查电机编号和对应计数。正指令实际倒转时，停止全部，在参数页修改该台 polarity=1，再应用，并导出 JSON，或调整电机两根功率线。

先确认 PWM、编码器、电源和方向，再运行速度闭环。初始 Kp=4/64、Ki=1/64 只是起始值。按实际电机、负载和编码器逐台调整 PI、加减速、最大输出和堵转阈值。编码器最高计数频率、中断丢计数条件、PWM 波形需实测。

## 命令行

把 COM3 替换为实际串口：

```powershell
python Host/motor_host.py --port COM3 --mode info
python Host/motor_host.py --port COM3 --mode query
python Host/motor_host.py --port COM3 --mode pwm --motor 1 --value 150 --seconds 2
python Host/motor_host.py --port COM3 --mode speed --motor 2 --value 100 --seconds 3
python Host/motor_host.py --port COM3 --mode speed --motor 2 --value 30 --unit rpm --seconds 3
python Host/motor_host.py --port COM3 --mode pwm --m1 200 --m2 -200 --m3 200 --m4 -200 --seconds 3
python Host/motor_host.py --port COM3 --mode stop
python Host/motor_host.py --port COM3 --mode clear
python Host/motor_host.py --port COM3 --mode zero --motor 1
```

速度的 count/s 目标必须为整数；RPM 可为小数，由上位机读取该台 CPR 后转换。未指定 --motor 的运动命令设置四台，未给出的目标按 0 处理。运动期间持续续发控制，结束或异常时尝试停止所选电机；断开后没有其他控制者发送心跳时，仍运行的电机会因整板失联而停机。

```powershell
python Host/motor_host.py --port COM3 --mode stop
python Host/motor_host.py --port COM3 --mode set --motor 1 --param cpr --value 600
python Host/motor_host.py --port COM3 --mode set --motor 1 --param kp --value 16
python Host/motor_host.py --port COM3 --mode get --motor 1 --param kp
python Host/motor_host.py --port COM3 --mode set --param timeout_ms --value 1000
python Host/motor_host.py --port COM3 --mode read-config --config motor_config.json
python Host/motor_host.py --port COM3 --mode write-config --config motor_config.json
```

write-config 停止全部后应用到 RAM，掉电恢复默认。read-config 同样先停止全部。单项 get 不要求停机，也不续期。

离线编码检查：

```powershell
python Host/motor_host.py --mode pwm --motor 1 --value 150 --dry-run
python Host/motor_host.py --mode speed --motor 1 --value 30 --unit rpm --cpr 600 --dry-run
```

## Python 与 ROS2 接口

```python
import sys, time, serial
sys.path.insert(0, "Host")
from motor_host import MotorClient

with serial.Serial("COM3", 115200, timeout=0.02, write_timeout=0.35) as port:
    client = MotorClient(port)
    client.stop()
    client.clear_faults()
    client.set_parameter(1, "cpr", 600)  # 停机修改；掉电恢复默认，可导出 JSON
    try:
        client.set_all(["speed", "pwm", "speed", "stop"], [200, 300, 200, 0])
        for _ in range(10):
            state = client.heartbeat()
            if state.fault:
                raise RuntimeError(state.fault_text)
            print(state.motors[0].speed, state.motors[0].count)
            time.sleep(0.1)  # 此例使用默认 1000 ms 失联时间
        client.set_motor(2, "pwm", -200)  # 其余三台目标保持原状态
    finally:
        client.stop()
```

ROS2 节点可复用该 API，用一个串口工作线程处理订阅目标、心跳和反馈。RPM、轮径换算和车体运动学放在电脑侧；有符号计数为推定方向，不能用于可靠的有符号里程计。此工程提供串口 API，未引入依赖特定 ROS2 发行版的节点包。

## 开发配置与资源

- Firmware/Include/parameters.h：稳定参数 ID，新增参数时同步修改固件范围/默认值、上位机表和格式版本。
- Firmware/Source/parameters.c：默认值、验证、参数访问（仅 RAM）。
- Firmware/Source/control_logic.c：定点 PI，增益固定除以 64，含抗积分饱和。
- Firmware/Source/main.c：四台独立状态机、同步请求、保护和反馈。
- Firmware/Include/board_config.h：硬件频率、DIR 端口/引脚和可用 PWM 路由。编译时检查 DIR 重复及与 PWM/UART/编码器的冲突。
- Peripheral/Source/motor_hal.c：紧凑 STC 外设适配层；原通用 SDK 仍保留，但未参与构建。

DIR 可从本板空闲 P1/P3 GPIO 中重新选择，初始化模式跟随配置。PWM 默认使用 P1.0/1.2/1.4/1.6；BOARD_PWM_ROUTE=0xC0 可将 M4 PWM 改到 P3.4，需要同步改接线并解决冲突。编码器 A 使用固定 INT0～INT3，引脚不能随意改到普通 GPIO。控制周期固定 50 ms，修改需同步重新定义参数单位并整定算法。

链接范围为 8 KB CODE、256 B 内部 RAM、1 KB XDATA，启动清零 1 KB XDATA。继续加功能时必须检查 Build/Listings 下的链接表与 HEX 地址；剩余程序空间较少。Keil 使用兼容 8052 模板，实际硬件寄存器来自 STC8H 库头文件。

本版构建：CODE 7098 字节、DATA 89 字节、XDATA 534 字节；HEX 地址跨度 7113 字节，最高地址 0x1bc8，未超过 0x1FFF。比 V3.1 的 8016 字节代码减少 918 字节。Keil 编译为 0 错误、0 警告。

## 软件验证

Windows：

```powershell
python -B -m unittest discover -s Tests -p test_host.py -v
python -B -m unittest discover -s Tests -p test_gui.py -v
```

Linux/WSL，在项目目录执行：

```bash
gcc -std=c99 -Wall -Wextra -Werror -fsanitize=address,undefined -DMOTOR_HOST_TEST -DMOTOR_ENCODER_AB=0 -ITests/mocks -IFirmware/Include -IPeripheral/Include Tests/test_firmware.c Firmware/Source/main.c Firmware/Source/parameters.c Firmware/Source/motor_driver.c Firmware/Source/serial_link.c Firmware/Source/encoder.c Firmware/Source/protocol.c Firmware/Source/control_logic.c -o /tmp/stc8_motor_v4_tests
/tmp/stc8_motor_v4_tests
gcc -std=c99 -Wall -Wextra -Werror -shared -fPIC -fsanitize=undefined -DMOTOR_INTEGRATION_TEST -DMOTOR_HOST_TEST -DMOTOR_ENCODER_AB=0 -ITests/mocks -IFirmware/Include -IPeripheral/Include Tests/test_firmware.c Firmware/Source/main.c Firmware/Source/parameters.c Firmware/Source/motor_driver.c Firmware/Source/serial_link.c Firmware/Source/encoder.c Firmware/Source/protocol.c Firmware/Source/control_logic.c -o /tmp/stc8_motor_v4.so
MOTOR_FIRMWARE_LIB=/tmp/stc8_motor_v4.so python3 -B -m unittest discover -s Tests -p test_integration.py -v
```

C 测试覆盖混合模式、同步应用时序、范围拒绝、换向/减速、失联/溢出/调度/堵转、计数回绕、上电恢复默认、已移除指令拒绝和 PI 限幅。Python 测试覆盖 CRC、拆包/错包/非匹配序号、序号、旧固件拒绝、配置文件、RPM 换算、界面单台操作和 JSON 导入/应用。集成测试让 Python MotorClient 向同一份 C 固件逻辑发送二进制帧，核对参数和控制语义。

这些是软件与编译验证。真实硬件测试应另行完成，完整协议状态收发已有实板记录；其余仍需验证编码器频率上限、电机接线及闭环整定。

