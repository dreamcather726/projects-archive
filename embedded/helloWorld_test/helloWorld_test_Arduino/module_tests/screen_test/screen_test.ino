#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// ST7789 four-wire software SPI pins used by the main project.
static constexpr int8_t TFT_CS_PIN = 47;
static constexpr int8_t TFT_DC_PIN = 21;
static constexpr int8_t TFT_RST_PIN = -1;
static constexpr int8_t TFT_MOSI_PIN = 20;
static constexpr int8_t TFT_SCLK_PIN = 19;
static constexpr uint16_t TFT_WIDTH = 240;
static constexpr uint16_t TFT_HEIGHT = 320;

static Adafruit_ST7789 screen(TFT_CS_PIN,
                             TFT_DC_PIN,
                             TFT_MOSI_PIN,
                             TFT_SCLK_PIN,
                             TFT_RST_PIN);

static void drawTestPattern()
{
  screen.fillScreen(ST77XX_BLACK);
  screen.setTextWrap(false);
  screen.setTextSize(2);
  screen.setTextColor(ST77XX_WHITE);
  screen.setCursor(10, 10);
  screen.print("ST7789 SCREEN TEST");

  const uint16_t colors[] = {
    ST77XX_RED, ST77XX_GREEN, ST77XX_BLUE,
    ST77XX_CYAN, ST77XX_MAGENTA, ST77XX_YELLOW, ST77XX_WHITE
  };
  const int16_t bar_width = screen.width() / 7;
  for (uint8_t i = 0; i < 7; ++i) {
    const int16_t x = i * bar_width;
    const int16_t width = (i == 6) ? screen.width() - x : bar_width;
    screen.fillRect(x, 45, width, 55, colors[i]);
  }

  screen.drawRect(10, 115, 90, 55, ST77XX_WHITE);
  screen.fillCircle(145, 142, 27, ST77XX_GREEN);
  screen.drawLine(190, 115, 300, 170, ST77XX_YELLOW);
  screen.drawLine(300, 115, 190, 170, ST77XX_YELLOW);

  screen.setTextSize(1);
  screen.setTextColor(ST77XX_CYAN);
  screen.setCursor(10, 188);
  screen.print("Color / geometry / refresh");
}

void setup()
{
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== ST7789 SCREEN TEST ===");
  Serial.println("Pins: CS=47 DC=21 MOSI=20 SCLK=19 RST=-1");

  screen.init(TFT_WIDTH, TFT_HEIGHT);
  screen.setRotation(3);
  drawTestPattern();
  Serial.println("Check the screen for color bars, shapes and a running counter.");
}

void loop()
{
  static uint32_t last_update_ms = 0;
  if (millis() - last_update_ms < 500) {
    return;
  }
  last_update_ms = millis();

  screen.fillRect(10, 208, 220, 24, ST77XX_BLACK);
  screen.setTextSize(2);
  screen.setTextColor(ST77XX_YELLOW);
  screen.setCursor(10, 208);
  screen.printf("UP: %lu s", static_cast<unsigned long>(millis() / 1000));
}
