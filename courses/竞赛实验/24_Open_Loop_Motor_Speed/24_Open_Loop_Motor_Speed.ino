const uint8_t MOTOR_STANDBY_PIN = 8;

// 只使用 TB6612 的 A 通道控制一个电机。
const uint8_t MOTOR_IN1_PIN = 9;
const uint8_t MOTOR_IN2_PIN = 10;
const uint8_t MOTOR_PWM_PIN = 11;

// MIN_START_PWM 用来补偿电机刚启动时的死区。
const uint8_t MIN_START_PWM = 80;
const uint8_t TARGET_DRIVE_PWM = 150;
const uint8_t PWM_STEP = 5;

// 这些时间参数决定电机加速、运行和停顿的节奏。
const unsigned long RAMP_INTERVAL_MS = 60;
const unsigned long DRIVE_TIME_MS = 5000;
const unsigned long REST_TIME_MS = 1500;

// currentDrivePwm 会慢慢接近 targetPwm，实现软启动和软停止。
int16_t currentDrivePwm = 0;
unsigned long lastRampTimeMs = 0;
unsigned long actionStartTimeMs = 0;
bool isDriving = true;

void setup() {
    Serial.begin(115200);

    pinMode(MOTOR_STANDBY_PIN, OUTPUT);
    pinMode(MOTOR_IN1_PIN, OUTPUT);
    pinMode(MOTOR_IN2_PIN, OUTPUT);
    pinMode(MOTOR_PWM_PIN, OUTPUT);

    digitalWrite(MOTOR_STANDBY_PIN, HIGH);  // 拉高 STBY，驱动模块才会输出。
    actionStartTimeMs = millis();
}

void loop() {
    unsigned long currentTimeMs = millis();

    if (isDriving && currentTimeMs - actionStartTimeMs >= DRIVE_TIME_MS) {
        isDriving = false;
        actionStartTimeMs = currentTimeMs;
    }

    if (!isDriving && currentTimeMs - actionStartTimeMs >= REST_TIME_MS) {
        isDriving = true;
        actionStartTimeMs = currentTimeMs;
    }

    int16_t targetPwm = isDriving ? TARGET_DRIVE_PWM : 0;
    updateRampPwm(targetPwm);

    // 先做死区补偿，再真正输出到电机。
    int16_t motorPwm = applyDeadZone(currentDrivePwm);
    setMotorSpeed(motorPwm);

    Serial.print("target_pwm=");
    Serial.print(targetPwm);
    Serial.print(" current_pwm=");
    Serial.print(currentDrivePwm);
    Serial.print(" motor_pwm=");
    Serial.println(motorPwm);

    delay(100);
}

void updateRampPwm(int16_t targetPwm) {
    unsigned long currentTimeMs = millis();
    if (currentTimeMs - lastRampTimeMs < RAMP_INTERVAL_MS) {
        return;
    }

    // 每隔一小段时间只改变一点 PWM，电机速度就会更平滑。
    if (currentDrivePwm < targetPwm) {
        currentDrivePwm += PWM_STEP;
        if (currentDrivePwm > targetPwm) {
            currentDrivePwm = targetPwm;
        }
    } else if (currentDrivePwm > targetPwm) {
        currentDrivePwm -= PWM_STEP;
        if (currentDrivePwm < targetPwm) {
            currentDrivePwm = targetPwm;
        }
    }

    lastRampTimeMs = currentTimeMs;
}

int16_t applyDeadZone(int16_t motorPwm) {
    if (motorPwm == 0) {
        return 0;
    }

    // PWM 太小时电机可能不动，直接提升到最小启动 PWM。
    if (motorPwm > 0 && motorPwm < MIN_START_PWM) {
        return MIN_START_PWM;
    }

    if (motorPwm < 0 && motorPwm > -MIN_START_PWM) {
        return -MIN_START_PWM;
    }

    return constrain(motorPwm, -255, 255);
}

void setMotorSpeed(int16_t motorPwm) {
    int16_t limitedPwm = constrain(motorPwm, -255, 255);

    // 方向由 IN1/IN2 决定，速度由 PWM 决定。
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

    analogWrite(MOTOR_PWM_PIN, abs(limitedPwm));
}
