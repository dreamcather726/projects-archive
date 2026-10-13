#include <ESP32Servo.h>

const uint8_t SERVO_PIN = 4;

const uint8_t OPEN_ANGLE = 30;
const uint8_t MIDDLE_ANGLE = 90;
const uint8_t CLOSE_ANGLE = 100;

Servo gripperServo;

void setup() {
    gripperServo.attach(SERVO_PIN);
}

void loop() {
    openGripper();
    delay(1000);

    moveToMiddle();
    delay(1000);

    closeGripper();
    delay(1000);
}

void openGripper() {
    gripperServo.write(OPEN_ANGLE);
}

void moveToMiddle() {
    gripperServo.write(MIDDLE_ANGLE);
}

void closeGripper() {
    gripperServo.write(CLOSE_ANGLE);
}
