#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include "image.h"

// TFT屏幕引脚定义 - ST7789S 240x320
#define TFT_CS   41
#define TFT_RST  45
#define TFT_DC   40
#define TFT_MOSI 47
#define TFT_SCLK 21
#define TFT_BL   42
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);



void initTFT() {
    Serial.println("=== 初始化ST7789S TFT屏幕 ===");
    
    // 初始化背光引脚
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH); // 开启背光
    
    // 初始化ST7789S TFT屏幕
    tft.init(240, 320);           // 初始化ST7789S，240x320分辨率
    tft.setRotation(1);           // 设置屏幕旋转方向 (横屏320x240)
    tft.fillScreen(ST77XX_BLACK); // 清空屏幕为黑色
    
    // 测试屏幕是否正常工作
    Serial.println("执行屏幕测试...");
    tft.fillScreen(ST77XX_RED);
    delay(200);
    
    Serial.println("ST7789S屏幕初始化完成! (分辨率: 320x240横屏模式)");
}

void displayImage() {
    // 显示静态图片
    Serial.println("显示图片到屏幕...");
    
    // 清空屏幕
    tft.fillScreen(ST77XX_BLACK);
    
    // 显示图片数据
    // 注意：这里使用了一个简单的演示，实际图片数据应该通过image_converter.py转换
    int img_width = image_1_width;
    int img_height = image_1_height;
    
    // 计算居中位置（如果图片小于屏幕）
    int start_x = (320 - img_width) / 2;
    int start_y = (240 - img_height) / 2;
    if (start_x < 0) start_x = 0;
    if (start_y < 0) start_y = 0;
    
    Serial.printf("图片尺寸: %dx%d, 显示位置: (%d,%d)\n", img_width, img_height, start_x, start_y);
    
    // 显示真实图片数据
    // 使用转换后的图片数据
    tft.drawRGBBitmap(start_x, start_y, image_1, img_width, img_height);
}

 


void setup() {
    Serial.begin(115200);
    Serial.println("");
    Serial.println("========================================");
    Serial.println("ESP32-S3 + ST7789S + 音频演示项目");
    Serial.println("========================================");
    
    // 等待串口稳定
    delay(1000);
    
    Serial.println("硬件配置:");
    Serial.println("  屏幕: ST7789S 240×320 TFT显示屏");  
    Serial.println("");
    
    // 初始化TFT屏幕
    initTFT();
    
    // 显示静态图片
    displayImage();
    
    Serial.println("");
    Serial.println("========================================");
    Serial.println("系统启动完成!");
    Serial.println("========================================");
    Serial.println("可用串口命令:");
    Serial.println("  clear - 重新显示图片");
    Serial.println("  test  - 彩色显示测试");
    Serial.println("========================================");
}

void loop() {
    // 静态图片显示模式 - 不需要动态更新
    
    // 检查串口输入
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        if (command == "clear") {
            Serial.println("清屏重新显示图片...");
            displayImage();
        } else if (command == "test") {
            Serial.println("执行彩色显示测试...");
            // 测试显示不同颜色和图形
            tft.fillScreen(ST77XX_RED);
            delay(300);
            tft.fillScreen(ST77XX_GREEN);
            delay(300);
            tft.fillScreen(ST77XX_BLUE);
            delay(300);
            tft.fillScreen(ST77XX_YELLOW);
            delay(300);
            tft.fillScreen(ST77XX_MAGENTA);
            delay(300);
            tft.fillScreen(ST77XX_CYAN);
            delay(300);
            displayImage();
            Serial.println("颜色测试完成");
        } else {
            Serial.println("未知命令: " + command);
            Serial.println("输入以下命令之一: clear, test");
            Serial.println("要更换显示图片，请使用tools/image_converter.py转换图片");
        }
    }
    
    delay(100); // 小延迟避免CPU占用过高
}
