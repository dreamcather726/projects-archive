#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

const int8_t TFT_CS_PIN = 14;
const int8_t TFT_DC_PIN = 13;
const int8_t TFT_RST_PIN = -1;
const int8_t TFT_SDA_PIN = 12;
const int8_t TFT_SCL_PIN = 11;

const uint16_t TFT_WIDTH = 240;
const uint16_t TFT_HEIGHT = 320;

Adafruit_ST7789 screen(TFT_CS_PIN, TFT_DC_PIN, TFT_SDA_PIN, TFT_SCL_PIN, TFT_RST_PIN);

float speedValue = 0.35;
uint8_t batteryPercent = 85;

void setup() {
    screen.init(TFT_WIDTH, TFT_HEIGHT);
    screen.setRotation(3);
    screen.fillScreen(ST77XX_BLACK);

    screen.setTextSize(3);
    screen.setTextColor(ST77XX_WHITE);
    screen.setCursor(10, 10);
    screen.print("Car Status");

    screen.setCursor(10, 50);
    screen.setTextColor(ST77XX_GREEN);
    screen.print("State: READY");

    screen.setCursor(10, 90);
    screen.setTextColor(ST77XX_CYAN);
    screen.print("Speed:");

    screen.setCursor(10, 130);
    screen.setTextColor(ST77XX_YELLOW);
    screen.print("Battery:");

    screen.setCursor(10, 170);
    screen.setTextColor(ST77XX_MAGENTA);
    screen.print("Mode: AUTO");

    updateValues();
}

void loop() {
    speedValue = speedValue + 0.05;
    if (speedValue > 0.8) {
        speedValue = 0.35;
    }

    batteryPercent--;
    if (batteryPercent < 20) {
        batteryPercent = 85;
    }

    updateValues();
    delay(1000);
}

void updateValues() {
    screen.fillRect(135, 90, 180, 28, ST77XX_BLACK);
    screen.setCursor(135, 90);
    screen.setTextColor(ST77XX_CYAN);
    screen.print(speedValue);
    screen.print(" m/s");

    screen.fillRect(170, 130, 120, 28, ST77XX_BLACK);
    screen.setCursor(170, 130);
    screen.setTextColor(ST77XX_YELLOW);
    screen.print(batteryPercent);
    screen.print("%");
}
