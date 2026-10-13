/*

  程序功能：使用编码器 2 倍频中断计数，在 ST7789 TFT 上显示计数、距离和速度。
  使用方法：编码器 A/B 接 GPIO 6/7并连接TFT；程序已按60mm轮径、单边沿每圈430次设置。
  运行结果：TFT 每 200ms 更新一次 Count、Distance(cm) 和 Speed(cm/s)，反转时数值为负。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

const int ENCODER_A_PIN = 6;
const int ENCODER_B_PIN = 7;

// 实测：车轮转一圈，单边沿计数430次
const int SINGLE_EDGE_COUNTS_PER_REVOLUTION = 430;
const int ENCODER_MULTIPLY = 2;
const int COUNTS_PER_REVOLUTION =SINGLE_EDGE_COUNTS_PER_REVOLUTION * ENCODER_MULTIPLY;
const float WHEEL_DIAMETER_MM = 60.0;

const int8_t TFT_CS_PIN = 47;
const int8_t TFT_DC_PIN = 21;
const int8_t TFT_RST_PIN = -1;
const int8_t TFT_SDA_PIN = 20;
const int8_t TFT_SCL_PIN = 19;
const uint16_t TFT_NATIVE_WIDTH = 240;
const uint16_t TFT_NATIVE_HEIGHT = 320;
const uint8_t TFT_ROTATION = 3;

Adafruit_ST7789 screen(
  TFT_CS_PIN,
  TFT_DC_PIN,
  TFT_SDA_PIN,
  TFT_SCL_PIN,
  TFT_RST_PIN
);

volatile long encoderCount = 0;

long lastCount = 0;
unsigned long lastMeasureTime = 0;

// 只处理 A 相上升沿和下降沿，即 2 倍频
void IRAM_ATTR countEncoder2x() {
  int levelA = digitalRead(ENCODER_A_PIN);
  int levelB = digitalRead(ENCODER_B_PIN);

  // 上升沿：读取B相，B为高记作正转
  if (levelA == HIGH) {
    if (levelB == HIGH) {
      encoderCount = encoderCount + 1;
    } else {
      encoderCount = encoderCount - 1;
    }
  }

  // 下降沿：同一正转方向下，B相应为低电平
  if (levelA == LOW) {
    if (levelB == LOW) {
      encoderCount = encoderCount + 1;
    } else {
      encoderCount = encoderCount - 1;
    }
  }
}

void setup() {
  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), countEncoder2x, CHANGE);

  screen.init(TFT_NATIVE_WIDTH, TFT_NATIVE_HEIGHT);
  screen.setRotation(TFT_ROTATION);
  screen.setTextWrap(false);
  drawStaticScreen();

  lastMeasureTime = millis();
}

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - lastMeasureTime >= 200) {
    noInterrupts();
    long currentCount = encoderCount;
    interrupts();

    unsigned long timeChange = currentTime - lastMeasureTime;
    long countChange = currentCount - lastCount;

    float wheelLengthCm = PI * WHEEL_DIAMETER_MM / 10.0;
    float distanceCm = currentCount * wheelLengthCm
                     / COUNTS_PER_REVOLUTION;
    float speedCmPerSecond = countChange * wheelLengthCm
                           / COUNTS_PER_REVOLUTION
                           * 1000.0 / timeChange;

    updateScreen(currentCount, distanceCm, speedCmPerSecond);

    lastCount = currentCount;
    lastMeasureTime = currentTime;
  }
}

void drawStaticScreen() {
  screen.fillScreen(ST77XX_BLACK);
  screen.setTextSize(2);
  screen.setTextColor(ST77XX_CYAN);
  screen.setCursor(30, 12);
  screen.print("ENCODER 2X MONITOR");
  screen.drawFastHLine(0, 40, 320, ST77XX_BLUE);

  screen.setTextColor(ST77XX_WHITE);
  screen.setCursor(12, 62);
  screen.print("Count:");
  screen.setCursor(12, 112);
  screen.print("Distance:");
  screen.setCursor(12, 162);
  screen.print("Speed:");

  screen.setTextSize(1);
  screen.setCursor(12, 220);
  screen.print("Distance: cm    Speed: cm/s");
}

void updateScreen(long countValue, float distanceCm, float speedCmPerSecond) {
  screen.setTextSize(2);
  screen.setTextColor(ST77XX_YELLOW);

  screen.fillRect(135, 58, 180, 24, ST77XX_BLACK);
  screen.setCursor(140, 62);
  screen.print(countValue);

  screen.fillRect(135, 108, 180, 24, ST77XX_BLACK);
  screen.setCursor(140, 112);
  screen.print(distanceCm, 2);

  screen.fillRect(135, 158, 180, 24, ST77XX_BLACK);
  screen.setCursor(140, 162);
  screen.print(speedCmPerSecond, 2);
}
