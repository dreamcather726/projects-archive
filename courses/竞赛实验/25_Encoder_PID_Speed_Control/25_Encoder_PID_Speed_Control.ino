const uint8_t MOTOR_STANDBY_PIN = 8;

// TB6612 A 通道控制一个带编码器的电机。
const uint8_t MOTOR_IN1_PIN = 9;
const uint8_t MOTOR_IN2_PIN = 10;
const uint8_t MOTOR_PWM_PIN = 11;

// 本节只读取编码器 A 相，先学习测速和闭环控制。
const uint8_t ENCODER_PIN = 4;

// 目标速度使用“每秒编码器脉冲数”，便于初学者直接观察。
const float TARGET_PULSE_PER_SECOND = 120.0F;
const float KP = 1.2F;
const float KI = 0.08F;
const float KD = 0.0F;
const float INTEGRAL_LIMIT = 600.0F;

// PID 控制周期要稳定，速度打印可以慢一点，方便看串口。
const unsigned long CONTROL_INTERVAL_MS = 10;
const unsigned long PRINT_INTERVAL_MS = 500;

// 中断函数和主循环都会访问这个变量，所以要加 volatile。
volatile uint32_t encoderPulseCount = 0;

uint32_t lastPulseCount = 0;
float motorIntegral = 0.0F;
float lastMotorError = 0.0F;
float measuredPulsePerSecond = 0.0F;
int16_t outputPwm = 0;

unsigned long lastControlTimeMs = 0;
unsigned long lastPrintTimeMs = 0;

void setup() {
    Serial.begin(115200);

    pinMode(MOTOR_STANDBY_PIN, OUTPUT);
    pinMode(MOTOR_IN1_PIN, OUTPUT);
    pinMode(MOTOR_IN2_PIN, OUTPUT);
    pinMode(MOTOR_PWM_PIN, OUTPUT);
    pinMode(ENCODER_PIN, INPUT_PULLUP);

    // 编码器每来一个上升沿，就自动进入中断函数计数。
    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN), countEncoderPulse, RISING);

    digitalWrite(MOTOR_STANDBY_PIN, HIGH);
}

void loop() {
    unsigned long currentTimeMs = millis();
    if (currentTimeMs - lastControlTimeMs >= CONTROL_INTERVAL_MS) {
        updateSpeedPid(currentTimeMs - lastControlTimeMs);
        lastControlTimeMs = currentTimeMs;
    }

    if (currentTimeMs - lastPrintTimeMs >= PRINT_INTERVAL_MS) {
        Serial.print("target_pulse_s=");
        Serial.print(TARGET_PULSE_PER_SECOND);
        Serial.print(" measured_pulse_s=");
        Serial.print(measuredPulsePerSecond);
        Serial.print(" pwm=");
        Serial.println(outputPwm);
        lastPrintTimeMs = currentTimeMs;
    }
}

void updateSpeedPid(unsigned long intervalMs) {
    // 读取中断变量时短暂关中断，避免读到一半数值被改掉。
    noInterrupts();
    uint32_t currentPulseCount = encoderPulseCount;
    interrupts();

    // 本周期新增脉冲数 / 周期时间 = 当前速度。
    uint32_t pulseDelta = currentPulseCount - lastPulseCount;
    float intervalSeconds = intervalMs / 1000.0F;
    measuredPulsePerSecond = pulseDelta / intervalSeconds;

    outputPwm = calculatePidPwm(
        TARGET_PULSE_PER_SECOND,
        measuredPulsePerSecond,
        intervalSeconds);

    setMotorSpeed(outputPwm);
    lastPulseCount = currentPulseCount;
}

int16_t calculatePidPwm(float targetSpeed, float measuredSpeed, float intervalSeconds) {
    float error = targetSpeed - measuredSpeed;

    // I 项会累积历史误差，需要限制范围，避免积分饱和。
    motorIntegral += error * intervalSeconds;
    motorIntegral = constrain(motorIntegral, -INTEGRAL_LIMIT, INTEGRAL_LIMIT);

    // D 项观察误差变化速度，初学阶段 KD 可以保持 0。
    float derivative = (error - lastMotorError) / intervalSeconds;
    float outputValue = KP * error + KI * motorIntegral + KD * derivative;
    lastMotorError = error;

    return constrain((int16_t)outputValue, -255, 255);
}

void setMotorSpeed(int16_t motorPwm) {
    int16_t limitedPwm = constrain(motorPwm, -255, 255);

    // PID 输出为正就正转，为负就反转。
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

void countEncoderPulse() {
    encoderPulseCount++;
}
