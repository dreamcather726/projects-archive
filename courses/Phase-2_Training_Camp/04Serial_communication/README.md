# ESP32-S3 串口通信课堂实验

8 月 10 日的串口核心教学共 5 课时，每课时 45 分钟。第一课时先建立 UART 原理和
固定数据帧知识地图，随后依次讲单字节收发、组帧发送、收帧校验和固定帧应用。
按行文字命令、串口控制其他外设和双板 UART 通信可在后续实验课继续完成。

## 正式通信格式

```text
55 05 CMD DATA CHECKSUM FF
```

- 帧头固定为 `0x55`。
- 长度固定为 `0x05`，表示帧头之后还有 5 个字节。
- `CMD` 是指令类型，`DATA` 是一个指令数据字节。
- 校验位等于“长度 + 指令 + 数据”，结果只保留低 8 位。
- 帧尾固定为 `0xFF`。

示例 `55 05 01 FF 05 FF` 表示把 LED 亮度设为 255。正式协议及应答规则见
`DATA_FRAME.md`。

## 学习目标

完成课程后，学生应能：

1. 说出波特率、TX、RX、GND、8N1 和全双工的含义。
2. 区分 `print()`、`println()`、`write()`，以及 `read()` 和
   `readStringUntil()` 的使用场景。
3. 区分二进制、十进制和十六进制，并读懂一个字节的三种写法。
4. 说出帧头、帧长度、指令类型、指令数据、校验位和帧尾的作用。
5. 解析 `55 05 CMD DATA CHECKSUM FF` 格式的固定二进制帧。
6. 通过串口选择蜂鸣器歌曲、控制呼吸灯。
7. 完成两块 ESP32-S3 之间的 UART 交叉接线和双向收发。

## 8 月 10 日五课时安排

### 第 1 课时：串口数据帧组成与 UART 原理

1. 先展示本模块知识地图。
2. 讲 UART 异步通信、TX、RX、GND、波特率、8N1 和全双工。
3. 讲 bit、byte、ASCII、文本和原始字节。
4. 讲二进制、十进制、十六进制，以及 `BIN/DEC/HEX` 三种显示格式。
5. 用 `0b01010101 = 85 = 0x55` 和 `0b11111111 = 255 = 0xFF` 练习进制转换。
6. 重点拆解 `55 05 CMD DATA CHECKSUM FF` 六个字段，并列出各字段的三种进制。
7. 讲 `FRAME_LENGTH=5` 与整帧 6 字节的关系。
8. 手算普通加法校验和超过 `0xFF` 后保留低 8 位的校验。
9. 建立“先收完整帧、再校验、后执行、再应答”的基本流程。

使用代码：`00_frame_structure_demo/00_frame_structure_demo.ino`

正式讲稿：`教学资料/分课时教案/第11课时_串口数据帧组成与UART原理_教师讲稿.md`

### 第 2 课时：串口缓冲区与单字节收发

1. 使用 `Serial.begin()` 设置波特率。
2. 使用 `available()` 查询接收缓冲区中的字节数。
3. 比较 `peek()` 与 `read()`。
4. 比较 `print()`、`println()` 与 `write()`。
5. 观察字符 `A`、ASCII 65、回车 `\r` 和换行 `\n`。
6. 切换 No line ending、Newline、Both NL & CR 并记录现象。

使用代码：`01_serial_read_char/01_serial_read_char.ino`

### 第 3 课时：数据帧组帧与发送

1. 说明 `.h` 函数声明与 `.cpp` 具体实现的分工。
2. 讲 `FRAME_HEADER`、`FRAME_LENGTH`、`FRAME_TAIL`、`FRAME_SIZE`。
3. 逐行讲 `calculateChecksum(command, data)`。
4. 逐行讲 `sendFrame(command, data)` 和数组下标 0～5。
5. 说明 `Serial.write(frame, FRAME_SIZE)` 两个参数。

使用代码：`03_data_frame/SerialFrame.h`、`03_data_frame/SerialFrame.cpp` 第 1～31 行。

### 第 4 课时：数据帧接收与校验

1. 讲 `available() >= FRAME_SIZE`，不足 6 字节不读取半帧。
2. 讲寻找帧头和丢弃干扰字节。
3. 按顺序读取 length、command、data、checksum、tail。
4. 检查长度、帧尾和重新计算的校验和。
5. 全部通过后保存指令和数据并返回 `true`。
6. 对比正确帧、校验错误帧和帧前干扰数据。

使用代码：`03_data_frame/SerialFrame.cpp` 第 33～80 行。

### 第 5 课时：固定帧控制 LED 与 PING

1. 讲 `03_data_frame.ino` 中协议层与应用层的分工。
2. 使用 `readFrame()`、`getFrameCommand()`、`getFrameData()`。
3. 使用 `CMD_LED=0x01` 控制 GPIO 2 LED 并返回 `0x81` 应答。
4. 增加 `CMD_PING=0x05`，合法数据 `0x00` 返回 `0x85`。
5. 区分未知指令错误码 `0x01` 和数据不合法错误码 `0x02`。
6. 使用 HEX 工具验证正确请求、错误数据和错误校验帧。
7. 对照 `每日作业/08月10日_串口通信作业_学生版.md` 完成作业说明。

使用代码：`08_data_frame_ping/08_data_frame_ping.ino`、`SerialFrame.h`、`SerialFrame.cpp`。

## 8 月 11 日串口续课重点

1. 先复习数据帧六个字段，以及二进制、十进制和十六进制的对应关系。
2. 复习并演示 `sendFrame()` 组成和发送串口数据帧。
3. 重点讲 `readFrame()` 如何从缓冲区接收并寻找完整数据帧。
4. 对长度、帧尾和校验和进行检查，错误帧不执行。
5. 校验通过后读取 `command/data`，执行对应功能并发送应答。
6. 使用正确帧、错误帧和干扰数据完成接收与数据处理实验。

对应作业：`每日作业/08月11日_串口数据帧收发与库文件作业_学生版.md`。

### 后续实验课内容

- `02_read_string_until`：按行读取文字命令。
- `04_serial_song_switch`：数据帧控制歌曲。
- `05_serial_breathing_led`：数据帧控制呼吸灯。
- `06_uart_board_a`、`07_uart_board_b`：双板 UART 收发。

函数详细解释见 `SERIAL_FUNCTIONS.md`，逐行课堂话术见 `串口课堂代码详细讲解.md`。

## 文件说明

| 文件 | 主要知识点 |
|---|---|
| `00_frame_structure_demo` | 第一课时打印六个帧字段的三种进制并计算校验和 |
| `01_serial_read_char` | `available()`、`peek()`、`read()`、`write()` |
| `02_read_string_until` | `readStringUntil()`、`setTimeout()`、`trim()` |
| `03_data_frame` | 固定二进制帧接收、校验与应答 |
| `04_serial_song_switch` | 串口选择《两只老虎》或《小星星》 |
| `05_serial_breathing_led` | 串口启停、调速、设置 PWM 亮度 |
| `06_uart_board_a` | 文字命令转二进制帧，通过 `SerialFrame` 库和 `BoardSerial` 发送、接收应答 |
| `07_uart_board_b` | 通过 `SerialFrame` 库从 `BoardSerial` 收帧，控制 LED 并发送应答 |
| `08_data_frame_ping` | 8 月 10 日作业 PING 拓展的完整可运行工程 |
| A/B 工程中的 `SerialFrame.h` | 声明默认 USB 串口接口及可指定 `Stream` 串口对象的重载接口 |
| A/B 工程中的 `SerialFrame.cpp` | 统一完成组帧、发送、寻头、接收、校验和结果保存 |
| `SERIAL_FUNCTIONS.md` | Arduino/ESP32 常用串口函数手册 |
| `DATA_FRAME.md` | 数据帧结构、解析过程和常见错误 |
| `串口通信英语单词与函数笔记.md` | 面向英语基础薄弱学习者的单词、函数参数与记忆笔记 |
| `串口课堂代码详细讲解.md` | 8 月 10 日课堂代码逐段讲解、参数说明与作业衔接 |

## 串口工具设置

1. 字符读取和 `readStringUntil()` 示例使用普通串口监视器，波特率 `115200`。
2. 正式数据帧实验使用支持 HEX 发送和 HEX 显示的串口工具。
3. HEX 模式发送 `55 05 01 FF 05 FF`，不能把它当作普通文字发送。
4. 双板实验可在板 A 的普通串口监视器输入文字，板 A 会自动打包二进制帧。

## 单板外设接线

| 元件 | ESP32-S3 引脚 | 接法 |
|---|---:|---|
| 普通 LED | GPIO 2 | GPIO 2 → 220Ω 电阻 → LED 正极，LED 负极 → GND |
| 无源蜂鸣器驱动输入 | GPIO 48 | GPIO 48 → 蜂鸣器驱动电路输入 |
| 外设地线 | GND | 与 ESP32-S3 共地 |

> 建议使用带三极管驱动的无源蜂鸣器模块。不要用 GPIO 直接给工作电流较大的
> 裸蜂鸣器供电。

## 两块 ESP32-S3 的硬件串口接线

本例使用 ESP32-S3 自带的 `HardwareSerial` 创建 UART1，无需安装额外串口库；
RX 使用 GPIO 42，TX 使用 GPIO 41。

| 板 A | 连接 | 板 B |
|---|---|---|
| GPIO 41（TX） | → | GPIO 42（RX） |
| GPIO 42（RX） | ← | GPIO 41（TX） |
| GND | ↔ | GND |

两块板都直接在 `.ino` 主程序中创建并启动硬件串口：

```cpp
#define PIN_UART_RX 42
#define PIN_UART_TX 41

HardwareSerial BoardSerial(1);

void setup() {
  BoardSerial.begin(9600, SERIAL_8N1, PIN_UART_RX, PIN_UART_TX);
}
```

A/B 主程序把 `BoardSerial` 对象传给 `SerialFrame` 库，不再在 `.ino` 中重复编写
组帧、寻头和校验函数：

```cpp
sendFrame(BoardSerial, command, data);

if (readFrame(BoardSerial)) {
  byte command = getFrameCommand();
  byte data = getFrameData();
}
```

不传串口对象的 `sendFrame(command, data)` 和 `readFrame()` 仍默认使用 USB 串口
`Serial`，因此原有单板数据帧工程保持兼容。

板 A 的 USB 串口负责接收 `led 255`、`led 0`、`ping` 等文字命令；A/B 两板之间
不直接传输这些文字，只通过硬件串口发送 `55 05 CMD DATA CHECKSUM FF` 固定二进制帧。

如果自己的开发板没有引出 GPIO 41、GPIO 42，可修改 `PIN_UART_RX` 和
`PIN_UART_TX`，但仍要保证 TX 接对方 RX、RX 接对方 TX。

硬件串口由 ESP32-S3 的 UART 外设完成收发；课堂实验统一使用便于观察的 `9600` 波特率和 `SERIAL_8N1` 数据格式。

## 实验命令表

| HEX 数据帧 | 作用 |
|---|---|
| `55 05 01 FF 05 FF` | LED 亮度设为 255 |
| `55 05 01 00 06 FF` | LED 关闭 |
| `55 05 02 01 08 FF` | 播放《两只老虎》 |
| `55 05 02 02 09 FF` | 播放《小星星》 |
| `55 05 03 01 09 FF` | 启动呼吸灯 |
| `55 05 03 00 08 FF` | 停止呼吸灯 |
| `55 05 04 14 1D FF` | 呼吸灯每步延时 20 ms |
| `55 05 05 00 0A FF` | PING 测试 |

## 常见问题

- **没有任何输出：** 检查串口端口和波特率是否为 `115200`。
- **HEX 帧没有响应：** 检查是否启用了 HEX 发送，并重新计算加法校验。
- **文字示例一直等到超时：** 串口监视器可能没有发送 `\n`。
- **双板通信没有回应：** 检查 TX/RX 是否交叉、是否共地、两端是否都为 `9600`。
- **双板通信出现乱码：** 通常是波特率不一致或接线接触不良。
- **板载 LED 逻辑相反：** 某些开发板的 LED 为低电平点亮，可交换 `HIGH` 和 `LOW`。
- **发送歌曲命令后暂时不能处理新命令：** 歌曲示例使用 `delay()` 顺序播放，
  新命令会在当前歌曲播放完后处理；后续学习非阻塞定时后可继续改进。
