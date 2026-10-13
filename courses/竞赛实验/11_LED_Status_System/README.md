# 第11课 LED进阶应用与状态指示系统

## 实验目标

使用 ESP32-S3 控制普通 LED 和 RGB LED，实现基础亮灭控制和颜色状态显示。

## 需要的元器件

- ESP32-S3 开发板 1 块
- 普通 LED 1 个
- RGB LED 1 个
- 杜邦线若干
- USB 数据线 1 根

## 接线

| 元器件 | ESP32-S3 引脚 |
| --- | --- |
| 普通 LED | GPIO4 |
| RGB LED 红色脚 | GPIO8 |
| RGB LED 绿色脚 | GPIO9 |
| RGB LED 蓝色脚 | GPIO10 |

每个 LED 引脚都要串联电阻。
