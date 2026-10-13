const uint8_t TOUCH_SWITCH_PIN = 4;
const uint8_t FORCE_SENSOR_PIN = 1;
const uint8_t ALERT_LED_PIN = 42;

const uint16_t LIGHT_FORCE_LIMIT = 900;
const uint16_t SAFE_FORCE_LIMIT = 1800;

void setup() {
    Serial.begin(115200);

    pinMode(TOUCH_SWITCH_PIN, INPUT_PULLUP);
    pinMode(ALERT_LED_PIN, OUTPUT);
}

void loop() {
    bool isTouched = digitalRead(TOUCH_SWITCH_PIN) == LOW;
    uint16_t forceAdcValue = analogRead(FORCE_SENSOR_PIN);

    if (isTouched || forceAdcValue > SAFE_FORCE_LIMIT) {
        digitalWrite(ALERT_LED_PIN, HIGH);
    } else {
        digitalWrite(ALERT_LED_PIN, LOW);
    }

    Serial.print("touch=");
    Serial.print(isTouched ? "yes" : "no");
    Serial.print(" force_adc=");
    Serial.print(forceAdcValue);
    Serial.print(" level=");
    Serial.println(getForceLevel(forceAdcValue));

    delay(100);
}

const char *getForceLevel(uint16_t forceAdcValue) {
    if (forceAdcValue < LIGHT_FORCE_LIMIT) {
        return "light";
    }

    if (forceAdcValue < SAFE_FORCE_LIMIT) {
        return "normal";
    }

    return "heavy";
}
