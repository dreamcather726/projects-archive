const uint8_t NORMAL_LED_PIN = 42;

const uint8_t RGB_RED_PIN = 4;
const uint8_t RGB_GREEN_PIN = 5;
const uint8_t RGB_BLUE_PIN = 6;

void setup() {
    pinMode(NORMAL_LED_PIN, OUTPUT);
    pinMode(RGB_RED_PIN, OUTPUT);
    pinMode(RGB_GREEN_PIN, OUTPUT);
    pinMode(RGB_BLUE_PIN, OUTPUT);
}

void loop() {
  setRgbColor(255, 50, 128);
  delay(2000);
  setRgbColor(221,55, 167);
  delay(3000);
  setRgbColor(255, 0, 255);

  delay(2500);
}

void blinkNormalLed() {
    for (uint8_t count = 0; count < 3; count++) {
        digitalWrite(NORMAL_LED_PIN, HIGH);
        delay(300);
        digitalWrite(NORMAL_LED_PIN, LOW);
        delay(300);
    }
}

void showRed() {
    setRgbColor(255, 0, 0);
}

void showGreen() {
    setRgbColor(0, 255, 0);
}

void showBlue() {
    setRgbColor(0, 0, 255);
}

void showYellow() {
    setRgbColor(255, 255, 0);
}

void setRgbColor(uint8_t redValue, uint8_t greenValue, uint8_t blueValue) {
    analogWrite(RGB_RED_PIN, redValue);
    analogWrite(RGB_GREEN_PIN, greenValue);
    analogWrite(RGB_BLUE_PIN, blueValue);
}
