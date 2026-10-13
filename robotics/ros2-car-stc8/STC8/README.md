# STC8H1K08 四电机控制 · V4

上位机串口 → STC8H1K08 → 两片 TB6612 → 四台直流编码减速电机。每台独立 **DIR、PWM、编码器 A 相**。四路硬件 PWM 20 kHz、四路下降沿中断，方向由控制输出推定。

| 电机 | DIR | PWM | 编码器 A 相 |
|---|---|---|---|
| M1 左前 | P1.1 | P1.0 / PWM1P | P3.2 / INT0 |
| M2 右前 | P1.3 | P1.2 / PWM2P | P3.3 / INT1 |
| M3 左后 | P1.5 | P1.4 / PWM3P | P3.6 / INT2 |
| M4 右后 | P1.7 | P1.6 / PWM4P | P3.7 / INT3 |

两片 STBY 共接 P5.4；USB-TTL TXD → P3.0，RXD → P3.1；全部共地。DIR → IN1，你的硬件反相 DIR → IN2。逐脚接线见 [wiring.md](docs/wiring.md)。

## 先运行上位机

```powershell
python -m pip install -r Host/requirements.txt
python Host/motor_gui.py
```

没有接硬件时：`python Host/motor_gui.py --demo`。演示模式仅模拟界面数据。

V4.1 支持串口文本 `PING` → `PONG`。使用 HEX 模式时发送 `50 49 4E 47`，应回复 `50 4F 4E 47 0D 0A`；串口为 115200、8N1。该命令只检查通信，不延长电机运行心跳。

上位机支持四路独立或混合 PWM/速度模式、同步设置、立即/减速停止、RAM 参数读写、实时曲线、JSON 配置和 CSV 日志。功能对照见 [功能表](docs/features.md)。

本轮移除片内 EEPROM 保存、主动上报和重复控制指令。当前功能与协议见下方说明。

## 固件与参数

用 Keil 打开根目录 `STC8.uvproj`，构建唯一目标 **STC8_4Motor_AOnly**。HEX：

`Build/Objects/STC8_4Motor_AOnly/STC8_4Motor_AOnly.hex`

STC-ISP 选择 STC8H1K08、内部 24 MHz，P5.4 设置为 GPIO。上电始终停止，不恢复之前的运动目标。

常用参数通过串口修改，每台分别配置方向极性、最大 PWM/速度、每圈计数、Kp/Ki、积分限幅、加减速和堵转阈值。参数修改需停止全部电机，“应用”修改 RAM；用 JSON 保存到电脑，重启后导入并重新应用。引脚及硬件频率集中在 [board_config.h](Firmware/Include/board_config.h)。

速度默认单位 count/s。RPM 由上位机按每台输出轴每圈 A 相计数换算；默认每圈计数 1 是占位值，使用 RPM 前应填写实测值。仅 A 相不能提供可靠的真实方向或精确位置反馈。

## 文件布局

```text
STC8/
├─ STC8.uvproj          Keil 工程
├─ Firmware/
│  ├─ Source/           控制、参数、协议和计数
│  ├─ Include/          应用接口与板级配置
│  └─ Startup/          启动汇编
├─ Peripheral/
│  ├─ Include/          寄存器定义、外设接口
│  └─ Source/           紧凑电机适配层及原 STC 外设库
├─ Host/                图形界面、串口 API、命令行、离线演示
├─ Tests/               C、Python、界面及协议集成测试
├─ docs/                使用、接线、协议和功能表
├─ Tools/               STC-ISP 下载软件
└─ Build/               HEX、对象和编译日志
```

原 STC 外设源码保留在 Peripheral，工程通过紧凑适配层使用其寄存器定义，以控制 8 KB 芯片上的程序大小。当前 V4 使用四路独立 DIR/A 相，不包含旧共享 DIR 的 AB 目标。

## 命令行及验证

```powershell
python Host/motor_host.py --port COM3 --mode info
python Host/motor_host.py --port COM3 --mode pwm --motor 1 --value 150 --seconds 2
python Host/motor_host.py --port COM3 --mode stop
python -B -m unittest discover -s Tests -p test_host.py -v
python -B -m unittest discover -s Tests -p test_gui.py -v
```

完整操作、Python/ROS2 接入方式及测试命令见 [使用说明](docs/usage.md)，二进制帧见 [V4 协议](docs/protocol.md)。V4 需使用配套固件和上位机。V3 上位机和配置 JSON 不适用于 V4。软件验证使用模拟硬件，真实波形、计数频率和 PI 参数需上板验证。
