/*
  程序功能：驱动直流电机，使用编码器测量距离和速度，并在 ST7789 TFT 上显示。
  使用方法：电机 DIR/PWM 接 GPIO 2/1，编码器 A/B 接 GPIO 6/7；抬起车轮后上传程序。
  运行结果：电机以 PWM 100 转动，TFT 显示计数、距离、速度和当前 PWM。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

const int MOTOR_DIR_PIN = 2;
const int MOTOR_PWM_PIN = 1;
const int MOTOR_SPEED = 100;

const int ENCODER_A_PIN = 6;
const int ENCODER_B_PIN = 7;
// 实测单边沿每圈430次，双边沿2倍频后每圈860次
const int SINGLE_EDGE_COUNTS_PER_REVOLUTION = 430;
const int COUNTS_PER_REVOLUTION =
  SINGLE_EDGE_COUNTS_PER_REVOLUTION * 2;
const float WHEEL_DIAMETER_MM = 60.0;

const int8_t TFT_CS_PIN = 47;
const int8_t TFT_DC_PIN = 21;
const int8_t TFT_RST_PIN = -1;
const int8_t TFT_SDA_PIN = 20;
const int8_t TFT_SCL_PIN = 19;

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

// 只处理 A 相的上升沿和下降沿，即 2 倍频
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
  pinMode(MOTOR_DIR_PIN, OUTPUT);
  pinMode(MOTOR_PWM_PIN, OUTPUT);
  analogWrite(MOTOR_PWM_PIN, 0);

  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), countEncoder2x, CHANGE);

  screen.init(240, 320);
  screen.setRotation(3);
  screen.setTextWrap(false);
  drawStaticScreen();

  // 屏幕准备完成后再启动电机
  digitalWrite(MOTOR_DIR_PIN, LOW);
  analogWrite(MOTOR_PWM_PIN, MOTOR_SPEED);
  lastMeasureTime = millis();
}

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - lastMeasureTime >= 200) {
    noInterrupts();
    long currentCount = encoderCount;
    interrupts();

    long countChange = currentCount - lastCount;
    unsigned long timeChange = currentTime - lastMeasureTime;
    float wheelLengthCm = PI * WHEEL_DIAMETER_MM / 10.0;
    float distanceCm = currentCount * wheelLengthCm / COUNTS_PER_REVOLUTION;
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
  screen.setCursor(40, 10);
  screen.print("MOTOR + ENCODER");
  screen.drawFastHLine(0, 38, 320, ST77XX_BLUE);

  screen.setTextColor(ST77XX_WHITE);
  screen.setCursor(12, 58);
  screen.print("Count:");
  screen.setCursor(12, 98);
  screen.print("Distance:");
  screen.setCursor(12, 138);
  screen.print("Speed:");
  screen.setCursor(12, 178);
  screen.print("PWM:");

  screen.setTextColor(ST77XX_YELLOW);
  screen.setCursor(140, 178);
  screen.print(MOTOR_SPEED);
}

void updateScreen(long countValue, float distanceCm, float speedCmPerSecond) {
  screen.setTextSize(2);
  screen.setTextColor(ST77XX_GREEN);

  screen.fillRect(135, 55, 180, 22, ST77XX_BLACK);
  screen.setCursor(140, 58);
  screen.print(countValue);

  screen.fillRect(135, 95, 180, 22, ST77XX_BLACK);
  screen.setCursor(140, 98);
  screen.print(distanceCm, 2);
  screen.print(" cm");

  screen.fillRect(135, 135, 180, 22, ST77XX_BLACK);
  screen.setCursor(140, 138);
  screen.print(speedCmPerSecond, 2);
  screen.print(" cm/s");
}
