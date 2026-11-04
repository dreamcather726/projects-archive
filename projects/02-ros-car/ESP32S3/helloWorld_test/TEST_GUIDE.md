# ROS-CAR ESP32-S3 功能测试指南

## 1. 当前四个按键功能

| 按键引脚 | 功能 | 操作效果 |
|---|---|---|
| GPIO10 | I2S 喇叭测试音 | 每按一次，MAX98357 播放一段上升后下降的音阶 |
| GPIO11 | 无源蜂鸣器音乐 | 每按一次，GPIO48 蜂鸣器播放一段短旋律 |
| GPIO15 | 电机加速 | 每完整按下并松开一次，左右轮目标速度增加 5 cm/s，最高 60 cm/s |
| GPIO16 | 电机减速 | 每完整按下并松开一次，左右轮目标速度减少 5 cm/s，减到 0 时立即停车 |

按键使用内部上拉，每个按键的一端接对应 GPIO，另一端接 GND。空闲时原始电平应为 1，按下时应为 0。

## 2. 重要安全准备

1. 首次测试电机时，将小车架起，让车轮离地。
2. 确保电机、ESP32-S3、MAX98357、MPU6050 和循迹模块共地。
3. 喇叭必须接 MAX98357 的 `SPK+` / `SPK-`，不能直接 ESP32 GPIO。
4. 电池电压必须先经电阻分压，进入 GPIO14 的电压不能超过 3.3V。

## 3. I2S 喇叭接线

| MAX98357 | ESP32-S3 |
|---|---|
| DIN | GPIO40 |
| LRC / WS | GPIO39 |
| BCLK | GPIO38 |
| GND | GND |
| VIN | 按 MAX98357 模块规格供电 |

## 4. 编译、烧录与串口

在项目目录中执行：

```powershell
pio run
pio run -t upload
pio device monitor -b 115200
```

烧录后等待串口出现 `[DATA]`。只有进入主循环后按键功能才开始工作。当前已关闭约 20 秒的 MPU6050 开机自动校准，以缩短开机等待时间。

当前默认开启开机音频自检，不按任何按键也会依次执行：

```text
[SELFTEST] buzzer melody
[SELFTEST] I2S sweep
[SELFTEST] audio complete
```

正常情况下应先听到 GPIO48 蜂鸣器旋律，随后听到 MAX98357 喇叭的上升—下降音阶。该自检可通过 `AUDIO_SELF_TEST_AT_STARTUP` 关闭。

## 5. 按键响应测试

每个按键都只需快速按下并松开，无需长按。当前软件去抖时间为 5 ms。

串口周期输出中，未按键时应看到：

```text
[KEY] GPIO10 raw=1 pressed=0 GPIO11 raw=1 pressed=0 GPIO15 raw=1 pressed=0 GPIO16 raw=1 pressed=0
```

以 GPIO10 为例，按下时应立即变为：

```text
[KEY] GPIO10 raw=0 pressed=1 ...
[ACTION] I2S sweep started
```

若 `raw` 数值不变，是接线问题，不是去抖问题。用万用表通断档检查按下时 GPIO 是否与 GND 导通。

## 6. 分功能测试

### 6.1 GPIO10—I2S 起伏音频

1. 快速按一次 GPIO10 按键。
2. 串口应显示 `[ACTION] I2S sweep started`。
3. `[DATA]` 中 `i2s_sweep` 在播放期间为 1，播放完成后回到 0。
4. 喇叭应播放从低到高、再从高到低的音阶。

播放期间重复按下会报告 `I2S sweep busy`，不会叠加创建播放任务。

### 6.2 GPIO11—无源蜂鸣器旋律

1. 快速按一次 GPIO11 按键。
2. 串口应显示 `[ACTION] buzzer melody started`。
3. `[DATA]` 中 `buzzer_music` 播放时为 1，播放完成后为 0。
4. GPIO48 无源蜂鸣器应播放完整短旋律。

旋律采用非阻塞状态机，播放时屏幕、按键、PID 和数据上报应继续工作。

### 6.3 GPIO15—电机加速

1. 确保车轮离地。
2. 按一次 GPIO15，串口应显示 `motor target 5.0 cm/s`。
3. 继续每次按下并松开，目标速度应依次为 10、15、20 cm/s 等。
4. 查看 `[DATA]` 中 `cmd_l/cmd_r`、`speed_l/speed_r` 和 `pwm_l/pwm_r`。
5. 目标速度最高限制为 60 cm/s。

低于电机起动死区时车轮可能暂时不转，连续加速至 15–20 cm/s 后再观察。

### 6.4 GPIO16—电机减速

1. 在电机已有目标速度时，每按一次 GPIO16。
2. 串口中 `motor target` 应每次减少 5 cm/s。
3. 减到 `0.0 cm/s` 后，PID 状态和左右轮 PWM 会被清零。
4. 确认 `[DATA]` 中 `cmd_l=0.0,cmd_r=0.0,pwm_l=0,pwm_r=0`。

## 7. 屏幕检查

TFT 底部按键说明应显示：

```text
K10:I2S  K11:BUZ  K15:SPD+  K16:SPD-
```

屏幕还应显示编码器、速度、电池、循迹数据、按键状态、RPY 姿态角和陀螺仪原始数据。TFT 刷新周期为 50 ms。

## 8. 纯文本数据上报检查

程序每 50 ms 上报一行 `[DATA]`，二进制帧已关闭。关键字段如下：

- `keys`：四个按键的状态，顺序为 GPIO10/11/15/16。
- `i2s_sweep`：I2S 起伏音频是否正在播放。
- `buzzer_music`：无源蜂鸣器旋律是否正在播放。
- `cmd_l/cmd_r`：左右轮当前目标速度。
- `speed_l/speed_r`：左右轮实测速度。
- `line_bits/line_raw`：八路循迹状态和原始值。
- `roll/pitch/yaw/gx/gy/gz`：MPU6050 数据。

## 9. 常见问题

### 按键仍然需要长按

1. 先看 `[KEY]` 中 `raw`。按下时必须从 1 变为 0。
2. 确认等待到 `[DATA]` 开始输出后再测试，开机初始化阶段不读取按键。
3. 若按下时 `raw` 始终为 1，检查按键另一端是否接 GND。
4. 若 `raw` 变化但 `pressed` 不变，检查按键是否严重抖动或接触不良。

### MPU6050 显示 `IMU ERROR`

`Error 263` 是 I2C 超时。检查 GPIO8/SDA、GPIO9/SCL、供电、共地和上拉电压。MPU6050 失败后程序会继续运行，不会再无限阻塞按键。

### 串口显示播放但蜂鸣器不响

1. 确认使用的是无源蜂鸣器，而不是需要固定直流电压的有源模块接法。
2. 确认信号端接 GPIO48，GND 共地。
3. 开机串口应显示 `Buzzer ready: GPIO48 LEDC channel 0`。
4. 若蜂鸣器工作电流超过 GPIO 能力，必须使用三极管/MOSFET 驱动，不要由 GPIO48 直接大功率蜂鸣器。

### 串口显示 I2S 播放但喇叭不响

1. 开机串口应显示 `I2S speaker ready: DIN=40 LRC=39 BCLK=38 stereo`。
2. 确认 MAX98357 的 DIN/LRC/BCLK 没有接反，并与 ESP32 共地。
3. 确认 MAX98357 供电正常；若模块有 `SD`/`EN` 引脚，必须保持高电平才会开启放大器。
4. 喇叭只接 MAX98357 的 `SPK+` 和 `SPK-`，不将任何一端接 GND。
5. 程序已在左右 I2S 声道中输出相同波形，不受 MAX98357 L/R 声道选择影响。

### 电池电压不准

用万用表测量电池实际电压，再调整 `BATTERY_VOLTAGE_DIVIDER_RATIO`。必须先确认 GPIO14 上的物理电压不超过 3.3V。

## 10. 通过标准

全部条件满足即可认为测试通过：

- 快速点按各按键时，`raw/pressed` 和 `[ACTION]` 立即变化。
- I2S 起伏音频播放期间主循环仍持续上报数据。
- 蜂鸣器旋律播放期间按键和屏幕仍可响应。
- 电机目标速度能按 5 cm/s 步进增减，并能降到 0 立即停车。
- `[DATA]` 为可读纯文本，没有混入二进制帧。
