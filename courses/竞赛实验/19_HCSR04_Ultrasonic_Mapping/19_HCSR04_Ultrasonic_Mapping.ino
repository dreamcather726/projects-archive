const uint8_t TRIG_PIN = 4;
const uint8_t ECHO_PIN = 5;

const uint16_t MAX_DISTANCE_CM = 400;
const unsigned long ECHO_TIMEOUT_US = 30000;

void setup() {
    Serial.begin(115200);

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
}

void loop() {
    float distanceCm = measureDistanceCm();

    Serial.print("distance_cm=");
    Serial.print(distanceCm);
    Serial.print(" obstacle=");
    Serial.println(isObstacleNear(distanceCm) ? "yes" : "no");

    delay(200);
}

float measureDistanceCm() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    unsigned long echoTimeUs = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);
    if (echoTimeUs == 0) {
        return MAX_DISTANCE_CM;
    }

    return echoTimeUs * 0.0343F / 2.0F;
}

bool isObstacleNear(float distanceCm) {
    return distanceCm > 2.0F && distanceCm < 25.0F;
}
