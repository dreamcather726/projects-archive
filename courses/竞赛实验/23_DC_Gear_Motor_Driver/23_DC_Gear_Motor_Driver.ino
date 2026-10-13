const uint8_t MOTOR_STANDBY_PIN = 8;

// TB6612 的 A 通道：IN1/IN2 控制方向，PWM 控制速度。
const uint8_t MOTOR_IN1_PIN = 9;
const uint8_t MOTOR_IN2_PIN = 10;
const uint8_t MOTOR_PWM_PIN = 11;

// PWM 范围是 0~255，数值越大，电机通常转得越快。
const uint8_t DRIVE_PWM = 160;
const unsigned long ACTION_TIME_MS = 1500;
const unsigned long STOP_TIME_MS = 800;

void setup() {
    Serial.begin(115200);

    pinMode(MOTOR_STANDBY_PIN, OUTPUT);
    pinMode(MOTOR_IN1_PIN, OUTPUT);
    pinMode(MOTOR_IN2_PIN, OUTPUT);
    pinMode(MOTOR_PWM_PIN, OUTPUT);

    digitalWrite(MOTOR_STANDBY_PIN, HIGH);  // 拉高 STBY，驱动模块才会工作。
    stopMotor();
}

void loop() {
    Serial.println("forward");
    setMotorSpeed(DRIVE_PWM);
    delay(ACTION_TIME_MS);
    stopAndWait();

    Serial.println("backward");
    setMotorSpeed(-DRIVE_PWM);
    delay(ACTION_TIME_MS);
    stopAndWait();
}

void setMotorSpeed(int16_t motorPwm) {
    // 限制 PWM，避免传入超过电机驱动允许范围的数值。
    int16_t limitedPwm = constrain(motorPwm, -255, 255);

    // 正数正转，负数反转，0 表示停止滑行。
    if (limitedPwm > 0) {
        digitalWrite(MOTOR_IN1_PIN, HIGH);
        digitalWrite(MOTOR_IN2_PIN, LOW);
    } else if (limitedPwm < 0) {
        digitalWrite(MOTOR_IN1_PIN, LOW);
        digitalWrite(MOTOR_IN2_PIN, HIGH);
    } else {
        digitalWrite(MOTOR_IN1_PIN, LOW);
        digitalWrite(MOTOR_IN2_PIN, LOW);
    }

    analogWrite(MOTOR_PWM_PIN, abs(limitedPwm));  // PWM 只能写正数。
}

void stopMotor() {
    setMotorSpeed(0);
}

void stopAndWait() {
    stopMotor();
    delay(STOP_TIME_MS);
}
