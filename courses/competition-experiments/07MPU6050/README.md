# ESP32-S3 MPU6050 姿态传感器课程

本课程共 3 课时，使用 Arduino 库管理器中的 **MPU6050 by Electronic Cats**。
原始数据程序使用 `getMotion6()`，姿态程序按照该库的 DMP6 示例，通过 FIFO、
四元数和重力向量解算 Yaw、Pitch、Roll。

## 统一接线

| MPU6050 | ESP32-S3 |
|---|---:|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO 8 |
| SCL | GPIO 9 |
| AD0 | GND，地址为 `0x68` |

如果 AD0 接高电平，地址通常变为 `0x69`。本课程默认使用 `0x68`。

姿态程序使用 `dmpGetCurrentFIFOPacket()` 轮询 FIFO，因此本简化实验不连接 INT 引脚。

## 安装库

1. 打开 Arduino IDE 的“工具 → 管理库”。
2. 搜索 `MPU6050`。
3. 选择作者为 **Electronic Cats** 的库。
4. 安装后代码使用：

```cpp
#include "I2Cdev.h"
#include "MPU6050.h"                       // 原始数据程序
#include "MPU6050_6Axis_MotionApps20.h"   // DMP 姿态程序
```

不要安装成同名的其他 MPU6050 库，否则类名相同但函数接口可能不同。

## 3 课时安排

### 第 1 课时：代码讲解（一）

1. 启动 I2C：`Wire.begin(8, 9)`。
2. 创建对象：`MPU6050 mpu;`。
3. 使用 `mpu.initialize()` 唤醒并初始化传感器。
4. 使用 `mpu.testConnection()` 检查地址和通信。
5. 使用六个 `int16_t` 变量保存三轴加速度和三轴角速度。
6. 使用 `getMotion6()` 一次读取六轴原始数据。
7. 通过串口监视器观察数据变化。

使用代码：

- `01_mpu6050_raw/01_mpu6050_raw.ino`

### 第 2 课时前半：代码讲解（二）

1. 默认 ±2g 量程下，用原始值除以 16384 得到 g。
2. 默认 ±250°/s 量程下，用原始值除以 131 得到 °/s。
3. 使用 `dmpInitialize()` 初始化 MPU6050 内部的 DMP。
4. 使用 `CalibrateAccel()` 和 `CalibrateGyro()` 自动校准。
5. 从 FIFO 数据包中读取四元数和重力向量。
6. 使用 `dmpGetYawPitchRoll()` 得到 Yaw、Pitch、Roll。

使用代码：

- `02_mpu6050_units/02_mpu6050_units.ino`
- `03_mpu6050_tilt/03_mpu6050_tilt.ino`

### 第 2 课时后半：原理讲解（0.5 课时）

- 加速度计测量 X、Y、Z 三个方向的比力，静止时也会测到重力。
- 陀螺仪测量绕 X、Y、Z 三个轴旋转的角速度。
- `int16_t` 范围为 -32768～32767，适合保存 MPU6050 原始寄存器值。
- 只用加速度计算倾角会受到线性加速度干扰，只对陀螺仪积分则会逐渐漂移。
- DMP 在 MPU6050 内部融合加速度和陀螺仪数据，并输出四元数等处理结果。
- 四元数再结合重力向量，可以换算为便于观察的 Yaw、Pitch、Roll。
- MPU6050 没有磁力计，Yaw 没有绝对方向参考，静止时也可能缓慢漂移。
- DMP 数据先进入 FIFO；程序应取得完整数据包后再更新姿态角。

### 第 3 课时：学生实验（1 课时）

1. 运行连接测试，确认串口显示“MPU6050 连接成功”。
2. 让模块平放，记录 AX、AY、AZ 的原始值。
3. 分别沿 X、Y、Z 方向快速移动，观察加速度变化。
4. 分别绕三个轴转动，观察角速度正负号变化。
5. 运行物理量程序，检查平放时总加速度是否接近 1g。
6. 上传 DMP 程序，上电校准期间保持模块静止、水平放置。
7. 分别转动三个方向，观察 Yaw、Pitch、Roll 的变化。
8. 保持模块静止一段时间，观察没有磁力计约束时的 Yaw 漂移。

## 关键代码

### 初始化与连接检查

```cpp
mpu.initialize();

if (!mpu.testConnection()) {
  Serial.println("MPU6050 连接失败");
}
```

### 读取六轴数据

```cpp
mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
```

变量前面的 `&` 表示把变量的位置交给库函数，使函数能够把读取结果写入这些变量。
课堂只需先理解“六个变量会被填入新数据”。

### 原始值换算

```cpp
float axG = ax / 16384.0;
float gxDps = gx / 131.0;
```

这两个比例只适用于 `initialize()` 的默认量程。改变满量程设置后，换算比例也必须改变。

### DMP 姿态解算

```cpp
if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) {
  mpu.dmpGetQuaternion(&quaternion, fifoBuffer);
  mpu.dmpGetGravity(&gravity, &quaternion);
  mpu.dmpGetYawPitchRoll(ypr, &quaternion, &gravity);
}
```

数据处理顺序为：FIFO 数据包 → 四元数 → 重力向量 → Yaw/Pitch/Roll。
库函数需要修改四元数和重力向量变量，因此这里的 `&` 是 DMP 官方接口要求的写法。

## 常见问题

- **提示连接失败：** 先运行 I2C 扫描器，检查是否找到 `0x68` 或 `0x69`。
- **编译提示缺少头文件：** 检查是否安装了 Electronic Cats 的 MPU6050 库。
- **静止时数值不是零：** 加速度计会测到重力，陀螺仪也存在零偏和噪声。
- **角度跳动：** 保持模块静止、导线短且牢固，并适当增加平均或滤波。
- **角度方向相反：** 传感器安装方向不同，可交换轴或修改正负号。
- **DMP 初始化失败：** 检查是否使用 Electronic Cats 库，以及头文件是否为
  `MPU6050_6Axis_MotionApps20.h`。
- **上电后角度不稳定：** 自动校准期间不要移动模块，等待校准结束后再操作。
