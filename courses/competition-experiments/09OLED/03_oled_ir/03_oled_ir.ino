/*
  程序功能：读取红外模块的数字状态，并实时显示在 OLED 屏幕上。
  使用方法：OLED 接 GPIO 8/9，红外模块数字输出接 GPIO 40，用物体遮挡或移开红外探头。
  运行结果：OLED 根据红外输入状态切换显示 DETECTED 和 CLEAR。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define PIN_SDA 8
#define PIN_SCL 9
#define PIN_IR 40

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);
  pinMode(PIN_IR, INPUT);

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED 初始化失败");

    while (true) {
      delay(1000);
    }
  }
}

void loop() {
  // 常见红外避障模块检测到物体时输出 LOW
  bool detected = digitalRead(PIN_IR) == LOW;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("IR SENSOR");

  display.setTextSize(2);
  display.setCursor(0, 24);

  if (detected) {
    display.println("DETECTED");
  } else {
    display.println("CLEAR");
  }

  display.display();
  delay(100);
}
