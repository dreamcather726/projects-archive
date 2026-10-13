# 第二模块 实验

> 上课前先完成 `1_Introduce.md` 开头“串口通信知识点总览”的讲解。8 月 10 日课堂按实验 1、实验 3 的协议代码和 PING 扩展为主；实验 2、4～7 可作为后续串口实验课内容。教师逐行讲解稿见 `串口课堂代码详细讲解.md`。

## 实验1

### 1. 实验1预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 本实验重点是理解“串口缓冲区里到底收到了什么字节”。
  - `peek()` 只看不取，`read()` 会把字节真正取走。
  - 一个字符发送到串口后，本质上还是一个字节数值。
- 本实验新增的术语、模块或函数：
  - `Serial.available()`
  - `Serial.peek()`
  - `Serial.read()`
  - `Serial.write()`
- 开始本实验前需要特别提醒的点：
  - 串口监视器波特率设置为 `115200`。
  - 可以发送字符 `A`、数字或回车换行来观察差异。

### 2. 实验1接线
- 接线表：

| 元件 | ESP32-S3 引脚 | 接法 |
| --- | --- | --- |
| USB 数据线 | USB 接口 | 连接电脑，打开串口监视器 |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：逐个读取电脑发送的串口字符，并显示字符及其 ASCII 数值。
  使用方法：打开 115200 串口监视器，发送示例字符 A，也可以发送数字或回车换行。
  运行结果：发送 A 时显示字符 A 和 ASCII 数值 65，同时显示 available、peek、read 结果。
*/

void setup() {
  // 启动 USB 串口
  Serial.begin(115200);

  // 给出提示信息
  Serial.println("请输入任意字符：");
}

void loop() {
  // available() 返回接收缓冲区中还没有读取的字节数
  if (Serial.available() > 0) {
    int byteCount = Serial.available();

    // peek() 只查看下一个字节，不把它从缓冲区取走
    int nextByte = Serial.peek();

    // read() 读取一个字节，并把它从缓冲区取走
    int receivedByte = Serial.read();

    Serial.print("待读字节数：");
    Serial.print(byteCount);
    Serial.print("，peek：");
    Serial.print(nextByte);
    Serial.print("，read：");
    Serial.print(receivedByte);
    Serial.print("，字符：");

    // 特殊处理回车和换行，方便课堂观察
    if (receivedByte == '\r') {
      Serial.println("[回车]");
    } else if (receivedByte == '\n') {
      Serial.println("[换行]");
    } else {
      // write() 直接发送这个字节本身
      Serial.write(receivedByte);
      Serial.println();
    }
  }
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 启动串口并打印提示信息。
  2. 持续检查缓冲区中是否收到新字节。
  3. 先用 `available()` 查看当前还有多少字节可读。
  4. 再用 `peek()` 看下一个字节是什么。
  5. 最后用 `read()` 把这个字节真正取出来并显示。
- `setup()` 的作用：
  - 初始化串口并打印引导文字。
- `loop()` 的作用：
  - 持续接收和显示串口中的单个字节。
- 数据输入、处理、输出的执行过程：
  - 输入：电脑发送到串口监视器的一个字节。
  - 处理：程序分别查看、读取并判断这个字节是不是回车或换行。
  - 输出：把字节数、数值和字符显示在串口监视器中。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `Serial.available()`
  - `Serial.peek()`
  - `Serial.read()`
  - `Serial.write(receivedByte)`
- 每个函数的作用、写法、参数和返回值：
  - `Serial.available()`
    - 函数作用：返回接收缓冲区里当前可读取的字节数。
    - 参数：无。
    - 返回值：整数，表示还没读取的字节数量。
  - `Serial.peek()`
    - 函数作用：查看下一个字节，但不把它取走。
    - 参数：无。
    - 返回值：下一个字节的数值；如果没有数据，返回 `-1`。
  - `Serial.read()`
    - 函数作用：读取一个字节，并把它从缓冲区移走。
    - 参数：无。
    - 返回值：读取到的字节数值；如果没有数据，返回 `-1`。
  - `Serial.write(receivedByte)`
    - 函数作用：把原始字节直接发回串口。
    - 参数1：要发送的字节数值。
    - 返回值：发送的字节数。
- 关键代码片段分析：
  - `peek()` 和 `read()` 连续使用，可以清楚展示“只看不取”和“真正取走”的区别。
- 为什么这样写：
  - 串口通信的底层本质就是“一个字节一个字节地进出缓冲区”，这个实验是后面所有协议实验的基础。

---

## 实验2

### 1. 实验2预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 本实验从“读一个字符”升级为“读一整行文字”。
  - `readStringUntil('\n')` 会一直读到换行符或超时为止。
  - 如果发送端带有回车 `\r`，通常要用 `trim()` 清掉尾部残留字符。
- 本实验新增的术语、模块或函数：
  - `readStringUntil()`
  - `setTimeout()`
  - `trim()`
  - `String`
- 开始本实验前需要特别提醒的点：
  - 串口监视器波特率设置为 `115200`。
  - 换行方式建议选择 `Newline` 或 `Both NL & CR`。

### 2. 实验2接线
- 接线表：

| 元件 | ESP32-S3 引脚 | 接法 |
| --- | --- | --- |
| USB 数据线 | USB 接口 | 连接电脑，打开串口监视器 |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：使用 readStringUntil() 读取一行文字，并根据文字内容进行回复。
  使用方法：串口设为 115200 和 Newline，发送示例文字 hello、name 或 QingChao。
  运行结果：hello 回复“你好”，name 回复程序名称，其他文字会被原样显示。
*/

void setup() {
  // 启动串口通信
  Serial.begin(115200);

  // readStringUntil() 最多等待 100 毫秒，避免长时间卡住程序
  Serial.setTimeout(100);

  Serial.println("请选择串口监视器的 Newline 或 Both NL & CR：");
  Serial.println("输入 hello、name 或其他文字，然后点击发送。");
}

void loop() {
  if (Serial.available() > 0) {
    // 读取到换行符为止，返回的字符串中不包含换行符
    String message = Serial.readStringUntil('\n');

    // trim() 去掉字符串两端的空格、回车和换行
    message.trim();

    if (message.length() == 0) {
      return;
    }

    // 根据不同的文字命令返回不同回复
    if (message == "hello") {
      Serial.println("ESP32：你好！");
    } else if (message == "name") {
      Serial.println("ESP32：我是串口实验程序。");
    } else {
      Serial.print("ESP32 收到：");
      Serial.println(message);
    }
  }
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 启动串口并设置读取超时时间。
  2. 检查串口中是否收到数据。
  3. 用 `readStringUntil('\n')` 读取一整行文字。
  4. 用 `trim()` 去掉回车、换行和两端空格。
  5. 根据文字内容判断要回复什么。
- `setup()` 的作用：
  - 初始化串口、超时设置和提示信息。
- `loop()` 的作用：
  - 不断接收整行文字命令并返回对应回复。
- 数据输入、处理、输出的执行过程：
  - 输入：电脑发送的一整行文字。
  - 处理：程序读取、清理并比较字符串内容。
  - 输出：向串口监视器发送回复文字。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `Serial.readStringUntil('\n')`
  - `Serial.setTimeout(100)`
  - `message.trim()`
  - `message == "hello"`
- 每个函数的作用、写法、参数和返回值：
  - `Serial.readStringUntil('\n')`
    - 函数作用：读取字符串，直到遇到指定结束符或超时。
    - 参数1：`'\n'`，表示以换行作为一条命令的结束标志。
    - 返回值：返回读取到的 `String` 字符串。
  - `Serial.setTimeout(100)`
    - 函数作用：设置需要等待数据的串口函数最长等待 `100ms`。
    - 参数1：`100`，超时时间，单位毫秒。
    - 返回值：无返回值。
  - `message.trim()`
    - 函数作用：去掉字符串两端的空白字符、回车和换行。
    - 参数：无。
    - 返回值：无返回值，直接修改原字符串。
- 关键代码片段分析：
  - `if (message.length() == 0) return;` 用来过滤掉空行，避免误判。
- 为什么这样写：
  - 相比单字节读取，按行读取更符合人工输入文字命令的使用习惯。

---

## 实验3

### 1. 实验3预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 从这个实验开始，串口数据不再是普通文字，而是固定格式的二进制帧。
  - 数据帧结构固定为 `55 05 CMD DATA CHECKSUM FF`。
  - 校验正确前，程序不能执行外设动作。
- 本实验新增的术语、模块或函数：
  - `readFrame()`
  - `sendFrame()`
  - `getFrameCommand()`
  - `getFrameData()`
- 开始本实验前需要特别提醒的点：
  - 串口工具要使用 `115200`、HEX 发送和 HEX 显示。
  - 示例数据帧 `55 05 01 FF 05 FF` 表示把 LED 亮度设置为 `255`。

### 2. 实验3接线
- 接线表：

| 元件 | ESP32-S3 引脚 | 接法 |
| --- | --- | --- |
| LED | `GPIO 2` | `GPIO 2 -> 电阻 -> LED 正极` |
| LED 地线 | `GND` | LED 负极接 `GND` |
| USB 数据线 | USB 接口 | 连接电脑，使用 HEX 串口工具 |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：接收并校验固定格式的数据帧，根据指令设置 LED 亮度并返回应答帧。
  使用方法：串口设为 115200、HEX 发送和显示；示例发送 55 05 01 FF 05 FF。
  运行结果：GPIO 2 的 LED 亮度变为 255，并应答 55 05 81 FF 85 FF。
*/

#include "SerialFrame.h"

// 0x01 表示设置 LED 亮度，数据范围为 0x00～0xFF
#define CMD_LED 0x01
#define CMD_ERROR 0x7F

#define PIN_LED 2

void setup() {
  // 启动串口，并初始化 LED 引脚
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);
}

void loop() {
  // 只有在收到并校验通过完整数据帧后，readFrame() 才返回 true
  if (readFrame()) {
    byte command = getFrameCommand();
    byte data = getFrameData();

    if (command == CMD_LED) {
      // data 直接作为 PWM 亮度值
      analogWrite(PIN_LED, data);

      // 0x81 是 0x01 指令的成功应答，数据原样返回
      sendFrame(0x81, data);
    } else {
      // 错误帧：0x01 表示不支持这个指令
      sendFrame(CMD_ERROR, 0x01);
    }
  }
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 启动串口并初始化 LED 引脚。
  2. 在循环中不断调用 `readFrame()`。
  3. 当 `readFrame()` 返回 `true` 时，说明已经收到一条校验通过的完整数据帧。
  4. 读取指令字节和数据字节。
  5. 如果是 LED 指令，就设置亮度并返回成功应答；否则返回错误应答。
- `setup()` 的作用：
  - 初始化串口和 LED。
- `loop()` 的作用：
  - 持续接收数据帧、解析命令并回发应答。
- 数据输入、处理、输出的执行过程：
  - 输入：HEX 工具发送的 6 字节数据帧。
  - 处理：协议层先做帧头、长度、校验和帧尾检查，主程序再判断命令类型。
  - 输出：LED 亮度变化，并向串口发送应答帧。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `readFrame()`
  - `getFrameCommand()`
  - `getFrameData()`
  - `sendFrame(0x81, data)`
- 每个函数的作用、写法、参数和返回值：
  - `readFrame()`
    - 函数作用：检查串口里是否已经收到一条完整且正确的数据帧。
    - 参数：无。
    - 返回值：`true` 表示接收到合法数据帧，`false` 表示还没有或校验失败。
  - `getFrameCommand()`
    - 函数作用：取得上一条合法数据帧中的指令字节。
    - 参数：无。
    - 返回值：返回 `CMD` 字节。
  - `getFrameData()`
    - 函数作用：取得上一条合法数据帧中的数据字节。
    - 参数：无。
    - 返回值：返回 `DATA` 字节。
  - `sendFrame(0x81, data)`
    - 函数作用：按照协议自动补齐帧头、长度、校验和帧尾，并发送应答帧。
    - 参数1：应答指令字节。
    - 参数2：应答数据字节。
    - 返回值：无返回值。
- 关键代码片段分析：
  - `if (readFrame()) { ... }` 表示“先校验通过，再执行控制”，这是正式协议最关键的思路。
- 为什么这样写：
  - 正式通信不能像文字命令那样收到内容就执行，必须先做完整性和正确性检查。

---

## 实验4

### 1. 实验4预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 本实验把数据帧和前面学过的无源蜂鸣器播放歌曲结合起来。
  - 同一个指令类型 `CMD_SONG`，通过不同数据值区分不同歌曲。
  - 歌曲播放函数 `playSong()` 会顺序遍历音高数组和时长数组。
- 本实验新增的术语、模块或函数：
  - `CMD_SONG`
  - `playSong()`
  - `tigerMelody[]`
  - `starMelody[]`
- 开始本实验前需要特别提醒的点：
  - 串口工具使用 `115200` 和 HEX 模式。
  - 无源蜂鸣器接 `GPIO 48`。

### 2. 实验4接线
- 接线表：

| 元件 | ESP32-S3 引脚 | 接法 |
| --- | --- | --- |
| 无源蜂鸣器驱动输入 | `GPIO 48` | 控制端接 `GPIO 48` |
| 无源蜂鸣器地线 | `GND` | 与开发板共地 |
| USB 数据线 | USB 接口 | 连接电脑，使用 HEX 串口工具 |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：接收串口数据帧，控制 GPIO 48 的无源蜂鸣器切换播放两首歌曲。
  使用方法：串口设为 115200 和 HEX；发送 55 05 02 01 08 FF 或 55 05 02 02 09 FF。
  运行结果：分别播放《两只老虎》或《小星星》，并返回对应的 0x82 应答帧。
*/

#include "SerialFrame.h"

// 0x02 表示选择歌曲，数据 0x01 和 0x02 分别代表两首歌
#define CMD_SONG 0x02
#define CMD_ERROR 0x7F

// 无源蜂鸣器的驱动输入连接 GPIO 48
#define PIN_BUZZER 48

// 《两只老虎》的音高数组
const int tigerMelody[] = {
  262, 294, 330, 262,
  262, 294, 330, 262,
  330, 349, 392,
  330, 349, 392,
  392, 440, 392, 349, 330, 262,
  392, 440, 392, 349, 330, 262,
  262, 196, 262,
  262, 196, 262
};

// 《两只老虎》中每个音符的持续时间
const int tigerTime[] = {
  400, 400, 400, 400,
  400, 400, 400, 400,
  400, 400, 800,
  400, 400, 800,
  200, 200, 200, 200, 400, 400,
  200, 200, 200, 200, 400, 400,
  400, 400, 800,
  400, 400, 800
};

// 《小星星》的音高数组
const int starMelody[] = {
  262, 262, 392, 392, 440, 440, 392,
  349, 349, 330, 330, 294, 294, 262,
  392, 392, 349, 349, 330, 330, 294,
  392, 392, 349, 349, 330, 330, 294,
  262, 262, 392, 392, 440, 440, 392,
  349, 349, 330, 330, 294, 294, 262
};

// 《小星星》中每个音符的持续时间
const int starTime[] = {
  400, 400, 400, 400, 400, 400, 800,
  400, 400, 400, 400, 400, 400, 800,
  400, 400, 400, 400, 400, 400, 800,
  400, 400, 400, 400, 400, 400, 800,
  400, 400, 400, 400, 400, 400, 800,
  400, 400, 400, 400, 400, 400, 800
};

const int TIGER_COUNT = sizeof(tigerMelody) / sizeof(tigerMelody[0]);
const int STAR_COUNT = sizeof(starMelody) / sizeof(starMelody[0]);

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BUZZER, OUTPUT);
}

void loop() {
  if (readFrame()) {
    byte command = getFrameCommand();
    byte data = getFrameData();

    if (command != CMD_SONG) {
      sendFrame(CMD_ERROR, 0x01);
    } else if (data == 0x01) {
      sendFrame(0x82, data);
      playSong(tigerMelody, tigerTime, TIGER_COUNT);
    } else if (data == 0x02) {
      sendFrame(0x82, data);
      playSong(starMelody, starTime, STAR_COUNT);
    } else {
      sendFrame(CMD_ERROR, 0x02);
    }
  }
}

void playSong(const int melody[], const int noteTime[], int noteCount) {
  // 依次播放数组中的每一个音符
  for (int i = 0; i < noteCount; i = i + 1) {
    tone(PIN_BUZZER, melody[i], noteTime[i]);
    delay(noteTime[i] + 60);
    noTone(PIN_BUZZER);
  }
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 初始化串口和蜂鸣器引脚。
  2. 接收一条合法数据帧。
  3. 判断是不是歌曲控制指令。
  4. 再判断数据字节是歌曲 1 还是歌曲 2。
  5. 返回成功或错误应答。
  6. 调用 `playSong()` 顺序播放整首歌曲。
- `setup()` 的作用：
  - 初始化串口和蜂鸣器输出引脚。
- `loop()` 的作用：
  - 持续接收歌曲控制帧并决定播放哪一首。
- 数据输入、处理、输出的执行过程：
  - 输入：HEX 串口工具发送的歌曲控制数据帧。
  - 处理：校验后判断指令和数据是否合法，并选择对应数组。
  - 输出：蜂鸣器播放歌曲，并返回应答帧。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `if (command != CMD_SONG)`
  - `playSong(tigerMelody, tigerTime, TIGER_COUNT)`
  - `const int melody[]`
  - `tone(PIN_BUZZER, melody[i], noteTime[i])`
- 每个函数的作用、写法、参数和返回值：
  - `playSong(const int melody[], const int noteTime[], int noteCount)`
    - 函数作用：按数组顺序播放整首歌。
    - 参数1：音高数组。
    - 参数2：时长数组。
    - 参数3：音符个数。
    - 返回值：无返回值。
  - `tone(PIN_BUZZER, melody[i], noteTime[i])`
    - 函数作用：让无源蜂鸣器按照当前音符频率和时长发声。
    - 参数1：蜂鸣器引脚。
    - 参数2：当前音符频率。
    - 参数3：当前音符时长。
    - 返回值：无返回值。
- 关键代码片段分析：
  - `command` 决定“是不是歌曲功能”，`data` 决定“播放哪首歌”，这是“一个指令类型 + 一个数据字节”的典型用法。
- 为什么这样写：
  - 串口协议不需要为每首歌都单独定义一个命令，只要用同一个命令配合不同数据即可扩展功能。

---

## 实验5

### 1. 实验5预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 本实验把串口协议和 LED 呼吸灯结合起来。
  - 呼吸灯运行过程中，程序还需要继续检查串口是否收到新命令。
  - `CMD_LED`、`CMD_BREATH`、`CMD_SPEED` 分别对应亮度、启停和速度控制。
- 本实验新增的术语、模块或函数：
  - `runCommand()`
  - `breathing`
  - `brightness`
  - `speedDelay`
- 开始本实验前需要特别提醒的点：
  - 使用 `115200` 和 HEX 模式发送。
  - LED 接 `GPIO 2`。

### 2. 实验5接线
- 接线表：

| 元件 | ESP32-S3 引脚 | 接法 |
| --- | --- | --- |
| LED | `GPIO 2` | `GPIO 2 -> 电阻 -> LED 正极` |
| LED 地线 | `GND` | LED 负极接 `GND` |
| USB 数据线 | USB 接口 | 连接电脑，使用 HEX 串口工具 |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：通过串口数据帧设置 LED 亮度，并控制呼吸灯的启停和速度。
  使用方法：串口设为 115200 和 HEX；示例发送 55 05 03 01 09 FF 启动呼吸灯。
  运行结果：GPIO 2 的 LED 开始呼吸，并应答 55 05 83 01 89 FF。
*/

#include "SerialFrame.h"

#define CMD_LED 0x01
#define CMD_BREATH 0x03
#define CMD_SPEED 0x04
#define CMD_ERROR 0x7F

#define PIN_LED 2

// 呼吸灯运行状态和亮度参数
bool breathing = false;
int brightness = 0;
int changeValue = 5;
int speedDelay = 20;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);
}

void loop() {
  // 先处理新的串口命令
  if (readFrame()) {
    byte command = getFrameCommand();
    byte data = getFrameData();
    runCommand(command, data);
  }

  // 如果呼吸灯处于运行状态，就持续更新亮度
  if (breathing == true) {
    analogWrite(PIN_LED, brightness);
    brightness = brightness + changeValue;

    if (brightness >= 255) {
      brightness = 255;
      changeValue = -5;
    }

    if (brightness <= 0) {
      brightness = 0;
      changeValue = 5;
    }

    delay(speedDelay);
  }
}

void runCommand(byte command, byte data) {
  if (command == CMD_LED) {
    // 手动亮度设置时先关闭呼吸模式
    breathing = false;
    brightness = data;
    analogWrite(PIN_LED, brightness);
    sendFrame(0x81, data);
  } else if (command == CMD_BREATH && data <= 1) {
    // data 为 0 或 1，分别表示关闭或开启呼吸灯
    breathing = data;
    if (breathing == false) {
      analogWrite(PIN_LED, 0);
    }
    sendFrame(0x83, data);
  } else if (command == CMD_SPEED && data >= 2 && data <= 100) {
    // 设置每一步亮度变化之间的延时
    speedDelay = data;
    sendFrame(0x84, data);
  } else {
    sendFrame(CMD_ERROR, 0x02);
  }
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 初始化串口和 LED 引脚。
  2. 循环中先检查是否收到新数据帧。
  3. 收到后调用 `runCommand()` 按不同命令执行控制。
  4. 如果呼吸模式开启，就持续更新亮度值。
  5. 当亮度到达最大值或最小值时，改变变化方向。
- `setup()` 的作用：
  - 初始化串口和 LED。
- `loop()` 的作用：
  - 同时处理串口命令和呼吸灯动画更新。
- 数据输入、处理、输出的执行过程：
  - 输入：HEX 数据帧。
  - 处理：解析命令类型，修改运行状态变量。
  - 输出：LED 亮度变化，并返回对应应答帧。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `runCommand(command, data)`
  - `if (breathing == true)`
  - `brightness = brightness + changeValue`
  - `speedDelay = data`
- 每个函数的作用、写法、参数和返回值：
  - `runCommand(byte command, byte data)`
    - 函数作用：根据收到的指令和数据执行具体控制逻辑。
    - 参数1：指令类型。
    - 参数2：指令数据。
    - 返回值：无返回值。
  - `analogWrite(PIN_LED, brightness)`
    - 函数作用：输出当前亮度对应的 PWM 数值。
    - 参数1：LED 引脚。
    - 参数2：当前亮度值。
    - 返回值：无返回值。
- 关键代码片段分析：
  - `brightness` 和 `changeValue` 的配合决定了呼吸灯的“来回变化”。
- 为什么这样写：
  - 这个实验把“协议控制”和“持续运行的动画”放在同一个循环中，是后面更复杂串口控制程序的基础。

---

## 实验6

### 1. 实验6预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 这是双板通信中的发送端程序。
  - 板 A 负责把“人类可读的文字命令”转换成“机器可读的二进制帧”。
  - 这里除了 `Serial`，还新增了 `SoftwareSerial` 作为板间通信通道。
- 本实验新增的术语、模块或函数：
  - `SoftwareSerial SoftSerial(...)`
  - `sendSoftFrame()`
  - `readSoftFrame()`
  - `startsWith()`
- 开始本实验前需要特别提醒的点：
  - 板 A 和板 B 的 `TX/RX` 要交叉连接并共地。
  - 软串口波特率统一为 `9600`。
  - 板 A 通过 USB 串口和电脑通信，波特率仍为 `115200`。

### 2. 实验6接线
- 接线表：

| 板 A | 连接 | 板 B |
| --- | --- | --- |
| `GPIO 41 (TX)` | → | `GPIO 42 (RX)` |
| `GPIO 42 (RX)` | ← | `GPIO 41 (TX)` |
| `GND` | ↔ | `GND` |
| USB | 连接电脑 | USB 可独立供电 |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：板 A 把电脑输入的 LED 或 PING 命令转换成数据帧，通过软串口发送给板 B。
  使用方法：板间 TX/RX 交叉并共地；打开板 A 的 115200 串口，输入 led 255 或 ping。
  运行结果：led 255 使板 B 的 LED 最亮；ping 会收到板 B 的 0x85、0x00 回复。
*/

#include <SoftwareSerial.h>
#include "SerialFrame.h"

#define PIN_UART_RX 42
#define PIN_UART_TX 41

#define CMD_LED 0x01
#define CMD_PING 0x05

// 创建软串口：第一个是 RX，第二个是 TX
SoftwareSerial SoftSerial(PIN_UART_RX, PIN_UART_TX);

// 保存板 B 回复的指令和数据
byte frameCommand = 0;
byte frameData = 0;

void setup() {
  // USB 串口：连接电脑的串口监视器
  Serial.begin(115200);

  // 直接启动软串口，板 A 和板 B 都使用 9600
  SoftSerial.begin(9600);

  Serial.println("输入 led 255 设置板 B 的亮度，输入 ping 测试通信");
}

void loop() {
  // 电脑输入文字，板 A 把它转换成二进制数据帧
  if (Serial.available() > 0) {
    String message = Serial.readStringUntil('\n');
    message.trim();

    if (message.startsWith("led ")) {
      int value = message.substring(4).toInt();
      value = constrain(value, 0, 255);
      sendSoftFrame(CMD_LED, value);
      Serial.println("板 A 已发送 LED 指令");
    } else if (message == "ping") {
      sendSoftFrame(CMD_PING, 0x00);
      Serial.println("板 A 已发送 PING 指令");
    } else {
      Serial.println("命令格式错误");
    }
  }

  // 板 A 从软串口接收板 B 的二进制应答
  if (readSoftFrame()) {
    Serial.printf("板 B 回复：指令 0x%02X，数据 0x%02X\n",
                  frameCommand, frameData);
  }
}

// 使用软串口发送一条完整数据帧
void sendSoftFrame(byte command, byte data) {
  SoftSerial.write(FRAME_HEADER);
  SoftSerial.write(FRAME_LENGTH);
  SoftSerial.write(command);
  SoftSerial.write(data);
  SoftSerial.write(calculateChecksum(command, data));
  SoftSerial.write(FRAME_TAIL);
}

// 使用软串口接收并检查一条数据帧
bool readSoftFrame() {
  while (SoftSerial.available() >= FRAME_SIZE) {
    byte header = SoftSerial.read();

    // 丢弃帧头前面的干扰字节
    if (header != FRAME_HEADER) {
      continue;
    }

    byte length = SoftSerial.read();
    byte command = SoftSerial.read();
    byte data = SoftSerial.read();
    byte checksum = SoftSerial.read();
    byte tail = SoftSerial.read();

    // 长度、校验位和帧尾都正确才接收
    if (length == FRAME_LENGTH &&
        checksum == calculateChecksum(command, data) &&
        tail == FRAME_TAIL) {
      frameCommand = command;
      frameData = data;
      return true;
    }
  }

  return false;
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 启动 USB 串口和软串口。
  2. 从电脑读取一行文字命令。
  3. 判断是 `led 数值` 还是 `ping`。
  4. 把文字命令打包成二进制数据帧，通过软串口发给板 B。
  5. 再检查软串口是否收到板 B 的应答帧。
  6. 如果收到，就在电脑串口监视器中显示应答内容。
- `setup()` 的作用：
  - 初始化两个串口并打印提示信息。
- `loop()` 的作用：
  - 同时承担“文字命令解析”“数据帧发送”“应答帧接收”三件事。
- 数据输入、处理、输出的执行过程：
  - 输入：电脑输入的文本命令，以及板 B 返回的应答帧。
  - 处理：板 A 先把文字转换为二进制帧，再解析板 B 返回的帧。
  - 输出：通过软串口向板 B 发帧，并通过 USB 串口把结果显示给电脑。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `SoftwareSerial SoftSerial(PIN_UART_RX, PIN_UART_TX)`
  - `sendSoftFrame(CMD_LED, value)`
  - `message.startsWith("led ")`
  - `Serial.printf(...)`
- 每个函数的作用、写法、参数和返回值：
  - `SoftwareSerial SoftSerial(PIN_UART_RX, PIN_UART_TX)`
    - 作用：创建一个软串口对象。
    - 参数1：接收引脚 `RX`。
    - 参数2：发送引脚 `TX`。
  - `sendSoftFrame(byte command, byte data)`
    - 函数作用：按协议拼出完整数据帧并通过软串口发送。
    - 参数1：命令字节。
    - 参数2：数据字节。
    - 返回值：无返回值。
  - `message.startsWith("led ")`
    - 函数作用：判断输入命令是否以 `led ` 开头。
    - 参数1：前缀字符串。
    - 返回值：`true` 或 `false`。
- 关键代码片段分析：
  - `文字命令 -> 数据帧 -> 板 B 应答 -> 串口显示` 体现了“人机界面”和“设备协议”分层处理的思路。
- 为什么这样写：
  - 学生用电脑输入文字更直观，板间通信用二进制帧更规范，这个实验正好把两层连接起来。

---

## 实验7

### 1. 实验7预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 这是双板通信中的接收端程序。
  - 板 B 主要负责校验数据帧、执行命令并返回应答。
  - 板 B 自己不直接和电脑交互，它通过软串口和板 A 通信。
- 本实验新增的术语、模块或函数：
  - `frameCommand`
  - `frameData`
  - `CMD_PING`
  - `CMD_ERROR`
- 开始本实验前需要特别提醒的点：
  - 板 B 软串口波特率必须和板 A 一致。
  - LED 接 `GPIO 2`。

### 2. 实验7接线
- 接线表：

| 板 B | 连接 | 板 A |
| --- | --- | --- |
| `GPIO 41 (TX)` | → | `GPIO 42 (RX)` |
| `GPIO 42 (RX)` | ← | `GPIO 41 (TX)` |
| `GND` | ↔ | `GND` |
| LED `GPIO 2` | 本板控制 | 外接 LED 或板载 LED |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：板 B 通过软串口接收板 A 的数据帧，控制 LED 并发送应答帧。
  使用方法：板 B 上传本程序，GPIO 41 TX 接板 A 的 GPIO 42 RX，GPIO 42 RX 接板 A 的 GPIO 41 TX，并共地。
  运行结果：板 A 输入 led 数值时板 B 改变 LED 亮度；输入 ping 时板 B 返回通信应答。
*/

#include <SoftwareSerial.h>
#include "SerialFrame.h"

#define PIN_UART_RX 42
#define PIN_UART_TX 41
#define PIN_LED 2

#define CMD_LED 0x01
#define CMD_PING 0x05
#define CMD_ERROR 0x7F

// 创建软串口：第一个是 RX，第二个是 TX
SoftwareSerial SoftSerial(PIN_UART_RX, PIN_UART_TX);

// 保存板 A 发来的指令和数据
byte frameCommand = 0;
byte frameData = 0;

void setup() {
  pinMode(PIN_LED, OUTPUT);

  // 直接启动软串口，板 A 和板 B 都使用 9600
  SoftSerial.begin(9600);
}

void loop() {
  if (readSoftFrame()) {
    if (frameCommand == CMD_LED) {
      // 根据收到的数据设置 LED 亮度
      analogWrite(PIN_LED, frameData);
      sendSoftFrame(0x81, frameData);
    } else if (frameCommand == CMD_PING) {
      // PING 指令返回固定应答
      sendSoftFrame(0x85, 0x00);
    } else {
      // 未知指令返回错误码
      sendSoftFrame(CMD_ERROR, 0x01);
    }
  }
}

// 使用软串口发送一条完整数据帧
void sendSoftFrame(byte command, byte data) {
  SoftSerial.write(FRAME_HEADER);
  SoftSerial.write(FRAME_LENGTH);
  SoftSerial.write(command);
  SoftSerial.write(data);
  SoftSerial.write(calculateChecksum(command, data));
  SoftSerial.write(FRAME_TAIL);
}

// 使用软串口接收并检查一条数据帧
bool readSoftFrame() {
  while (SoftSerial.available() >= FRAME_SIZE) {
    byte header = SoftSerial.read();

    // 丢弃帧头前面的干扰字节
    if (header != FRAME_HEADER) {
      continue;
    }

    byte length = SoftSerial.read();
    byte command = SoftSerial.read();
    byte data = SoftSerial.read();
    byte checksum = SoftSerial.read();
    byte tail = SoftSerial.read();

    // 长度、校验位和帧尾都正确才接收
    if (length == FRAME_LENGTH &&
        checksum == calculateChecksum(command, data) &&
        tail == FRAME_TAIL) {
      frameCommand = command;
      frameData = data;
      return true;
    }
  }

  return false;
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 初始化 LED 和软串口。
  2. 持续监听来自板 A 的数据帧。
  3. 校验通过后，读取命令字节和数据字节。
  4. 如果是 LED 指令，就控制 LED 并回发成功应答。
  5. 如果是 PING 指令，就回发固定通信应答。
  6. 如果命令不认识，就回发错误应答。
- `setup()` 的作用：
  - 初始化 LED 引脚和软串口。
- `loop()` 的作用：
  - 持续接收板 A 的指令并执行。
- 数据输入、处理、输出的执行过程：
  - 输入：板 A 发来的二进制数据帧。
  - 处理：协议校验 + 命令判断。
  - 输出：LED 亮度变化，并通过软串口回发应答帧。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `if (frameCommand == CMD_LED)`
  - `analogWrite(PIN_LED, frameData)`
  - `sendSoftFrame(0x85, 0x00)`
  - `readSoftFrame()`
- 每个函数的作用、写法、参数和返回值：
  - `readSoftFrame()`
    - 函数作用：从软串口中接收并校验一条完整数据帧。
    - 参数：无。
    - 返回值：`true` 表示收到合法帧，`false` 表示没有合法帧。
  - `analogWrite(PIN_LED, frameData)`
    - 函数作用：使用接收到的数据值控制 LED 亮度。
    - 参数1：LED 引脚。
    - 参数2：数据帧里的亮度数据。
    - 返回值：无返回值。
- 关键代码片段分析：
  - 板 B 的逻辑非常像“一个小型外设控制器”：接收命令、执行动作、返回结果。
- 为什么这样写：
  - 双板通信实验的重点不是再做复杂外设，而是让学生真正理解“命令从一块板传到另一块板并得到应答”的完整链路。

---

## 总结
- 实验目标：
  - 学会读取单个字符和一整行文字。
  - 学会区分 `peek()`、`read()`、`print()`、`println()`、`write()`。
  - 学会解析固定格式二进制数据帧。
  - 学会通过串口协议控制歌曲和呼吸灯。
  - 学会完成两块开发板之间的 UART 双向通信。
- 单词：
  - `serial`：串口
  - `communication`：通信
  - `read`：读取
  - `write`：写入 / 发送原始字节
  - `available`：可用的 / 缓冲区中已有的
  - `peek`：偷看 / 只查看不取走
  - `frame`：数据帧
  - `header`：帧头
  - `length`：长度
  - `command`：指令
  - `data`：数据
  - `checksum`：校验和
  - `tail`：帧尾
  - `uart`：通用异步收发器
  - `timeout`：超时
  - `string`：字符串
  - `newline`：换行
  - `reply`：应答 / 回复
  - `ping`：连通性测试
- 接线：
  - 单板 LED 接 `GPIO 2`。
  - 单板无源蜂鸣器接 `GPIO 48`。
  - 双板软串口使用 `GPIO 41` 和 `GPIO 42`，两端 `TX/RX` 交叉连接并共地。
- 关键函数和参数：
  - `Serial.begin(115200)`
  - `Serial.available()`
  - `Serial.peek()`
  - `Serial.read()`
  - `Serial.readStringUntil('\n')`
  - `Serial.setTimeout(100)`
  - `Serial.write(...)`
  - `readFrame() / sendFrame()`
  - `SoftwareSerial SoftSerial(rx, tx)`
  - `SoftSerial.begin(9600)`
- 关键程序：
  - 单字符读取程序
  - 按行读取文字程序
  - 固定数据帧控制 LED 程序
  - 串口切歌程序
  - 串口呼吸灯程序
  - 板 A 发帧程序
  - 板 B 收帧应答程序
- 实验现象：
  - 单字符实验能看到字符和 ASCII 数值对应关系。
  - 按行实验能识别文字命令并返回不同回复。
  - 正确数据帧能控制 LED，并返回成功应答。
  - 歌曲和呼吸灯都可以被串口协议远程控制。
  - 板 A 输入文字命令后，可以远程控制板 B。
- 本课重点：
  - 文本串口适合调试，二进制数据帧适合正式协议。
  - 串口命令必须先校验，再执行。
  - 双板 UART 的关键是波特率一致、TX/RX 交叉和共地。
