# 第13课 TFT屏幕与 SPI 显示

## 实验目标

使用 ESP32-S3 通过 SPI 控制 ST7789 TFT 屏幕，在屏幕上显示小车状态信息。

## 需要的元器件

- ESP32-S3 开发板 1 块
- ST7789 SPI TFT 屏幕 1 块
- 杜邦线若干
- USB 数据线 1 根
- Arduino 库：
  - `Adafruit GFX Library`
  - `Adafruit ST7735 and ST7789 Library`

## 接线

| TFT 屏幕 | ESP32-S3 |
| --- | --- |
| BL | 接 3.3V，或按需要接可控 GPIO |
| CS | GPIO14 |
| DC | GPIO13 |
| RST | 3.3V |
| SDA | GPIO12 |
| SCL | GPIO11 |
| VDD | 3.3V |
| GND | GND |
