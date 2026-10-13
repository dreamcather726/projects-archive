/*
  程序功能：读取红外模块的数字状态，并实时显示在 OLED 屏幕上。
  使用方法：OLED 接 GPIO 8/9，红外模块数字输出接 GPIO 40，用物体遮挡或移开红外探头。
  运行结果：OLED 根据红外输入状态切换显示 DETECTED 和 CLEAR。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

// Wire 负责 I2C 通信，GFX 和 SSD1306 负责 OLED 显示。
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED 使用 GPIO 8/9，红外模块的数字输出接 GPIO 40。
#define PIN_SDA 8
#define PIN_SCL 9
#define PIN_IR 40

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C
#define OLED_RESET -1

// 创建 128×64 OLED 对象，后续通过 display 调用显示函数。
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);
  // 红外模块向 ESP32-S3 输出高、低电平，因此设置为数字输入。
  pinMode(PIN_IR, INPUT);

  // 初始化 I2C 总线，100 kHz 足够驱动本课单个 OLED。
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000);

  // 初始化失败时停止程序，并通过串口给出排查提示。
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED 初始化失败");

    while (true) {
      delay(1000);
    }
  }
}

void loop() {
  // 常见红外避障模块采用低电平有效：LOW 表示检测到物体。
  // 比较表达式的结果是 true 或 false，直接保存为布尔变量。
  bool detected = digitalRead(PIN_IR) == LOW;

  // 每次重画前先清除缓冲区，否则 DETECTED 变成 CLEAR 时会留下旧字符。
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("IR SENSOR");

  display.setTextSize(2);
  display.setCursor(0, 24);

  // 把不直观的高低电平翻译成学生容易理解的英文状态。
  if (detected) {
    display.println("DETECTED");
  } else {
    display.println("CLEAR");
  }

  // 将本轮完整画面从缓冲区刷新到 OLED。
  display.display();
  // 适当降低刷新频率，使画面稳定，也避免无意义地高速读取。
  delay(100);
}
