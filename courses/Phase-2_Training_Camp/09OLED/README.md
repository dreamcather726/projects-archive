# ESP32-S3 OLED 组合显示课程

本课程共 4 课时，使用 128×64 I2C SSD1306 OLED。代码从基础文字显示开始，再分别组合
MPU6050、红外模块、超声波和 RGB 灯，最后完成一个整合仪表盘。

## 使用的库

在 Arduino IDE 库管理器中安装：

1. **Adafruit SSD1306**
2. **Adafruit GFX Library**
3. **Adafruit NeoPixel**
4. **MPU6050 by Electronic Cats**

其中 OLED 基础、红外和超声波示例不需要 MPU6050 或 NeoPixel 库。

## 统一引脚和地址

| 模块 | 引脚/地址 |
|---|---|
| I2C SDA | GPIO 8 |
| I2C SCL | GPIO 9 |
| OLED 地址 | `0x3C` |
| MPU6050 地址 | `0x68` |
| 红外数字输出 | GPIO 40 |
| 超声波 TRIG | GPIO 12 |
| 超声波 ECHO | GPIO 13，5V 模块必须分压 |
| WS2812 RGB | GPIO 48 |

> GPIO 48 在前面的蜂鸣器课程中也被指定为无源蜂鸣器引脚。OLED+RGB 实验不能同时
> 使用 GPIO 48 的无源蜂鸣器；切换实验前先断电并按当前实验接线。

## 4 课时安排

### 第 1 课时：代码讲解（一）

1. 创建 `Adafruit_SSD1306` 对象。
2. 使用 `display.begin()` 初始化屏幕。
3. 使用 `clearDisplay()` 清除内存缓冲区。
4. 设置文字大小、颜色和光标位置。
5. 使用 `print()`、`println()` 写入文字和数字。
6. 使用 `display.display()` 把缓冲区内容发送到屏幕。
7. 在 OLED 上显示 MPU6050 的 Roll、Pitch。

使用代码：

- `01_oled_basic/01_oled_basic.ino`
- `02_oled_mpu6050/02_oled_mpu6050.ino`

### 第 2 课时前半：代码讲解（二）

1. OLED 显示红外检测状态。
2. OLED 显示超声波距离和无回波状态。
3. OLED 显示 WS2812 当前颜色。
4. 在整合程序中分行显示多个传感器数据。
5. 根据超声波距离设置 RGB 颜色。

使用代码：

- `03_oled_ir/03_oled_ir.ino`
- `04_oled_ultrasonic/04_oled_ultrasonic.ino`
- `05_oled_rgb/05_oled_rgb.ino`
- `06_oled_integrated/06_oled_integrated.ino`

### 第 2 课时后半：原理讲解（0.5 课时）

- SSD1306 OLED 的像素能够自行发光，不需要背光。
- 128×64 表示横向 128 个像素、纵向 64 个像素。
- Adafruit SSD1306 先在内存缓冲区绘图，调用 `display()` 后才刷新屏幕。
- OLED 和 MPU6050 可以并联在同一 I2C 总线上，因为地址分别为 `0x3C` 和 `0x68`。
- 默认字体主要支持 ASCII；直接显示中文需要额外字库，本基础课使用英文标签。
- 频繁刷新会占用 I2C 时间，基础实验使用 100～200 ms 刷新间隔。
- 长时间显示完全不变的高亮画面可能产生老化，实际项目可定时熄屏或移动内容。

### 第 3 课时：学生实验（一）

1. 显示自己的英文名字和班级。
2. 修改文字大小和光标位置。
3. 给 Roll、Pitch 增加单位。
4. 改变 MPU6050 安装方向，观察角度正负号。
5. 遮挡红外模块，确认 DETECTED/CLEAR 切换。
6. 移动障碍物，观察超声波距离。

### 第 4 课时：学生实验（二）

1. 运行 RGB 示例，修改红、绿、蓝亮度。
2. 运行整合程序，确认 OLED 同时显示姿态、红外、距离和颜色。
3. 修改距离颜色规则：20 cm 内红色、50 cm 内黄色、其他绿色。
4. 故意断开一个 I2C 模块，观察初始化失败信息。
5. 选做：用 `drawRect()` 绘制距离条。
6. 选做：增加页面编号，每隔 2 秒切换一组信息。

## OLED 基础显示流程

```cpp
display.clearDisplay();
display.setTextColor(SSD1306_WHITE);
display.setTextSize(1);
display.setCursor(0, 0);
display.println("Hello");
display.display();
```

必须记住最后的 `display.display()`。前面的绘图函数只修改内存中的缓冲区。

## 各组合实验说明

### OLED + MPU6050

OLED 与 MPU6050 共用 SDA/SCL。程序按照 Electronic Cats 的 DMP6 示例读取 FIFO，
再通过四元数和重力向量得到 Yaw、Pitch、Roll。

### OLED + 红外模块

本例按常见低电平触发红外避障模块编写：

```cpp
bool detected = digitalRead(PIN_IR) == LOW;
```

如果实物模块为高电平触发，把 `LOW` 改为 `HIGH`。

### OLED + 超声波

TRIG 使用 GPIO 12，ECHO 使用 GPIO 13。经典 5V HC-SR04 的 ECHO 必须先降到 3.3V，
接线方式沿用“05超声波”课程。

### OLED + RGB

使用 GPIO 48 的单颗 WS2812。示例把亮度限制为 30，避免板载灯过亮。

### OLED 整合

整合程序完成：

1. 使用 DMP 读取 MPU6050 的 Yaw、Pitch、Roll。
2. 读取 GPIO 40 红外状态。
3. 读取超声波距离。
4. 按距离设置 RGB 颜色。
5. 把所有信息刷新到 OLED。

## I2C 接线

OLED 和 MPU6050 并联：

```text
ESP32-S3 GPIO 8  ── OLED SDA
                  └─ MPU6050 SDA

ESP32-S3 GPIO 9  ── OLED SCL
                  └─ MPU6050 SCL

所有模块 GND 必须共地
```

先运行 I2C 扫描器，整合实验应能看到 `0x3C` 和 `0x68`。

## 常见问题

- **OLED 全黑：** 检查地址是否为 `0x3C`，少数模块使用 `0x3D`。
- **编译找不到 OLED 头文件：** 安装 Adafruit SSD1306 和 Adafruit GFX。
- **MPU6050 连接失败：** 确认安装 Electronic Cats 库并扫描 `0x68`。
- **DMP 初始化失败：** 确认能够找到 `MPU6050_6Axis_MotionApps20.h`。
- **姿态角上电不稳定：** 校准过程中保持 MPU6050 静止、水平放置。
- **OLED 文字重叠：** 每次刷新前调用 `clearDisplay()`。
- **画面不更新：** 绘图完成后调用 `display.display()`。
- **红外状态相反：** 把判断中的 `LOW` 改为 `HIGH`。
- **超声波无回波：** 检查 GPIO 12、GPIO 13、共地和 ECHO 分压。
- **RGB 不亮：** 检查 GPIO 48、NeoPixel 库和 `rgb.show()`。
