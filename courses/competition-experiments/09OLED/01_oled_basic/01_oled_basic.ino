/*
  程序功能：初始化 I2C OLED 屏幕，并显示基础文字和图形。
  使用方法：OLED 的 SDA 接 GPIO 8、SCL 接 GPIO 9，确认地址为 0x3C 后上传程序。
  运行结果：128×64 OLED 显示示例文字、矩形、直线和圆形等基础内容。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define PIN_SDA 8
#define PIN_SCL 9

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED 初始化失败");

    while (true) {
      delay(1000);
    }
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("ESP32-S3");

  display.setTextSize(1);
  display.setCursor(0, 28);
  display.println("I2C OLED");
  display.println("SDA:8  SCL:9");

  // 绘图完成后必须调用 display() 才会真正显示
  display.display();
}

void loop() {
}
