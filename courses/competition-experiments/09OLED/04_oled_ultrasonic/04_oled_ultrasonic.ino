/*
  程序功能：使用超声波模块测量距离，并实时显示在 OLED 屏幕上。
  使用方法：OLED 接 GPIO 8/9，TRIG 接 GPIO 12、ECHO 经分压接 GPIO 13，然后移动障碍物。
  运行结果：OLED 实时显示距离和 cm 单位；无有效回波时显示 NO ECHO。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define PIN_SDA 8
#define PIN_SCL 9
#define PIN_TRIG 12
#define PIN_ECHO 13

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

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
  float distance = readDistance();

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("ULTRASONIC");

  display.setTextSize(2);
  display.setCursor(0, 24);

  if (distance == 0) {
    display.println("NO ECHO");
  } else {
    display.print(distance, 1);
    display.println(" cm");
  }

  display.display();
  delay(200);
}

float readDistance() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  unsigned long echoTime = pulseIn(PIN_ECHO, HIGH, 30000);
  float distance = echoTime * 0.0343 / 2.0;

  return distance;
}
