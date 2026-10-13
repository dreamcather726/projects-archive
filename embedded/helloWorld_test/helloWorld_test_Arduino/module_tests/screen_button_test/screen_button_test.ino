#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// ST7789 screen pins.
static constexpr int8_t TFT_CS_PIN = 47;
static constexpr int8_t TFT_DC_PIN = 21;
static constexpr int8_t TFT_RST_PIN = -1;
static constexpr int8_t TFT_MOSI_PIN = 20;
static constexpr int8_t TFT_SCLK_PIN = 19;
static constexpr uint16_t TFT_WIDTH = 240;
static constexpr uint16_t TFT_HEIGHT = 320;

// Four active-low buttons with internal pull-ups.
static constexpr uint8_t BUTTON_COUNT = 4;
static constexpr uint8_t BUTTON_PINS[BUTTON_COUNT] = {10, 11, 15, 16};
static constexpr uint32_t DEBOUNCE_MS = 20;

struct ButtonState {
  bool raw_pressed;
  bool stable_pressed;
  uint32_t raw_changed_ms;
  uint32_t press_count;
};

static Adafruit_ST7789 screen(TFT_CS_PIN,
                             TFT_DC_PIN,
                             TFT_MOSI_PIN,
                             TFT_SCLK_PIN,
                             TFT_RST_PIN);
static ButtonState buttons[BUTTON_COUNT];
static uint32_t total_events = 0;

static bool readPressed(uint8_t index)
{
  return digitalRead(BUTTON_PINS[index]) == LOW;
}

static void drawButtonCard(uint8_t index)
{
  static constexpr int16_t CARD_X = 6;
  static constexpr int16_t CARD_Y = 54;
  static constexpr int16_t CARD_WIDTH = 73;
  static constexpr int16_t CARD_HEIGHT = 124;
  static constexpr int16_t CARD_GAP = 5;

  const int16_t x = CARD_X + index * (CARD_WIDTH + CARD_GAP);
  const bool pressed = buttons[index].stable_pressed;
  const uint16_t idle_color = screen.color565(45, 45, 45);
  const uint16_t card_color = pressed ? ST77XX_GREEN : idle_color;

  screen.fillRect(x, CARD_Y, CARD_WIDTH, CARD_HEIGHT, ST77XX_BLACK);
  screen.drawRect(x, CARD_Y, CARD_WIDTH, CARD_HEIGHT,
                  pressed ? ST77XX_GREEN : ST77XX_WHITE);

  screen.setTextSize(2);
  screen.setTextColor(pressed ? ST77XX_GREEN : ST77XX_WHITE);
  screen.setCursor(x + 22, CARD_Y + 9);
  screen.printf("K%u", index + 1);

  screen.setTextSize(1);
  screen.setTextColor(ST77XX_CYAN);
  screen.setCursor(x + 17, CARD_Y + 35);
  screen.printf("GPIO%u", BUTTON_PINS[index]);

  screen.fillRoundRect(x + 5, CARD_Y + 52, CARD_WIDTH - 10, 31, 4, card_color);
  screen.setTextSize(2);
  screen.setTextColor(ST77XX_WHITE);
  screen.setCursor(x + (pressed ? 11 : 22), CARD_Y + 60);
  screen.print(pressed ? "DOWN" : "UP");

  screen.setTextSize(1);
  screen.setTextColor(ST77XX_YELLOW);
  screen.setCursor(x + 8, CARD_Y + 94);
  screen.print("PRESSES");
  screen.setTextSize(2);
  screen.setCursor(x + 8, CARD_Y + 105);
  screen.printf("%5lu", static_cast<unsigned long>(buttons[index].press_count));
}

static void drawHeader()
{
  screen.fillScreen(ST77XX_BLACK);
  screen.setTextWrap(false);
  screen.setTextSize(2);
  screen.setTextColor(ST77XX_CYAN);
  screen.setCursor(8, 8);
  screen.print("SCREEN + BUTTON TEST");
  screen.drawFastHLine(8, 34, 304, ST77XX_BLUE);

  screen.setTextSize(1);
  screen.setTextColor(ST77XX_WHITE);
  screen.setCursor(8, 40);
  screen.print("Press K1-K4; the matching card turns green");

  for (uint8_t i = 0; i < BUTTON_COUNT; ++i) {
    drawButtonCard(i);
  }
}

static void drawFooter(const char *event_text)
{
  screen.fillRect(0, 184, screen.width(), 56, ST77XX_BLACK);
  screen.setTextSize(1);
  screen.setTextColor(ST77XX_MAGENTA);
  screen.setCursor(8, 188);
  screen.print(event_text);

  screen.setTextColor(ST77XX_WHITE);
  screen.setCursor(8, 207);
  screen.printf("EVENTS: %lu", static_cast<unsigned long>(total_events));
  screen.setCursor(190, 207);
  screen.printf("UP: %lus", static_cast<unsigned long>(millis() / 1000));

  screen.setTextColor(ST77XX_GREEN);
  screen.setCursor(8, 226);
  screen.print("DISPLAY OK + BUTTON INPUT OK");
}

void setup()
{
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== SCREEN + BUTTON TEST ===");
  Serial.println("Screen: CS=47 DC=21 MOSI=20 SCLK=19");
  Serial.println("Buttons: GPIO10 GPIO11 GPIO15 GPIO16, active LOW");

  for (uint8_t i = 0; i < BUTTON_COUNT; ++i) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    buttons[i].raw_pressed = readPressed(i);
    buttons[i].stable_pressed = buttons[i].raw_pressed;
    buttons[i].raw_changed_ms = millis();
    buttons[i].press_count = 0;
  }

  screen.init(TFT_WIDTH, TFT_HEIGHT);
  screen.setRotation(3);
  drawHeader();
  drawFooter("READY: press any button");
}

void loop()
{
  static uint32_t last_uptime_redraw_ms = 0;
  static char latest_event[40] = "READY: press any button";
  const uint32_t now_ms = millis();

  for (uint8_t i = 0; i < BUTTON_COUNT; ++i) {
    const bool pressed = readPressed(i);
    if (pressed != buttons[i].raw_pressed) {
      buttons[i].raw_pressed = pressed;
      buttons[i].raw_changed_ms = now_ms;
    }

    if (pressed != buttons[i].stable_pressed &&
        now_ms - buttons[i].raw_changed_ms >= DEBOUNCE_MS) {
      buttons[i].stable_pressed = pressed;
      ++total_events;
      if (pressed) {
        ++buttons[i].press_count;
      }

      snprintf(latest_event, sizeof(latest_event), "K%u GPIO%u %s",
               i + 1, BUTTON_PINS[i], pressed ? "PRESSED" : "RELEASED");
      Serial.println(latest_event);
      drawButtonCard(i);
      drawFooter(latest_event);
    }
  }

  if (now_ms - last_uptime_redraw_ms >= 1000) {
    last_uptime_redraw_ms = now_ms;
    drawFooter(latest_event);
  }
  delay(1);
}
