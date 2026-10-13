#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#include "animation_frames.h"

constexpr int SCREEN_WIDTH = 128;
constexpr int SCREEN_HEIGHT = 64;
constexpr int OLED_RESET_PIN = -1;
constexpr int OLED_I2C_ADDRESS = 0x3C;

constexpr int I2C_SDA_PIN =8;
constexpr int I2C_SCL_PIN = 9;

constexpr int FRAME_DELAY_MS = 10;
constexpr int ANIMATION_FRAME_COUNT = 30;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET_PIN);

const unsigned char *const animationFrames[ANIMATION_FRAME_COUNT] = {
  epd_bitmap_frame_1,
  epd_bitmap_frame_2,
  epd_bitmap_frame_3,
  epd_bitmap_frame_4,
  epd_bitmap_frame_5,
  epd_bitmap_frame_6,
  epd_bitmap_frame_7,
  epd_bitmap_frame_8,
  epd_bitmap_frame_9,
  epd_bitmap_frame_10,
  epd_bitmap_frame_11,
  epd_bitmap_frame_12,
  epd_bitmap_frame_13,
  epd_bitmap_frame_14,
  epd_bitmap_frame_15,
  epd_bitmap_frame_16,
  epd_bitmap_frame_17,
  epd_bitmap_frame_18,
  epd_bitmap_frame_19,
  epd_bitmap_frame_20,
  epd_bitmap_frame_21,
  epd_bitmap_frame_22,
  epd_bitmap_frame_23,
  epd_bitmap_frame_24,
  epd_bitmap_frame_25,
  epd_bitmap_frame_26,
  epd_bitmap_frame_27,
  epd_bitmap_frame_28,
  epd_bitmap_frame_29,
  epd_bitmap_frame_30,
};

uint8_t currentFrameIndex = 0;

void drawAnimationFrame() {
  display.clearDisplay();
  display.drawBitmap(
    0,
    0,
    animationFrames[currentFrameIndex],
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    SSD1306_WHITE
  );
  display.display();

  currentFrameIndex = (currentFrameIndex + 1) % ANIMATION_FRAME_COUNT;
}

void setup() {
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) {
    while (true) {
      delay(1000);
    }
  }

  display.clearDisplay();
  display.display();
}

void loop() {
  drawAnimationFrame();
  delay(FRAME_DELAY_MS);
}
