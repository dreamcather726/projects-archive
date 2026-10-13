/*
  程序功能：使用 Adafruit_ST7789 库初始化 TFT，并显示文字、线条和色块。
  使用方法：TFT 的 CS、DC、SDA、SCL 分别接 GPIO 47、21、20、19，上传后直接观察屏幕。
  运行结果：横向 320×240 屏幕显示标题、边框和红绿蓝三个色块。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

const int8_t TFT_CS_PIN = 47;
const int8_t TFT_DC_PIN = 21;
const int8_t TFT_RST_PIN = -1;

// TFT 丝印 SDA、SCL 在 SPI 模式下分别是 MOSI、SCK
const int8_t TFT_SDA_PIN = 20;
const int8_t TFT_SCL_PIN = 19;

const uint16_t TFT_NATIVE_WIDTH = 240;
const uint16_t TFT_NATIVE_HEIGHT = 320;
const uint8_t TFT_ROTATION = 3;

// 使用软件 SPI 创建 ST7789 屏幕对象
Adafruit_ST7789 screen(
  TFT_CS_PIN,
  TFT_DC_PIN,
  TFT_SDA_PIN,
  TFT_SCL_PIN,
  TFT_RST_PIN
);

void setup() {
  // 按照屏幕原生尺寸初始化，再旋转为横屏
  screen.init(TFT_NATIVE_WIDTH, TFT_NATIVE_HEIGHT);
  screen.setRotation(TFT_ROTATION);
  screen.setTextWrap(false);
  screen.fillScreen(ST77XX_BLACK);

  screen.drawRect(4, 4, 312, 232, ST77XX_WHITE);

  screen.setTextSize(2);
  screen.setTextColor(ST77XX_CYAN);
  screen.setCursor(52, 18);
  screen.print("QINGCHAO ST7789");

  screen.drawFastHLine(5, 46, 310, ST77XX_BLUE);

  // 显示红、绿、蓝三个基础色块
  screen.fillRect(25, 75, 75, 75, ST77XX_RED);
  screen.fillRect(122, 75, 75, 75, ST77XX_GREEN);
  screen.fillRect(219, 75, 75, 75, ST77XX_BLUE);

  screen.setTextColor(ST77XX_WHITE);
  screen.setCursor(68, 180);
  screen.print("320 x 240 READY");
}

void loop() {
  // 基础画面保持不变，主循环不需要重复刷新
}
