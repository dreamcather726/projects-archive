# 第二模块 实验

## 实验1

### 1. 实验1预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 本实验先不解析数据，只验证是否能正确读回 `32` 个原始字节。
  - 读取流程是：发送读取命令 -> 等待模块准备数据 -> 请求 `32` 字节 -> 逐字节打印。
  - 读取失败时，先不要急着改协议，优先检查 I2C 地址、接线和收到的字节数。
- 本实验新增的术语、模块或函数：
  - `Wire.beginTransmission()`
  - `Wire.write()`
  - `Wire.endTransmission()`
  - `Wire.requestFrom()`
  - `Wire.available()`
- 开始本实验前需要特别提醒的点：
  - 模块地址为 `0x27`。
  - 本实验命令为 `0x01`。
  - 程序要求每次收到完整 `32` 字节才继续处理。

### 2. 实验1接线
- 接线表：

| 巡线模块 | ESP32-S3 | 说明 |
| --- | --- | --- |
| `VCC` | 按模块标注 | 常见为 `3.3V` 或 `5V`，以模块说明为准 |
| `GND` | `GND` | 与开发板共地 |
| `SDA` | `GPIO 8` | I2C 数据线 |
| `SCL` | `GPIO 9` | I2C 时钟线 |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：通过 I2C 读取八路巡线模块返回的 32 个原始字节，并在串口显示。
  使用方法：巡线模块 SDA 接 GPIO 8、SCL 接 GPIO 9，打开 115200 串口并让各探头经过黑白表面。
  运行结果：串口每次显示 32 个十六进制原始字节，表面变化时对应数据随之改变。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include <Wire.h>

#define PIN_SDA 8
#define PIN_SCL 9

const uint8_t SENSOR_ADDRESS = 0x27;
const uint8_t READ_COMMAND = 0x01;
const uint8_t DATA_LENGTH = 32;

void setup() {
  // 启动串口，方便显示读取结果
  Serial.begin(115200);

  // 启动 I2C，并指定 SDA、SCL 引脚
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000);
}

void loop() {
  // 先向巡线模块发送“读取数据”的命令
  Wire.beginTransmission(SENSOR_ADDRESS);
  Wire.write(READ_COMMAND);
  int result = Wire.endTransmission();

  // 命令发送失败时，直接提示并等待下一次重试
  if (result != 0) {
    Serial.println("发送读取命令失败");
    delay(500);
    return;
  }

  // 给模块一点时间准备要返回的数据
  delay(10);

  // 向模块请求 32 个字节
  int receivedBytes = Wire.requestFrom(SENSOR_ADDRESS, DATA_LENGTH);

  if (receivedBytes != DATA_LENGTH) {
    Serial.println("没有收到完整的 32 字节");

    // 收到的数据帧不完整时，把残留字节清空，避免影响下一次读取
    while (Wire.available() > 0) {
      Wire.read();
    }

    delay(500);
    return;
  }

  Serial.print("原始字节：");

  // 按顺序把 32 个字节以十六进制打印出来
  for (int i = 0; i < DATA_LENGTH; i = i + 1) {
    int value = Wire.read();

    // 小于 16 的值前面补 0，方便观察
    if (value < 16) {
      Serial.print("0");
    }

    Serial.print(value, HEX);
    Serial.print(" ");
  }

  Serial.println();
  delay(500);
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 初始化串口和 I2C。
  2. 向地址 `0x27` 发送读取命令 `0x01`。
  3. 检查命令发送是否成功。
  4. 延时 `10ms`，等待模块准备数据。
  5. 请求 `32` 字节数据。
  6. 如果字节数不完整，就清空缓冲区并报错。
  7. 如果字节数完整，就逐字节以十六进制输出。
- `setup()` 的作用：
  - 初始化串口和 I2C 通信。
- `loop()` 的作用：
  - 周期性读取一帧原始字节并显示到串口。
- 数据输入、处理、输出的执行过程：
  - 输入：巡线模块返回的 `32` 个原始字节。
  - 处理：本实验只做完整性检查和十六进制格式化显示。
  - 输出：串口显示原始字节序列。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `Wire.beginTransmission(SENSOR_ADDRESS)`
  - `Wire.write(READ_COMMAND)`
  - `Wire.endTransmission()`
  - `Wire.requestFrom(SENSOR_ADDRESS, DATA_LENGTH)`
  - `Wire.available()`
- 每个函数的作用、写法、参数和返回值：
  - `Wire.beginTransmission(SENSOR_ADDRESS)`
    - 函数作用：开始向指定 I2C 地址发送数据。
    - 参数1：目标设备地址，本课为 `0x27`。
    - 返回值：无返回值。
  - `Wire.write(READ_COMMAND)`
    - 函数作用：把读取命令写入发送缓冲区。
    - 参数1：要发送的命令字节，本课为 `0x01`。
    - 返回值：返回写入的字节数。
  - `Wire.endTransmission()`
    - 函数作用：真正发出本次 I2C 传输，并返回结果。
    - 参数：无。
    - 返回值：`0` 表示成功，非 `0` 表示失败。
  - `Wire.requestFrom(SENSOR_ADDRESS, DATA_LENGTH)`
    - 函数作用：向指定设备请求若干字节数据。
    - 参数1：设备地址。
    - 参数2：请求长度，本课为 `32`。
    - 返回值：实际收到的字节数。
  - `Wire.available()`
    - 函数作用：查看接收缓冲区中还有多少字节可读。
    - 参数：无。
    - 返回值：可读取的字节数量。
- 关键代码片段分析：
  - `if (receivedBytes != DATA_LENGTH)` 是本实验最关键的防错判断，它确保只有收到完整数据帧时才继续处理。
- 为什么这样写：
  - 工程里先确认“通信通了、数据长度对了”，再去解释数据内容，排错效率会高很多。

---

## 实验2

### 1. 实验2预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 本实验要把后 `16` 个字节解析为 `8` 路模拟量。
  - 每一路模拟量由两个字节组成，先读高字节，再读低字节。
  - `uint16_t` 适合保存 `0~65535` 范围内的无符号数据。
- 本实验新增的术语、模块或函数：
  - `uint16_t`
  - `readSensorValues()`
  - `highByte`
  - `lowByte`
  - `sensorValue[8]`
- 开始本实验前需要特别提醒的点：
  - 当前程序按示例协议跳过前 `16` 个字节。
  - 如果实物协议不同，解析位置也要跟着调整。

### 2. 实验2接线
- 接线表：

| 巡线模块 | ESP32-S3 | 说明 |
| --- | --- | --- |
| `VCC` | 按模块标注 | 常见为 `3.3V` 或 `5V`，以模块说明为准 |
| `GND` | `GND` | 与开发板共地 |
| `SDA` | `GPIO 8` | I2C 数据线 |
| `SCL` | `GPIO 9` | I2C 时钟线 |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：通过 I2C 读取八路巡线模块，并解析、显示 8 路模拟量数据。
  使用方法：巡线模块 SDA 接 GPIO 8、SCL 接 GPIO 9，打开 115200 串口并移动黑线或白色底板。
  运行结果：串口依次显示 CH1～CH8 的模拟量，可观察每一路对黑白表面的响应。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include <Wire.h>

#define PIN_SDA 8
#define PIN_SCL 9

const uint8_t SENSOR_ADDRESS = 0x27;
const uint8_t READ_COMMAND = 0x01;
const uint8_t DATA_LENGTH = 32;

// 用数组保存 8 路巡线模拟量
uint16_t sensorValue[8];

void setup() {
  Serial.begin(115200);

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000);
}

void loop() {
  // 先读取并解析 8 路模拟量
  if (readSensorValues()) {
    // 解析成功后，依次打印 S1～S8
    for (int i = 0; i < 8; i = i + 1) {
      Serial.print("S");
      Serial.print(i + 1);
      Serial.print(":");
      Serial.print(sensorValue[i]);
      Serial.print("  ");
    }

    Serial.println();
  } else {
    Serial.println("巡线模块读取失败");
  }

  delay(200);
}

bool readSensorValues() {
  // 发送读取命令
  Wire.beginTransmission(SENSOR_ADDRESS);
  Wire.write(READ_COMMAND);

  if (Wire.endTransmission() != 0) {
    return false;
  }

  // 等待模块准备数据
  delay(10);

  // 请求 32 字节完整数据帧
  int receivedBytes = Wire.requestFrom(SENSOR_ADDRESS, DATA_LENGTH);

  if (receivedBytes != DATA_LENGTH) {
    // 帧不完整时清空缓冲区
    while (Wire.available() > 0) {
      Wire.read();
    }

    return false;
  }

  // 按示例协议跳过前 16 个字节
  for (int i = 0; i < 16; i = i + 1) {
    Wire.read();
  }

  // 后 16 个字节组成 8 路大端 16 位模拟量
  for (int i = 0; i < 8; i = i + 1) {
    uint8_t highByte = Wire.read();
    uint8_t lowByte = Wire.read();

    // 高字节在前、低字节在后，所以要合成为 16 位数值
    sensorValue[i] = highByte * 256 + lowByte;
  }

  return true;
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 初始化串口和 I2C。
  2. 在 `loop()` 中调用 `readSensorValues()`。
  3. `readSensorValues()` 先发送读取命令，再请求完整 `32` 字节。
  4. 跳过前 `16` 个当前课程暂不使用的字节。
  5. 读取后 `16` 个字节，并每两个字节合成一路模拟量。
  6. 返回主循环后，把 `S1~S8` 八路数值显示到串口。
- `setup()` 的作用：
  - 初始化串口和 I2C。
- `loop()` 的作用：
  - 周期性读取并显示八路模拟量。
- 数据输入、处理、输出的执行过程：
  - 输入：模块返回的 `32` 字节数据帧。
  - 处理：跳过前 `16` 字节，再把后 `16` 字节合成为 `8` 路 `uint16_t` 数据。
  - 输出：串口显示 `S1~S8` 的模拟量。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `bool readSensorValues()`
  - `uint16_t sensorValue[8]`
  - `uint8_t highByte = Wire.read()`
  - `sensorValue[i] = highByte * 256 + lowByte`
- 每个函数的作用、写法、参数和返回值：
  - `readSensorValues()`
    - 函数作用：完成一次巡线模块数据读取和八路模拟量解析。
    - 参数：无。
    - 返回值：`true` 表示读取成功，`false` 表示失败。
  - `uint16_t sensorValue[8]`
    - 作用：保存 `8` 路无符号 `16` 位模拟量。
  - `Wire.read()`
    - 函数作用：从 I2C 接收缓冲区中读取一个字节。
    - 参数：无。
    - 返回值：返回读取到的字节值。
- 关键代码片段分析：
  - `sensorValue[i] = highByte * 256 + lowByte` 的本质是把两个 `8` 位数据重新拼成一个 `16` 位数。
  - `highByte * 256` 等价于把高字节左移 `8` 位，是课堂上理解大端格式的关键。
- 为什么这样写：
  - 巡线模块真正有意义的是“每一路的模拟量”，而不是原始字节本身，所以这一实验把通信数据进一步变成了可观察、可分析的传感器数值。

---

## 总结
- 实验目标：
  - 学会向八路巡线模块发送读取命令。
  - 学会读取并检查完整的 `32` 字节数据帧。
  - 学会观察原始字节变化。
  - 学会把后 `16` 个字节解析成 `8` 路 `uint16_t` 模拟量。
- 单词：
  - `line`：巡线 / 线
  - `sensor`：传感器
  - `raw`：原始的
  - `byte`：字节
  - `analog`：模拟量的
  - `value`：数值
  - `address`：地址
  - `command`：命令
  - `data`：数据
  - `length`：长度
  - `high`：高
  - `low`：低
  - `read`：读取
  - `request`：请求
  - `available`：可用的
- 接线：
  - `VCC -> 按模块标注供电`
  - `GND -> GND`
  - `SDA -> GPIO 8`
  - `SCL -> GPIO 9`
- 关键函数和参数：
  - `Wire.beginTransmission(address)`
  - `Wire.write(0x01)`
  - `Wire.endTransmission()`
  - `Wire.requestFrom(address, 32)`
  - `Wire.available()`
  - `Wire.read()`
  - `readSensorValues()`
  - `sensorValue[i] = highByte * 256 + lowByte`
- 关键程序：
  - 八路巡线模块原始字节读取程序
  - 八路巡线模块模拟量解析程序
- 实验现象：
  - 串口会显示 `32` 个原始字节。
  - 串口会显示 `S1~S8` 八路模拟量。
  - 黑线、白底、高度变化都会引起数值变化。
- 本课重点：
  - 先验证通信和数据完整性，再做协议解析。
  - 一路 `16` 位模拟量由两个字节组成。
  - 模拟量需要先观察、记录、标定，不能直接套固定阈值。
