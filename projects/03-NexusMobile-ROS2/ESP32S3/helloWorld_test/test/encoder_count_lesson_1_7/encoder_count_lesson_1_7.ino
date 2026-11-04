#include <Arduino.h>

static constexpr uint8_t ENCODER_PIN = 6;
static constexpr uint32_t PRINT_INTERVAL_MS = 500;

static volatile int32_t encoder_count = 0;
static uint32_t last_print_ms = 0;

static void IRAM_ATTR onEncoderPulse()
{
  encoder_count++;
}

static int32_t getEncoderCount()
{
  noInterrupts();
  const int32_t safe_encoder_count = encoder_count;
  interrupts();
  return safe_encoder_count;
}

void setup()
{
  Serial.begin(115200);
  pinMode(ENCODER_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN), onEncoderPulse, RISING);
}

void loop()
{
  const uint32_t now_ms = millis();
  if (now_ms - last_print_ms < PRINT_INTERVAL_MS) {
    return;
  }
  last_print_ms = now_ms;

  Serial.print("encoder_count=");
  Serial.println(getEncoderCount());
}