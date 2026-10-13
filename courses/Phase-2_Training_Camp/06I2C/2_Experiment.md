# 第二模块 实验

## 实验1

### 1. 实验1预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 本实验不做复杂业务控制，核心目标是扫描总线上的 I2C 地址。
  - 扫描时主机会依次尝试多个地址，只要某个地址收到 `ACK`，就说明有设备响应。
  - `Wire.endTransmission()` 的返回值可以用来判断该地址是否有设备。
- 本实验新增的术语、模块或函数：
  - `Wire.begin()`
  - `Wire.setClock()`
  - `Wire.beginTransmission()`
  - `Wire.endTransmission()`
- 开始本实验前需要特别提醒的点：
  - 本课统一使用 `SDA = GPIO 8`、`SCL = GPIO 9`。
  - 扫描前先确认模块供电正常，并与开发板共地。
  - 串口监视器波特率设置为 `115200`。

### 2. 实验1接线
- 接线表：

| I2C 信号 / 模块引脚 | ESP32-S3 / 电路 | 说明 |
| --- | --- | --- |
| `SDA` | `GPIO 8` | I2C 数据线 |
| `SCL` | `GPIO 9` | I2C 时钟线 |
| `GND` | `GND` | 与开发板共地 |
| `VCC` | 按模块标注 | 常见为 `3.3V` 或 `5V`，以模块说明书为准 |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：扫描 I2C 总线，查找已连接设备并在串口显示设备地址。
  使用方法：I2C 设备的 SDA 接 GPIO 8、SCL 接 GPIO 9，打开波特率为 115200 的串口监视器。
  运行结果：串口显示找到的十六进制地址，例如 MPU6050 为 0x68、OLED 常为 0x3C。
*/

#include <Wire.h>

// 本课统一使用的 I2C 引脚
#define PIN_SDA 8
#define PIN_SCL 9

void setup() {
  // 启动串口，显示扫描结果
  Serial.begin(115200);

  // 启动 I2C，并指定 SDA、SCL 引脚
  Wire.begin(PIN_SDA, PIN_SCL);

  // 把 I2C 时钟设置为 100 kHz
  Wire.setClock(100000);

  Serial.println("开始扫描 I2C 地址");
}

void loop() {
  int deviceCount = 0;

  // I2C 的 7 位设备地址通常在 0x01～0x7E 之间
  for (int address = 1; address < 127; address = address + 1) {
    // 告诉主机：准备向这个地址发起一次通信
    Wire.beginTransmission(address);

    // 真正发出地址并等待应答，返回值 0 表示设备正常响应
    int result = Wire.endTransmission();

    if (result == 0) {
      Serial.print("找到设备：0x");

      // 为了让十六进制地址显示成两位，个位数前面补 0
      if (address < 16) {
        Serial.print("0");
      }

      // 以十六进制格式输出地址
      Serial.println(address, HEX);
      deviceCount = deviceCount + 1;
    }
  }

  Serial.print("设备数量：");
  Serial.println(deviceCount);
  Serial.println();

  // 每隔 3 秒重新扫描一次
  delay(3000);
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 在 `setup()` 中启动串口和 I2C 总线。
  2. 指定 `SDA`、`SCL` 引脚，并设置 I2C 时钟。
  3. 在 `loop()` 中把设备计数清零。
  4. 用 `for` 循环依次尝试地址 `0x01~0x7E`。
  5. 每次调用 `beginTransmission()` 和 `endTransmission()` 检查该地址是否有设备响应。
  6. 如果找到设备，就把地址打印到串口，并累加计数。
  7. 扫描完成后输出设备总数，等待 3 秒再次扫描。
- `setup()` 的作用：
  - 初始化串口和 I2C。
- `loop()` 的作用：
  - 周期性扫描 I2C 总线上的所有可能地址。
- 数据输入、处理、输出的执行过程：
  - 输入：I2C 从机对地址探测的 `ACK` 响应。
  - 处理：程序根据返回值判断当前地址是否有设备。
  - 输出：串口打印找到的设备地址和设备数量。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `Wire.begin(PIN_SDA, PIN_SCL)`
  - `Wire.setClock(100000)`
  - `Wire.beginTransmission(address)`
  - `Wire.endTransmission()`
- 每个函数的作用、写法、参数和返回值：
  - `Wire.begin(PIN_SDA, PIN_SCL)`
    - 函数作用：启动 I2C，并指定数据线和时钟线引脚。
    - 参数1：`PIN_SDA`，I2C 数据线引脚。
    - 参数2：`PIN_SCL`，I2C 时钟线引脚。
    - 返回值：无返回值。
  - `Wire.setClock(100000)`
    - 函数作用：设置 I2C 通信时钟频率。
    - 参数1：`100000`，表示 `100kHz`。
    - 返回值：无返回值。
  - `Wire.beginTransmission(address)`
    - 函数作用：准备向指定地址的 I2C 设备发起一次写传输。
    - 参数1：`address`，当前要尝试的设备地址。
    - 返回值：无返回值。
  - `Wire.endTransmission()`
    - 函数作用：真正发起本次 I2C 传输，并返回通信结果。
    - 参数：无。
    - 返回值：`0` 表示成功应答，其他值表示没有收到正常应答或通信错误。
- 关键代码片段分析：
  - `for (int address = 1; address < 127; address = address + 1)` 表示主机把常见 7 位地址范围全部尝试一遍。
  - `if (result == 0)` 是扫描器判断“这个地址上是否有设备”的核心。
- 为什么这样写：
  - 真实工程里，先扫地址再写业务代码，可以快速确认模块是否接好、地址是否正确，是排错非常常用的一步。

---

## 总结
- 实验目标：
  - 学会启动 I2C 总线并指定 `SDA`、`SCL` 引脚。
  - 学会扫描 I2C 总线上的设备地址。
  - 学会根据 `endTransmission()` 的返回值判断设备是否应答。
  - 学会通过串口查看 I2C 设备地址。
- 单词：
  - `I2C`：I2C 总线
  - `SDA`：串行数据线
  - `SCL`：串行时钟线
  - `address`：地址
  - `device`：设备
  - `scanner`：扫描器
  - `master`：主机
  - `slave`：从机
  - `transmission`：传输
  - `clock`：时钟
  - `read`：读取
  - `write`：写入
  - `ack`：应答
  - `bus`：总线
  - `data`：数据
- 接线：
  - `SDA -> GPIO 8`
  - `SCL -> GPIO 9`
  - `GND -> GND`
  - 模块供电按说明书连接
- 关键函数和参数：
  - `Wire.begin(sda, scl)`
  - `Wire.setClock(100000)`
  - `Wire.beginTransmission(address)`
  - `Wire.endTransmission()`
  - `Serial.println(address, HEX)`
- 关键程序：
  - I2C 地址扫描程序
- 实验现象：
  - 串口会显示找到的设备地址。
  - 串口会显示设备总数。
  - 接不同模块时，显示地址会发生变化。
- 本课重点：
  - I2C 总线的核心是“两根线 + 地址识别”。
  - 扫描器是 I2C 实验最基础也最重要的排错工具。
  - 同一总线不能有地址冲突，且 `SDA`、`SCL` 需要正确上拉。
