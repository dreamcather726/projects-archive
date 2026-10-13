/*
  程序功能：控制 WS2812 彩灯循环显示颜色，并在 OLED 上显示当前 RGB 数值。
  使用方法：OLED 接 GPIO 8/9、WS2812 数据端接 GPIO 48，上传程序后直接观察。
  运行结果：彩灯循环变色，OLED 同步显示当前颜色名称或 RGB 数值。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>

#define PIN_SDA 8
#define PIN_SCL 9
#define PIN_RGB 48

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_NeoPixel rgb(1, PIN_RGB, NEO_GRB + NEO_KHZ800);

int colorNumber = 0;

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

  rgb.begin();
  rgb.clear();
  rgb.show();
}

void loop() {
  String colorName;

  if (colorNumber == 0) {
    rgb.setPixelColor(0, rgb.Color(30, 0, 0));
    colorName = "RED";
  } else if (colorNumber == 1) {
    rgb.setPixelColor(0, rgb.Color(0, 30, 0));
    colorName = "GREEN";
  } else {
    rgb.setPixelColor(0, rgb.Color(0, 0, 30));
    colorName = "BLUE";
  }

  rgb.show();

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("RGB COLOR");

  display.setTextSize(2);
  display.setCursor(0, 24);
  display.println(colorName);
  display.display();

  colorNumber = colorNumber + 1;

  if (colorNumber >= 3) {
    colorNumber = 0;
  }

  delay(1000);
}
