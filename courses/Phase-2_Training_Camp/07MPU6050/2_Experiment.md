# 第二模块 实验

## 实验1

### 1. 实验1预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 本实验先不做单位换算，只观察六轴原始数据。
  - `getMotion6()` 会一次读取三轴加速度和三轴角速度。
  - 六个变量前面的 `&` 表示把变量地址交给库函数，让函数直接把数据写进去。
- 本实验新增的术语、模块或函数：
  - `mpu.initialize()`
  - `mpu.testConnection()`
  - `mpu.getMotion6()`
  - `int16_t`
- 开始本实验前需要特别提醒的点：
  - `SDA` 接 `GPIO 8`、`SCL` 接 `GPIO 9`。
  - `AD0` 默认接 `GND`，地址为 `0x68`。
  - 串口监视器波特率设置为 `115200`。

### 2. 实验1接线
- 接线表：

| MPU6050 | ESP32-S3 | 说明 |
| --- | --- | --- |
| `VCC` | `3.3V` | 模块供电 |
| `GND` | `GND` | 与开发板共地 |
| `SDA` | `GPIO 8` | I2C 数据线 |
| `SCL` | `GPIO 9` | I2C 时钟线 |
| `AD0` | `GND` | 默认地址 `0x68` |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：读取 MPU6050 的三轴加速度和三轴陀螺仪原始数据，并通过串口显示。
  使用方法：SDA 接 GPIO 8、SCL 接 GPIO 9，上传后打开 115200 串口并移动、转动模块。
  运行结果：串口持续显示 AX、AY、AZ、GX、GY、GZ 六个原始数值。
*/

#include <Wire.h>
#include "I2Cdev.h"
#include "MPU6050.h"

#define PIN_SDA 8
#define PIN_SCL 9

// 创建一个 MPU6050 传感器对象
MPU6050 mpu;

// 用六个 int16_t 变量保存加速度和角速度原始值
int16_t ax, ay, az;
int16_t gx, gy, gz;

void setup() {
  // 启动串口，显示读取结果
  Serial.begin(115200);

  // 启动 I2C，并指定 SDA、SCL 引脚
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000);

  // 初始化 MPU6050
  mpu.initialize();

  // 检查模块是否成功连接
  if (mpu.testConnection()) {
    Serial.println("MPU6050 连接成功");
  } else {
    Serial.println("MPU6050 连接失败");

    // 如果连接失败，就停在这里不断等待
    while (true) {
      delay(1000);
    }
  }
}

void loop() {
  // 一次性读取三轴加速度和三轴角速度原始值
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // 依次打印六个原始数值
  Serial.print("AX:");
  Serial.print(ax);
  Serial.print(" AY:");
  Serial.print(ay);
  Serial.print(" AZ:");
  Serial.print(az);

  Serial.print(" GX:");
  Serial.print(gx);
  Serial.print(" GY:");
  Serial.print(gy);
  Serial.print(" GZ:");
  Serial.println(gz);

  delay(200);
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 启动串口和 I2C。
  2. 创建并初始化 `MPU6050` 对象。
  3. 先用 `testConnection()` 检查模块是否连接成功。
  4. 在循环中反复调用 `getMotion6()` 读取六轴原始值。
  5. 把六个原始值打印到串口监视器。
- `setup()` 的作用：
  - 初始化串口、I2C 和 `MPU6050`，并做连接检查。
- `loop()` 的作用：
  - 周期性读取并显示六轴原始数据。
- 数据输入、处理、输出的执行过程：
  - 输入：`MPU6050` 六轴寄存器中的原始数据。
  - 处理：本实验基本不做换算，只完成读取和显示。
  - 输出：串口显示 `AX/AY/AZ/GX/GY/GZ` 六个原始值。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `mpu.initialize()`
  - `mpu.testConnection()`
  - `mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz)`
  - `int16_t ax, ay, az`
- 每个函数的作用、写法、参数和返回值：
  - `mpu.initialize()`
    - 函数作用：初始化 `MPU6050`，让模块进入可通信状态。
    - 参数：无。
    - 返回值：无返回值。
  - `mpu.testConnection()`
    - 函数作用：检查 I2C 地址和通信是否正常。
    - 参数：无。
    - 返回值：返回 `true` 或 `false`。
  - `mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz)`
    - 函数作用：一次读取三轴加速度和三轴角速度原始值。
    - 参数1~参数6：六个变量的地址，用来接收读取结果。
    - 返回值：无返回值。
- 关键代码片段分析：
  - 六个变量前的 `&` 不是“乘号”，而是把变量的位置交给函数，函数才能把新数据写进去。
- 为什么这样写：
  - 先观察原始值，能帮助学生建立“传感器其实先给我们的是寄存器数据，不是现成物理量”的认识。

---

## 实验2

### 1. 实验2预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 本实验在读取原始值的基础上，把数据换算为更容易理解的物理单位。
  - 默认量程下，加速度除以 `16384.0` 得到 `g`，角速度除以 `131.0` 得到 `°/s`。
  - 这些换算系数和默认量程绑定，如果改了量程，系数也要跟着变。
- 本实验新增的术语、模块或函数：
  - `axG`
  - `gxDps`
  - `g`
  - `°/s`
- 开始本实验前需要特别提醒的点：
  - 接线与实验1相同。
  - 平放模块时，某一加速度轴一般会接近 `1g`。

### 2. 实验2接线
- 接线表：

| MPU6050 | ESP32-S3 | 说明 |
| --- | --- | --- |
| `VCC` | `3.3V` | 模块供电 |
| `GND` | `GND` | 与开发板共地 |
| `SDA` | `GPIO 8` | I2C 数据线 |
| `SCL` | `GPIO 9` | I2C 时钟线 |
| `AD0` | `GND` | 默认地址 `0x68` |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：读取 MPU6050 数据，并转换为加速度和角速度的常用单位后串口显示。
  使用方法：SDA 接 GPIO 8、SCL 接 GPIO 9，打开 115200 串口并改变模块姿态和转速。
  运行结果：串口显示三轴加速度 g 和三轴角速度 °/s，平放时某一加速度轴约为 1g。
*/

#include <Wire.h>
#include "I2Cdev.h"
#include "MPU6050.h"

#define PIN_SDA 8
#define PIN_SCL 9

MPU6050 mpu;

int16_t ax, ay, az;
int16_t gx, gy, gz;

void setup() {
  Serial.begin(115200);

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000);

  mpu.initialize();

  // 连接失败时直接停住，避免继续输出无意义数据
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 连接失败");

    while (true) {
      delay(1000);
    }
  }
}

void loop() {
  // 先读取六轴原始值
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // initialize() 默认设置为加速度 ±2g、角速度 ±250°/s
  float axG = ax / 16384.0;
  float ayG = ay / 16384.0;
  float azG = az / 16384.0;

  float gxDps = gx / 131.0;
  float gyDps = gy / 131.0;
  float gzDps = gz / 131.0;

  // 打印换算后的物理量
  Serial.print("加速度(g) X:");
  Serial.print(axG, 2);
  Serial.print(" Y:");
  Serial.print(ayG, 2);
  Serial.print(" Z:");
  Serial.print(azG, 2);

  Serial.print(" 角速度(度/秒) X:");
  Serial.print(gxDps, 1);
  Serial.print(" Y:");
  Serial.print(gyDps, 1);
  Serial.print(" Z:");
  Serial.println(gzDps, 1);

  delay(200);
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 初始化串口、I2C 和 `MPU6050`。
  2. 检查连接是否正常。
  3. 在循环中读取六轴原始值。
  4. 用固定比例把原始值换算为加速度和角速度单位。
  5. 把换算后的结果打印到串口。
- `setup()` 的作用：
  - 初始化硬件并检查通信。
- `loop()` 的作用：
  - 反复读取原始值并做单位换算。
- 数据输入、处理、输出的执行过程：
  - 输入：六轴原始寄存器值。
  - 处理：按默认量程比例换算成 `g` 和 `°/s`。
  - 输出：串口显示三轴加速度和三轴角速度物理量。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `float axG = ax / 16384.0`
  - `float gxDps = gx / 131.0`
  - `Serial.print(axG, 2)`
  - `Serial.print(gxDps, 1)`
- 每个函数的作用、写法、参数和返回值：
  - `ax / 16384.0`
    - 作用：把默认 ±2g 量程下的加速度原始值换算成 `g`。
  - `gx / 131.0`
    - 作用：把默认 ±250°/s 量程下的角速度原始值换算成 `度/秒`。
  - `Serial.print(axG, 2)`
    - 函数作用：输出浮点数，并保留 `2` 位小数。
    - 参数1：要输出的变量。
    - 参数2：保留的小数位数。
    - 返回值：发送的字节数。
- 关键代码片段分析：
  - 加速度和角速度都来自同一次 `getMotion6()` 读取，但换算比例不同，说明“同样是原始值，不同传感器通道要用不同规则解释”。
- 为什么这样写：
  - 把原始值换算成物理单位后，学生更容易把数据和实际运动现象对应起来。

---

## 实验3

### 1. 实验3预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 本实验不再手动用原始值算角度，而是调用 `MPU6050` 内部的 DMP 做姿态解算。
  - DMP 处理后的数据先放进 `FIFO`，程序拿到完整数据包后再求出姿态角。
  - `Yaw`、`Pitch`、`Roll` 是更直观的角度表示，但 `Yaw` 没有磁力计约束时会漂移。
- 本实验新增的术语、模块或函数：
  - `dmpInitialize()`
  - `CalibrateAccel()`
  - `CalibrateGyro()`
  - `dmpGetCurrentFIFOPacket()`
  - `dmpGetQuaternion()`
  - `dmpGetGravity()`
  - `dmpGetYawPitchRoll()`
- 开始本实验前需要特别提醒的点：
  - 使用 `MPU6050_6Axis_MotionApps20.h` 头文件。
  - 上电校准时要保持模块静止、水平放置。
  - 本课姿态程序不连接 `INT` 引脚，使用轮询 FIFO 的方式读取。

### 2. 实验3接线
- 接线表：

| MPU6050 | ESP32-S3 | 说明 |
| --- | --- | --- |
| `VCC` | `3.3V` | 模块供电 |
| `GND` | `GND` | 与开发板共地 |
| `SDA` | `GPIO 8` | I2C 数据线 |
| `SCL` | `GPIO 9` | I2C 时钟线 |
| `AD0` | `GND` | 默认地址 `0x68` |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：使用 MPU6050 内部 DMP 解算 Yaw、Pitch、Roll，并通过串口显示。
  使用方法：SDA 接 GPIO 8、SCL 接 GPIO 9；上电校准时保持静止，之后打开 115200 串口转动模块。
  运行结果：串口持续显示 Yaw、Pitch、Roll 角度；无磁力计约束时 Yaw 可能缓慢漂移。
*/

#include <Wire.h>
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"

#define PIN_SDA 8
#define PIN_SCL 9

MPU6050 mpu;

// DMP 解算需要使用的变量
bool dmpReady = false;
uint8_t fifoBuffer[64];
Quaternion quaternion;
VectorFloat gravity;
float ypr[3];

float yaw = 0;
float pitch = 0;
float roll = 0;

void setup() {
  Serial.begin(115200);

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);

  // 初始化 MPU6050
  mpu.initialize();

  // 检查模块连接
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 连接失败");

    while (true) {
      delay(1000);
    }
  }

  Serial.println("正在初始化 DMP...");
  byte dmpStatus = mpu.dmpInitialize();

  // DMP 初始化失败时输出错误代码并停住
  if (dmpStatus != 0) {
    Serial.print("DMP 初始化失败，错误代码：");
    Serial.println(dmpStatus);

    while (true) {
      delay(1000);
    }
  }

  // 模块校准时应保持静止、水平放置
  Serial.println("正在校准，请保持模块静止...");
  mpu.setXGyroOffset(0);
  mpu.setYGyroOffset(0);
  mpu.setZGyroOffset(0);
  mpu.setXAccelOffset(0);
  mpu.setYAccelOffset(0);
  mpu.setZAccelOffset(0);
  mpu.CalibrateAccel(6);
  mpu.CalibrateGyro(6);
  mpu.PrintActiveOffsets();

  // 开启 MPU6050 内部的 DMP
  mpu.setDMPEnabled(true);
  dmpReady = true;
  Serial.println("DMP 初始化完成");
}

void loop() {
  // 只有读取到一组完整姿态角时才输出
  if (readDmpAngles()) {
    Serial.print("Yaw:");
    Serial.print(yaw, 1);
    Serial.print("  Pitch:");
    Serial.print(pitch, 1);
    Serial.print("  Roll:");
    Serial.println(roll, 1);
  }

  delay(100);
}

// 从 DMP 的 FIFO 中读取一组姿态角
bool readDmpAngles() {
  if (dmpReady == false) {
    return false;
  }

  // 没有收到完整数据包时，暂时不更新角度
  if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer) == false) {
    return false;
  }

  // FIFO 数据包 → 四元数 → 重力向量 → Yaw/Pitch/Roll
  mpu.dmpGetQuaternion(&quaternion, fifoBuffer);
  mpu.dmpGetGravity(&gravity, &quaternion);
  mpu.dmpGetYawPitchRoll(ypr, &quaternion, &gravity);

  // 弧度转角度
  yaw = ypr[0] * 180.0 / PI;
  pitch = ypr[1] * 180.0 / PI;
  roll = ypr[2] * 180.0 / PI;

  return true;
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 初始化串口、I2C 和 `MPU6050`。
  2. 检查连接并执行 DMP 初始化。
  3. 做加速度计和陀螺仪校准。
  4. 开启 DMP，并把 `dmpReady` 设为 `true`。
  5. 在循环中不断尝试从 FIFO 读取完整数据包。
  6. 把 FIFO 数据转换成四元数、重力向量，再转换成 `Yaw/Pitch/Roll`。
  7. 把弧度结果换算成角度后输出到串口。
- `setup()` 的作用：
  - 初始化硬件、完成 DMP 初始化和校准。
- `loop()` 的作用：
  - 周期性读取最新姿态角并输出。
- 数据输入、处理、输出的执行过程：
  - 输入：DMP 放入 FIFO 的姿态数据包。
  - 处理：FIFO 数据包 -> 四元数 -> 重力向量 -> `Yaw/Pitch/Roll` -> 角度值。
  - 输出：串口显示姿态角。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `mpu.dmpInitialize()`
  - `mpu.CalibrateAccel(6)`
  - `mpu.CalibrateGyro(6)`
  - `mpu.dmpGetCurrentFIFOPacket(fifoBuffer)`
  - `mpu.dmpGetQuaternion(&quaternion, fifoBuffer)`
  - `mpu.dmpGetYawPitchRoll(ypr, &quaternion, &gravity)`
- 每个函数的作用、写法、参数和返回值：
  - `mpu.dmpInitialize()`
    - 函数作用：初始化 `MPU6050` 内部的 DMP。
    - 参数：无。
    - 返回值：`0` 表示成功，非 `0` 表示失败。
  - `mpu.CalibrateAccel(6)`
    - 函数作用：自动校准加速度计。
    - 参数1：校准次数或校准等级参数，课堂先理解为“校准轮数”。
    - 返回值：无返回值。
  - `mpu.CalibrateGyro(6)`
    - 函数作用：自动校准陀螺仪。
    - 参数1：校准次数或校准等级参数。
    - 返回值：无返回值。
  - `mpu.dmpGetCurrentFIFOPacket(fifoBuffer)`
    - 函数作用：尝试从 FIFO 中取出一包完整 DMP 数据。
    - 参数1：接收数据包的缓冲区。
    - 返回值：`true` 表示取到完整包，`false` 表示暂时没有。
  - `mpu.dmpGetQuaternion(&quaternion, fifoBuffer)`
    - 函数作用：从 FIFO 数据包中提取四元数。
    - 参数1：四元数变量地址。
    - 参数2：FIFO 数据缓冲区。
    - 返回值：无返回值。
  - `mpu.dmpGetYawPitchRoll(ypr, &quaternion, &gravity)`
    - 函数作用：根据四元数和重力向量计算姿态角。
    - 参数1：保存结果的数组。
    - 参数2：四元数变量地址。
    - 参数3：重力向量变量地址。
    - 返回值：无返回值。
- 关键代码片段分析：
  - `FIFO -> 四元数 -> 重力向量 -> 姿态角` 这条链路是本实验的核心，不建议学生只背函数名而不理解顺序。
- 为什么这样写：
  - 原始加速度和角速度更接近底层数据，而 DMP 姿态角更接近最终应用场景，这一实验把两层联系了起来。

---

## 总结
- 实验目标：
  - 学会初始化 `MPU6050` 并检查连接。
  - 学会读取六轴原始数据。
  - 学会把原始值换算成 `g` 和 `°/s`。
  - 学会使用 DMP 解算 `Yaw/Pitch/Roll`。
- 单词：
  - `mpu`：姿态传感器对象名
  - `accel`：加速度
  - `gyro`：陀螺仪 / 角速度
  - `raw`：原始的
  - `units`：物理单位
  - `yaw`：偏航角
  - `pitch`：俯仰角
  - `roll`：横滚角
  - `quaternion`：四元数
  - `gravity`：重力向量
  - `fifo`：先进先出缓冲区
  - `motion`：运动
  - `connection`：连接
  - `initialize`：初始化
  - `calibrate`：校准
- 接线：
  - `VCC -> 3.3V`
  - `GND -> GND`
  - `SDA -> GPIO 8`
  - `SCL -> GPIO 9`
  - `AD0 -> GND`
- 关键函数和参数：
  - `mpu.initialize()`
  - `mpu.testConnection()`
  - `mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz)`
  - `ax / 16384.0`
  - `gx / 131.0`
  - `mpu.dmpInitialize()`
  - `mpu.CalibrateAccel(6)`
  - `mpu.CalibrateGyro(6)`
  - `mpu.dmpGetCurrentFIFOPacket(fifoBuffer)`
  - `mpu.dmpGetYawPitchRoll(ypr, &quaternion, &gravity)`
- 关键程序：
  - `MPU6050` 原始数据读取程序
  - `MPU6050` 物理量换算程序
  - `MPU6050` DMP 姿态角程序
- 实验现象：
  - 串口会显示六轴原始值。
  - 串口会显示三轴加速度和三轴角速度物理量。
  - 串口会显示 `Yaw/Pitch/Roll` 姿态角。
- 本课重点：
  - 先理解原始值，再理解单位换算，最后理解姿态解算。
  - DMP 姿态角更高级，但建立在前面 I2C 通信、原始数据和校准基础之上。
  - 没有磁力计参考时，`Yaw` 漂移是正常现象。
