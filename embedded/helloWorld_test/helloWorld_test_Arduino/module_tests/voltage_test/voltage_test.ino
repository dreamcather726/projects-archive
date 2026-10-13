#include <Arduino.h>

// Battery voltage divider output connected to this ADC pin.
static constexpr uint8_t VOLTAGE_ADC_PIN = 14;

// Battery voltage / ADC pin voltage. Change this after measuring the divider.
// The main project currently uses 10.0, for example 90k top + 10k bottom.
static constexpr float VOLTAGE_DIVIDER_RATIO = 10.0f;
static constexpr uint8_t SAMPLE_COUNT = 32;
static constexpr uint32_t PRINT_INTERVAL_MS = 500;

static void readAndPrintVoltage()
{
  uint32_t raw_sum = 0;
  uint32_t millivolt_sum = 0;
  uint16_t raw_min = 4095;
  uint16_t raw_max = 0;

  for (uint8_t i = 0; i < SAMPLE_COUNT; ++i) {
    const uint16_t raw = analogRead(VOLTAGE_ADC_PIN);
    const uint32_t millivolts = analogReadMilliVolts(VOLTAGE_ADC_PIN);
    raw_sum += raw;
    millivolt_sum += millivolts;
    raw_min = min(raw_min, raw);
    raw_max = max(raw_max, raw);
    delay(2);
  }

  const float raw_average = static_cast<float>(raw_sum) / SAMPLE_COUNT;
  const float adc_voltage =
      static_cast<float>(millivolt_sum) / SAMPLE_COUNT / 1000.0f;
  const float input_voltage = adc_voltage * VOLTAGE_DIVIDER_RATIO;

  Serial.printf("RAW=%7.1f range=%u..%u  ADC=%.4f V  INPUT=%.3f V  SCALE=x%.3f\n",
                static_cast<double>(raw_average), raw_min, raw_max,
                static_cast<double>(adc_voltage),
                static_cast<double>(input_voltage),
                static_cast<double>(VOLTAGE_DIVIDER_RATIO));

  if (raw_max >= 4090) {
    Serial.println("WARNING: ADC is near full scale; reduce the input or divider ratio.");
  }
}

void setup()
{
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== VOLTAGE INPUT TEST ===");
  Serial.println("ADC pin: GPIO14, attenuation: 11 dB, resolution: 12 bit");
  Serial.println("WARNING: The voltage directly on GPIO14 must not exceed 3.3 V.");

  pinMode(VOLTAGE_ADC_PIN, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(VOLTAGE_ADC_PIN, ADC_11db);
}

void loop()
{
  static uint32_t last_print_ms = 0;
  const uint32_t now_ms = millis();
  if (now_ms - last_print_ms >= PRINT_INTERVAL_MS) {
    last_print_ms = now_ms;
    readAndPrintVoltage();
  }
  delay(1);
}
