/*
  程序功能：控制 WS2812 彩灯循环显示颜色，并在 OLED 上显示当前 RGB 数值。
  使用方法：OLED 接 GPIO 8/9、WS2812 数据端接 GPIO 48，上传程序后直接观察。
  运行结果：彩灯循环变色，OLED 同步显示当前颜色名称或 RGB 数值。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

// Wire、GFX、SSD1306 负责 OLED；NeoPixel 负责单总线 WS2812 彩灯。
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>

// OLED 使用 I2C 引脚 8/9，WS2812 的数据线单独接 GPIO 48。
#define PIN_SDA 8
#define PIN_SCL 9
#define PIN_RGB 48

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C
#define OLED_RESET -1

// OLED 对象负责画面，rgb 对象负责 1 颗按 GRB 顺序传输数据的灯珠。
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_NeoPixel rgb(1, PIN_RGB, NEO_GRB + NEO_KHZ800);

// 用 0、1、2 表示红、绿、蓝三个循环状态。
int colorNumber = 0;

void setup() {
  Serial.begin(115200);

  // OLED 通过 I2C 初始化。
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000);

  // 屏幕初始化失败时停止，便于先解决显示问题再观察联动。
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED 初始化失败");

    while (true) {
      delay(1000);
    }
  }

  // WS2812 初始化后先清零并 show()，保证上电时灯珠处于关闭状态。
  rgb.begin();
  rgb.clear();
  rgb.show();
}

void loop() {
  // colorName 与实际灯色在同一分支赋值，保证屏幕文字和灯光一致。
  String colorName;

  if (colorNumber == 0) {
    // RGB 数值只设到 30，降低亮度和课堂供电压力。
    rgb.setPixelColor(0, rgb.Color(30, 0, 0));
    colorName = "RED";
  } else if (colorNumber == 1) {
    rgb.setPixelColor(0, rgb.Color(0, 30, 0));
    colorName = "GREEN";
  } else {
    rgb.setPixelColor(0, rgb.Color(0, 0, 30));
    colorName = "BLUE";
  }

  // setPixelColor() 只修改库中的颜色数据，show() 才真正发送到灯珠。
  rgb.show();

  // OLED 同样采用“先绘制到缓冲区，再统一刷新”的方式。
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("RGB COLOR");

  display.setTextSize(2);
  display.setCursor(0, 24);
  display.println(colorName);
  display.display();

  // 状态加 1 后到达 3 就回到 0，形成 0→1→2→0 的循环。
  colorNumber = colorNumber + 1;

  if (colorNumber >= 3) {
    colorNumber = 0;
  }

  // 每种颜色保持 1 秒，方便观察灯光与文字是否同步。
  delay(1000);
}
