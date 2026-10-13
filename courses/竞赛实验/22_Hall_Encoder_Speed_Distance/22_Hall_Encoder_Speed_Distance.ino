const uint8_t ENCODER_PIN = 4;

const uint16_t PULSES_PER_REVOLUTION = 20;
const float WHEEL_DIAMETER_CM = 6.5F;
const unsigned long SAMPLE_INTERVAL_MS = 1000;

volatile uint32_t encoderPulseCount = 0;
uint32_t lastPulseCount = 0;
unsigned long lastSampleTimeMs = 0;

void setup() {
    Serial.begin(115200);

    pinMode(ENCODER_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN), countEncoderPulse, RISING);
}

void loop() {
    unsigned long currentTimeMs = millis();
    if (currentTimeMs - lastSampleTimeMs < SAMPLE_INTERVAL_MS) {
        return;
    }

    noInterrupts();
    uint32_t currentPulseCount = encoderPulseCount;
    interrupts();

    uint32_t samplePulseCount = currentPulseCount - lastPulseCount;
    float wheelRpm = calculateWheelRpm(samplePulseCount);
    float distanceCm = calculateDistanceCm(currentPulseCount);

    Serial.print("rpm=");
    Serial.print(wheelRpm);
    Serial.print(" distance_cm=");
    Serial.println(distanceCm);

    lastPulseCount = currentPulseCount;
    lastSampleTimeMs = currentTimeMs;
}

void countEncoderPulse() {
    encoderPulseCount++;
}

float calculateWheelRpm(uint32_t samplePulseCount) {
    float revolutions = (float)samplePulseCount / PULSES_PER_REVOLUTION;
    return revolutions * 60.0F;
}

float calculateDistanceCm(uint32_t totalPulseCount) {
    float revolutions = (float)totalPulseCount / PULSES_PER_REVOLUTION;
    float wheelCircumferenceCm = WHEEL_DIAMETER_CM * 3.14159F;
    return revolutions * wheelCircumferenceCm;
}
