# Arduino / ESP32 常用串口函数

## 1. 串口对象从哪里来

Arduino 程序通常不需要为 `Serial` 单独创建对象。ESP32-S3 常见的串口对象有：

- `Serial`：通常连接电脑，用于串口监视器调试。
- `Serial0`、`Serial1`、`Serial2`：硬件 UART，具体可用数量和默认引脚取决于芯片及核心版本。
- `EspSoftwareSerial`：使用普通 GPIO 和程序模拟串口收发。

本课程的双板通信使用：

```cpp
#include <SoftwareSerial.h>

SoftwareSerial SoftSerial(42, 41);

void setup() {
  SoftSerial.begin(9600);
}
```

软串口对象也提供常用的输入和输出函数：

```text
Print：print、println、write、printf
  ↑
Stream：available、read、peek、readStringUntil、parseInt、setTimeout
  ↑
软串口：begin、end、flush
```

## 2. 初始化与关闭

### `begin()`

连接串口监视器时常用：

```cpp
Serial.begin(115200);
```

本课程的软串口指定格式和引脚时使用：

```cpp
SoftwareSerial SoftSerial(PIN_RX, PIN_TX);

void setup() {
  SoftSerial.begin(9600);
}
```

- `9600`：每秒传输的符号数，本课程可近似理解为波特率。
- `PIN_RX`：软串口接收引脚。
- `PIN_TX`：软串口发送引脚。
- 默认使用 8 个数据位、无校验、1 个停止位。
- 两端的波特率和数据格式必须相同。

### `end()`

```cpp
SoftSerial.end();
```

停止这个串口。普通课堂程序通常从开机一直使用串口，不需要调用它。

## 3. 判断是否收到数据

### `available()`

```cpp
int byteCount = Serial.available();
```

返回接收缓冲区中可以读取的字节数。它返回的是字节数，不是“有几条消息”。

正确的基本写法：

```cpp
if (Serial.available() > 0) {
  int receivedByte = Serial.read();
}
```

### `availableForWrite()`

```cpp
int freeCount = Serial.availableForWrite();
```

返回发送缓冲区还能放入多少字节。大量、连续发送数据时才常用，入门实验了解即可。

## 4. 单字节读取

### `read()`

```cpp
int receivedByte = Serial.read();
```

从接收缓冲区取走一个字节。返回类型是 `int`，原因是没有数据时会返回 `-1`。
因此最好先用 `available()` 判断，再调用 `read()`。

字节是字符时可以转换为 `char`：

```cpp
char receivedChar = (char)Serial.read();
```

### `peek()`

```cpp
int nextByte = Serial.peek();
```

查看下一个字节，但不把它从接收缓冲区取走。没有数据时返回 `-1`。

`peek()` 像“看一眼”，`read()` 像“拿出来”。连续调用两次 `peek()` 会看到同一个字节，
连续调用两次 `read()` 会取出两个不同的字节。

## 5. 字符串和整行读取

### `readString()`

```cpp
String message = Serial.readString();
```

持续读取字符，直到一段时间没有新字符并触发超时。它不知道一条消息应该在哪里结束，
因此不适合边界明确的数据帧。

### `readStringUntil()`

```cpp
String message = Serial.readStringUntil('\n');
```

读取字符，直到遇到指定结束符或等待超时。结束符会被读取，但不会放入返回的字符串。

如果发送端使用 `\r\n`，以 `\n` 为结束符读取后，字符串末尾仍可能留有 `\r`，
所以通常紧接着写：

```cpp
message.trim();
```

典型写法：

```cpp
if (Serial.available() > 0) {
  String frame = Serial.readStringUntil('\n');
  frame.trim();
  Serial.println(frame);
}
```

注意：`available() > 0` 只说明至少到了一个字节，不保证整行已经到齐。
`readStringUntil()` 仍可能等待剩余内容，直到读到 `\n` 或超时。

### `setTimeout()`

```cpp
Serial.setTimeout(100);
```

把需要等待数据的读取函数最长等待时间设为 100 ms。它会影响：

- `readString()`
- `readStringUntil()`
- `readBytes()`
- `readBytesUntil()`
- `parseInt()`
- `parseFloat()`
- `find()`

默认超时通常是 1000 ms。超时太大会让程序显得卡顿，太小则可能在慢速传输时只读到
半条数据。课堂短命令可先使用 `100 ms`。

## 6. 读取到字符数组

### `readBytes()`

```cpp
char buffer[20];
int count = Serial.readBytes(buffer, 19);
buffer[count] = '\0';
```

最多读取指定数量的字节，或在超时后停止。它不会自动在末尾添加 C 字符串结束符
`'\0'`，把缓冲区当字符串使用前要自己添加。

### `readBytesUntil()`

```cpp
char buffer[20];
int count = Serial.readBytesUntil('\n', buffer, 19);
buffer[count] = '\0';
```

读取到结束符、缓冲区已满或超时为止。结束符不会被写入 `buffer`。

课堂前期用 `String` 更直观；需要长时间稳定运行或严格控制内存时，再学习字符数组。

## 7. 直接读取数字

### `parseInt()`

```cpp
int number = Serial.parseInt();
```

跳过非数字字符，读取一个整数，遇到无法继续组成数字的字符或超时后返回。

### `parseFloat()`

```cpp
float number = Serial.parseFloat();
```

读取一个小数。它和 `parseInt()` 一样可能等待超时。

这两个函数适合快速文字实验，但不适合正式二进制帧：读取失败和真正收到数字 `0`
都可能得到 `0`。正式协议应逐字节接收，并检查帧头、长度、校验位和帧尾。

## 8. 查找指定内容

### `find()`

```cpp
bool found = Serial.find("START");
```

持续读取，直到找到指定文字或超时。搜索过程中经过的数据会被从缓冲区取走。
如果后续还需要完整数据帧，不应先随意调用 `find()`。

### `findUntil()`

```cpp
bool found = Serial.findUntil("LED", "\n");
```

在遇到终止字符串 `\n` 之前查找 `LED`。用于简单文本搜索，结构化命令仍建议先读取
完整一行再解析。

## 9. 文本输出

### `print()`

```cpp
Serial.print("value = ");
Serial.print(123);
```

把数据转换成便于阅读的文本发送，不自动换行。

常用进制格式：

```cpp
Serial.println(255, DEC);  // 255
Serial.println(255, HEX);  // FF
Serial.println(255, BIN);  // 11111111
```

小数位数：

```cpp
Serial.println(3.14159, 2);  // 3.14
```

### `println()`

```cpp
Serial.println("finished");
```

发送内容后再发送换行。`Serial.println()` 不带参数时只发送换行。

### `printf()`

ESP32 Arduino 支持格式化输出：

```cpp
int value = 128;
Serial.printf("value = %d\n", value);
```

它很灵活，但格式符较多，初学阶段优先使用 `print()` 和 `println()`。某些非 ESP32
Arduino 开发板不一定支持相同的 `printf()`。

## 10. 原始字节输出

### `write()`

```cpp
Serial.write(65);
```

发送数值为 65 的一个原始字节，串口监视器会把它显示为字符 `A`。

对比：

```cpp
Serial.print(65);  // 发送字符 '6' 和 '5'，共两个字节
Serial.write(65);  // 发送二进制值 65，共一个字节
```

发送字节数组：

```cpp
byte data[] = {0xAA, 0x01, 0x55};
Serial.write(data, 3);
```

`write()` 适合二进制协议；`print()` 适合串口监视器和入门文本协议。

## 11. `flush()` 的真实作用

```cpp
Serial.flush();
```

在当前 Arduino 实现中，它通常等待已经写入发送缓冲区的数据发送完毕。
它不是“清空接收缓冲区”。不要用它删除收到但尚未读取的数据。

如果课堂实验确实要丢弃当前接收数据，可逐字节读取：

```cpp
while (Serial.available() > 0) {
  Serial.read();
}
```

## 12. 选择函数的简单规则

| 目标 | 建议函数 |
|---|---|
| 读取一个按键字符 | `available()` + `read()` |
| 偷看下一个字节 | `peek()` |
| 读取以换行结尾的命令 | `readStringUntil('\n')` |
| 快速读取一个整数 | `parseInt()` |
| 读取人工输入的文字命令 | `readStringUntil()` + `trim()` |
| 解析正式二进制帧 | `read()` + 字节数组 + 完整校验 |
| 输出调试文字 | `print()`、`println()` |
| 格式化输出多项数据 | ESP32 的 `printf()` |
| 发送二进制数据 | `write()` |
| 缩短阻塞等待 | `setTimeout()` |
| 等待发送完成 | `flush()` |
