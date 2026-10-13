#include <Arduino.h>

// Passive buzzer signal pin used by the main project.
static constexpr uint8_t BUZZER_PIN = 48;
static constexpr uint16_t TONE_DURATION_MS = 300;

static void playTone(uint16_t frequency_hz)
{
  Serial.printf("Playing %u Hz for %u ms\n", frequency_hz, TONE_DURATION_MS);
  tone(BUZZER_PIN, frequency_hz, TONE_DURATION_MS);
}

static void playStartupTest()
{
  const uint16_t frequencies[] = {1000, 2000, 3000};
  for (uint8_t i = 0; i < 3; ++i) {
    playTone(frequencies[i]);
    delay(TONE_DURATION_MS + 150);
  }
  noTone(BUZZER_PIN);
  digitalWrite(BUZZER_PIN, LOW);
}

static void printHelp()
{
  Serial.println("Commands: 1=1000Hz, 2=2000Hz, 3=3000Hz, a=three-tone test, s=stop");
}

void setup()
{
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  delay(500);

  Serial.println("\n=== PASSIVE BUZZER TEST ===");
  Serial.println("Signal pin: GPIO48");
  Serial.println("The buzzer should play three ascending tones once.");
  playStartupTest();
  printHelp();
}

void loop()
{
  if (!Serial.available()) {
    delay(1);
    return;
  }

  const char command = static_cast<char>(Serial.read());
  switch (command) {
    case '1': playTone(1000); break;
    case '2': playTone(2000); break;
    case '3': playTone(3000); break;
    case 'a':
    case 'A': playStartupTest(); break;
    case 's':
    case 'S':
      noTone(BUZZER_PIN);
      digitalWrite(BUZZER_PIN, LOW);
      Serial.println("Buzzer stopped");
      break;
    case '?': printHelp(); break;
    default: break;
  }
}
