# 单模块测试程序

每个子目录都是一个可独立打开、编译和烧录的 Arduino 草图。测试板为
`ESP32S3 Dev Module`，串口监视器波特率均为 `115200`。

| 测试程序 | 硬件与引脚 | 通过标准 |
| --- | --- | --- |
| `imu_test/imu_test.ino` | MPU6050：SDA=8、SCL=9、地址 0x68 | 串口先显示 `PASS`，移动板子时 R/P/Y 和原始值发生变化 |
| `screen_test/screen_test.ino` | ST7789：CS=47、DC=21、MOSI=20、SCLK=19 | 屏幕显示七色条、图形，秒数持续刷新 |
| `button_test/button_test.ino` | 按键：GPIO10、11、15、16，低电平有效 | 每次按下和松开各输出一次事件，无连续抖动 |
| `screen_button_test/screen_button_test.ino` | 上述 ST7789 屏幕和四个按键 | 屏幕实时显示按键状态；按下时对应卡片变绿，计数增加且串口输出事件 |
| `buzzer_test/buzzer_test.ino` | 无源蜂鸣器：GPIO48 | 上电依次发出 1000、2000、3000 Hz 三个音调，串口命令可重复测试 |
| `i2s_music_test/i2s_music_test.ino` | MAX98357：BCLK=38、WS/LRC=39、DIN=40 | 上电播放内置旋律；串口可重播、停止和调节音量 |
| `voltage_test/voltage_test.ino` | 电压采样：GPIO14，暂按 10 倍分压换算 | 串口每 500 ms 输出 ADC 原始值、ADC 引脚电压和分压前输入电压 |
| `motor_test/motor_test.ino` | 右电机 PWM=42、DIR=41；左电机 PWM=1、DIR=2 | 串口命令可分别驱动两个电机，方向正确，3 秒后自动停止 |
| `motor_full_power_test/motor_full_power_test.ino` | 同上 | 初始化后立即让双电机以 PWM=255 满速运行，串口 `s` 可紧急停止 |
| `motor_gentle_test/motor_gentle_test.ino` | 同上 | 保持停止 1 秒，然后每 50 ms 增加 5 PWM，缓慢升到 PWM=120 |

## 使用步骤

1. 在 Arduino IDE 中打开所需子目录内与目录同名的 `.ino` 文件。
2. 开发板选择 `ESP32S3 Dev Module`，然后验证并上传。
3. 打开波特率为 `115200` 的串口监视器，按上表检查结果。

IMU 测试需要安装 `MPU6050 by Electronic Cats 1.4.4`。屏幕测试需要安装
`Adafruit GFX Library 1.12.6` 和 `Adafruit ST7735 and ST7789 Library 1.11.0`。

电压测试中的 `VOLTAGE_DIVIDER_RATIO` 暂时沿用主程序的 `10.0`。GPIO14 上的实际
电压不得超过 3.3 V；如万用表实测值与串口结果不一致，请按实际分压电阻或实测比例
修改该常量。

所有电机测试上电前务必架空车轮。普通电机测试上电默认停止，仅在收到串口命令后
运行，并在 3 秒后自动停止。满速版会在初始化后立即持续以 PWM=255 运行；温柔版
延时 1 秒后缓慢升到 PWM=120。满速版和温柔版均可发送 `s` 停止、发送 `r` 重新运行。
