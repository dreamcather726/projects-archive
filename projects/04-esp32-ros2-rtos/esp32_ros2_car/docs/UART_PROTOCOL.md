# UART0 标准帧发送测试

## 当前程序

程序暂时只做一件事：UART0 每 200 ms 发送一帧 `FUNC=0x01` 的标准二进制帧。

| 项目 | 当前值 |
|---|---|
| 串口 | UART0，保留当前控制台引脚 |
| 参数 | 115200 8N1 |
| 发送周期 | 200 ms |
| 功能码 | `0x01` |
| Payload | 4 字节递增计数器，小端序 |
| 最大 Payload | 32 字节 |

MPU6050、编码器、PID、速度命令、接收任务和接收队列均未包含在当前串口模块中。

## 帧结构

```text
AA | LEN | FUNC | PAYLOAD... | CRC_L | CRC_H | BB
```

| 字段 | 长度 | 当前测试值 |
|---|---:|---|
| 帧头 | 1 | `AA` |
| LEN | 1 | `05`，即一个 FUNC 加四个 Payload 字节 |
| FUNC | 1 | `01` |
| Payload | 4 | 递增的 `uint32_t` 计数器 |
| CRC | 2 | CRC16-Modbus，低字节先发送 |
| 帧尾 | 1 | `BB` |

CRC 初始值为 `0xFFFF`，多项式为 `0xA001`，计算范围是 `LEN + FUNC + PAYLOAD`。

前几帧为：

```text
计数器 0：AA 05 01 00 00 00 00 3D 8E BB
计数器 1：AA 05 01 01 00 00 00 3C 72 BB
计数器 2：AA 05 01 02 00 00 00 3C 36 BB
```

## 保留的接口

| 函数 | 作用 |
|---|---|
| `uart_protocol_init()` | 初始化 UART0，115200 8N1 |
| `uart_protocol_crc16()` | 计算 CRC16-Modbus |
| `uart_protocol_build_frame()` | 根据 FUNC 和 Payload 生成完整标准帧 |
| `uart_protocol_send_frame()` | 组帧并同步发送，发送过程有互斥锁保护 |

主程序的发送代码：

```cpp
uint8_t payload[4];
write_u32_le(payload, frame_counter++);

ESP_ERROR_CHECK(uart_protocol_send_frame(
    0x01,
    payload,
    sizeof(payload),
    100));
```

## 测试注意事项

当前 UART0 同时用于 ESP-IDF 日志，因此上电开始阶段可能先看到文本日志，随后每 200 ms 出现一帧二进制数据。普通文本串口监视器可能把二进制显示成乱码，应使用支持十六进制接收显示的串口工具。
