# ESP32-S3 ST7789 TFT 彩屏整合实验

本课程共 2 课时。屏幕程序按照青潮智科提供的 ST7789 示例编写，使用
`Adafruit_GFX` 和 `Adafruit_ST7789`，把 MPU6050、红外、超声波和 RGB 状态集中
显示在横向 320×240 TFT 彩屏上。

## 安装库

在 Arduino IDE 的库管理器中安装：

1. **Adafruit ST7735 and ST7789 Library**
2. **Adafruit GFX Library**
3. **MPU6050 by Electronic Cats**
4. **Adafruit NeoPixel**

`Wire` 和 `SPI` 来自 ESP32 Arduino 核心，不需要单独安装。

程序使用以下头文件：

```cpp
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
```

## ST7789 屏幕接线

| TFT 引脚 | ESP32-S3 | 作用 |
|---|---:|---|
| CS | GPIO 47 | 片选 |
| DC | GPIO 21 | 数据/命令选择 |
| RST | 不接 | 程序中设置为 `-1` |
| SDA | GPIO 20 | SPI 模式下作为 MOSI 数据线 |
| SCL | GPIO 19 | SPI 模式下作为 SCK 时钟线 |
| GND | GND | 必须共地 |

这里 TFT 丝印的 SDA、SCL 是 SPI 信号，不是 I2C 的 SDA、SCL，不要接到 GPIO 8、9。

屏幕原生尺寸为 240×320，程序使用：

```cpp
screen.init(240, 320);
screen.setRotation(3);
```

旋转后显示区域为横向 320×240。

## 软件 SPI 屏幕对象

```cpp
Adafruit_ST7789 screen(
  47,    // CS
  21,    // DC
  20,    // MOSI，屏幕丝印 SDA
  19,    // SCK，屏幕丝印 SCL
  -1     // 不使用独立 RST
);
```

这个五参数构造方式使用软件 SPI，因此 MOSI 和 SCK 可以直接按照示例指定为 GPIO 20、19。

## 其他模块接线

| 功能 | 引脚/地址 |
|---|---|
| I2C SDA | GPIO 8 |
| I2C SCL | GPIO 9 |
| MPU6050 地址 | `0x68` |
| 红外数字输出 | GPIO 40 |
| 超声波 TRIG | GPIO 12 |
| 超声波 ECHO | GPIO 13，5V ECHO 必须分压 |
| WS2812 RGB | GPIO 48 |

GPIO 21 在本实验中是 TFT 的 DC，不能同时连接前面课程中的 GPIO 21 有源蜂鸣器。
GPIO 48 在本实验中连接 WS2812，不能同时连接 GPIO 48 无源蜂鸣器。

## 两份代码

- `01_tft_basic/01_tft_basic.ino`：检查屏幕接线、方向、颜色和文字。
- `02_tft_integrated/02_tft_integrated.ino`：显示所有传感器数据并控制 RGB。

必须先运行基础程序。基础画面正常后，再上传整合程序。

## 2 课时安排

### 第 1 课时前半：代码讲解（0.5 课时）

1. 区分 TFT 的 SPI SDA/SCL 与传感器的 I2C SDA/SCL。
2. 使用 `Adafruit_ST7789` 创建软件 SPI 屏幕对象。
3. 使用 `screen.init(240, 320)` 初始化屏幕。
4. 使用 `setRotation(3)` 设置 320×240 横屏。
5. 使用 `fillScreen()`、`drawRect()`、`drawFastHLine()` 绘图。
6. 使用 `setTextColor()`、`setTextSize()`、`setCursor()` 显示文字。
7. 使用 `fillRect()` 只清除数值区域，减少整屏刷新闪烁。

### 第 1 课时后半：原理讲解（0.5 课时）

- ST7789 是 TFT 屏幕控制芯片，负责接收 SPI 指令和像素颜色数据。
- `Adafruit_GFX` 提供通用文字、坐标、线条、矩形等绘图函数。
- `Adafruit_ST7789` 负责屏幕初始化和 ST7789 专用通信。
- SPI 的 MOSI 负责发送数据，SCK 提供时钟，CS 选择设备，DC 区分命令与数据。
- `RGB565` 使用 16 位表示颜色，库中的 `ST77XX_RED` 等常量已经封装颜色值。
- 固定标签只绘制一次，动态数值局部刷新，可以明显减少屏幕闪烁。

### 第 2 课时：学生实验（1 课时）

1. 上传基础程序，确认屏幕显示 `QINGCHAO ST7789` 和红绿蓝色块。
2. 修改 `setRotation(0～3)`，观察画面方向和坐标变化。
3. 修改标题、文字颜色和色块尺寸。
4. 上传整合程序，上电校准期间保持 MPU6050 静止、水平。
5. 转动 MPU6050，观察 Yaw、Pitch、Roll。
6. 遮挡红外模块，观察 DETECTED/CLEAR。
7. 移动障碍物，观察距离和 RGB 红、黄、绿变化。
8. 断开一个传感器，观察错误或无数据状态。

## 整合显示内容

```text
SENSOR DASHBOARD
Yaw:    25.6 deg
Pitch:  -5.2 deg
Roll:   12.3 deg
IR:     CLEAR
Dist:   38.6 cm
RGB:    YELLOW
```

RGB 规则：

| 距离 | RGB |
|---:|---|
| 无回波 | 关闭 |
| ≤20 cm | 红色 |
| 20～50 cm | 黄色 |
| >50 cm | 绿色 |

## MPU6050 DMP 解算

姿态部分继续使用 Electronic Cats DMP6 示例中的处理流程：

```text
FIFO 数据包 → 四元数 → 重力向量 → Yaw/Pitch/Roll
```

程序采用 `dmpGetCurrentFIFOPacket()` 轮询 FIFO，不需要连接 MPU6050 的 INT 引脚。
MPU6050 没有磁力计，因此 Yaw 可能随时间缓慢漂移。

## 常见问题

- **屏幕白屏：** 检查 CS=47、DC=21、SDA/MOSI=20、SCL/SCK=19 和供电共地。
- **编译找不到 Adafruit_ST7789.h：** 安装 `Adafruit ST7735 and ST7789 Library`。
- **画面方向不对：** 确认使用 `setRotation(3)`。
- **画面尺寸或偏移不对：** 确认屏幕确实是示例对应的 ST7789 240×320。
- **颜色异常：** 先运行基础程序观察红、绿、蓝色块是否正确。
- **MPU6050 ERROR：** 检查 I2C GPIO 8、9 和地址 `0x68`。
- **DMP INIT ERROR：** 确认安装的是 Electronic Cats MPU6050 库。
- **姿态角上电不稳定：** 校准过程中保持 MPU6050 静止、水平放置。
- **超声波无回波：** 检查 GPIO 12、13 和 ECHO 分压。
- **RGB 不亮：** 检查 GPIO 48 和 `rgb.show()`。

产品声明：本课程及示例程序为青潮智科产品配套教学内容。
