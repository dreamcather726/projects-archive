#include <Arduino.h>

// The four buttons are active-low and use the ESP32-S3 internal pull-ups.
static constexpr uint8_t BUTTON_COUNT = 4;
static constexpr uint8_t BUTTON_PINS[BUTTON_COUNT] = {10, 11, 15, 16};
static constexpr uint32_t DEBOUNCE_MS = 20;

struct ButtonState {
  bool raw_pressed;
  bool stable_pressed;
  uint32_t raw_changed_ms;
};

static ButtonState buttons[BUTTON_COUNT];

static bool readPressed(uint8_t index)
{
  return digitalRead(BUTTON_PINS[index]) == LOW;
}

static void printAllStates()
{
  Serial.printf("STATE  GPIO10=%u GPIO11=%u GPIO15=%u GPIO16=%u\n",
                buttons[0].stable_pressed,
                buttons[1].stable_pressed,
                buttons[2].stable_pressed,
                buttons[3].stable_pressed);
}

void setup()
{
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== BUTTON TEST ===");
  Serial.println("Press each button. PRESSED and RELEASED should each print once.");

  for (uint8_t i = 0; i < BUTTON_COUNT; ++i) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    buttons[i].raw_pressed = readPressed(i);
    buttons[i].stable_pressed = buttons[i].raw_pressed;
    buttons[i].raw_changed_ms = millis();
  }
  printAllStates();
}

void loop()
{
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
      Serial.printf("GPIO%u %s\n", BUTTON_PINS[i], pressed ? "PRESSED" : "RELEASED");
      printAllStates();
    }
  }
  delay(1);
}
