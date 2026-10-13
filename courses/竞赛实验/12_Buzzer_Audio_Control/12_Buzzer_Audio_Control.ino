const uint8_t ACTIVE_BUZZER_PIN = 7 ;//
const uint8_t PASSIVE_BUZZER_PIN = 8;//

const uint16_t NOTE_DO = 262;
const uint16_t NOTE_RE = 294;
const uint16_t NOTE_MI = 330;
const uint16_t NOTE_FA = 349;
const uint16_t NOTE_SOL = 392;

void setup() {
    pinMode(ACTIVE_BUZZER_PIN, OUTPUT);
}

void loop() {
    playAlarmSound();
    delay(1000);

    playTwoTigers();
    delay(2000);
}

void playAlarmSound() {
    for (uint8_t count = 0; count < 3; count++) {
        digitalWrite(ACTIVE_BUZZER_PIN, HIGH);
        delay(200);
        digitalWrite(ACTIVE_BUZZER_PIN, LOW);
        delay(200);
    }
}

void playTwoTigers() {
    playNote(NOTE_DO, 300);
    playNote(NOTE_RE, 300);
    playNote(NOTE_MI, 300);
    playNote(NOTE_DO, 300);

    playNote(NOTE_DO, 300);
    playNote(NOTE_RE, 300);
    playNote(NOTE_MI, 300);
    playNote(NOTE_DO, 300);

    playNote(NOTE_MI, 300);
    playNote(NOTE_FA, 300);
    playNote(NOTE_SOL, 600);
}

void playNote(uint16_t frequency, uint16_t durationMs) {
    tone(PASSIVE_BUZZER_PIN, frequency);
    delay(durationMs);
    noTone(PASSIVE_BUZZER_PIN);
    delay(80);
}
