/*
  程序功能：通过串口控制舵机，并在 ST7789 TFT 上显示目标角度、Duty 和进度条。
  使用方法：舵机信号接 GPIO4、TFT 按固定引脚连接；串口发送 g0、g90 或 g180。
  运行结果：舵机转到目标位置，TFT 数字和角度进度条同步更新。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

#define SERVO_PIN 4
#define SERVO_CHANNEL 0

const int SERVO_FREQUENCY = 50;
const int SERVO_RESOLUTION = 14;

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

int currentAngle = 90;

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(100);

  ledcAttachChannel(SERVO_PIN, 50, 14, SERVO_CHANNEL);

  screen.init(240, 320);
  screen.setRotation(3);
  screen.setTextWrap(false);
  drawStaticScreen();

  setServoAngle(currentAngle);
  Serial.println("命令：g0、g90、g180");
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.startsWith("g")) {
      int angle = command.substring(1).toInt();
      setServoAngle(angle);
    }
  }
}

void setServoAngle(int angle) {
  currentAngle = constrain(angle, 0, 180);
  int duty = map(currentAngle, 0, 180, 410, 2048);
  ledcWriteChannel(SERVO_CHANNEL, duty);
  updateScreen(currentAngle, duty);
}

void drawStaticScreen() {
  screen.fillScreen(ST77XX_BLACK);
  screen.setTextSize(2);
  screen.setTextColor(ST77XX_CYAN);
  screen.setCursor(65, 15);
  screen.print("SERVO MONITOR");
  screen.drawFastHLine(0, 45, 320, ST77XX_BLUE);

  screen.setTextColor(ST77XX_WHITE);
  screen.setCursor(25, 72);
  screen.print("Angle:");
  screen.setCursor(25, 112);
  screen.print("Duty:");
  screen.drawRect(25, 165, 270, 30, ST77XX_WHITE);
  screen.setTextSize(1);
  screen.setCursor(25, 205);
  screen.print("0 deg");
  screen.setCursor(262, 205);
  screen.print("180");
}

void updateScreen(int angle, int duty) {
  screen.setTextSize(2);
  screen.setTextColor(ST77XX_YELLOW);

  screen.fillRect(130, 68, 170, 25, ST77XX_BLACK);
  screen.setCursor(140, 72);
  screen.print(angle);
  screen.print(" deg");

  screen.fillRect(130, 108, 170, 25, ST77XX_BLACK);
  screen.setCursor(140, 112);
  screen.print(duty);

  screen.fillRect(26, 166, 268, 28, ST77XX_BLACK);
  int barWidth = map(angle, 0, 180, 0, 268);
  screen.fillRect(26, 166, barWidth, 28, ST77XX_GREEN);
}
