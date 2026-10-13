const uint8_t MOTOR_STANDBY_PIN = 8;

// 第27课使用左右两个电机和两个编码器。
const uint8_t LEFT_MOTOR_IN1_PIN = 9;
const uint8_t LEFT_MOTOR_IN2_PIN = 10;
const uint8_t LEFT_MOTOR_PWM_PIN = 11;

const uint8_t RIGHT_MOTOR_IN1_PIN = 12;
const uint8_t RIGHT_MOTOR_IN2_PIN = 13;
const uint8_t RIGHT_MOTOR_PWM_PIN = 14;

const uint8_t LEFT_ENCODER_PIN = 4;
const uint8_t RIGHT_ENCODER_PIN = 5;

// 这些参数决定“脉冲数”如何换算成真实距离。
const uint16_t PULSES_PER_REVOLUTION = 20;
const float WHEEL_DIAMETER_CM = 6.5F;
const float WHEEL_BASE_CM = 14.0F;

// 接近目标时用慢速，减少冲过头。
const uint8_t FAST_PWM = 150;
const uint8_t SLOW_PWM = 90;
const float SLOW_DOWN_DISTANCE_CM = 12.0F;
const unsigned long STEP_PAUSE_MS = 700;

enum MotionStep {
    // 把复杂路线拆成几个简单步骤。
    STEP_FORWARD_FIRST,
    STEP_TURN_RIGHT,
    STEP_FORWARD_SECOND,
    STEP_FINISHED
};

volatile uint32_t leftEncoderPulseCount = 0;
volatile uint32_t rightEncoderPulseCount = 0;

MotionStep currentStep = STEP_FORWARD_FIRST;
uint32_t stepStartLeftPulseCount = 0;
uint32_t stepStartRightPulseCount = 0;
unsigned long stepFinishedTimeMs = 0;
bool isStepPaused = false;

void setup() {
    Serial.begin(115200);

    pinMode(MOTOR_STANDBY_PIN, OUTPUT);
    pinMode(LEFT_MOTOR_IN1_PIN, OUTPUT);
    pinMode(LEFT_MOTOR_IN2_PIN, OUTPUT);
    pinMode(LEFT_MOTOR_PWM_PIN, OUTPUT);
    pinMode(RIGHT_MOTOR_IN1_PIN, OUTPUT);
    pinMode(RIGHT_MOTOR_IN2_PIN, OUTPUT);
    pinMode(RIGHT_MOTOR_PWM_PIN, OUTPUT);
    pinMode(LEFT_ENCODER_PIN, INPUT_PULLUP);
    pinMode(RIGHT_ENCODER_PIN, INPUT_PULLUP);

    // 编码器脉冲用中断计数，轮子转动时不容易漏数。
    attachInterrupt(digitalPinToInterrupt(LEFT_ENCODER_PIN), countLeftEncoderPulse, RISING);
    attachInterrupt(digitalPinToInterrupt(RIGHT_ENCODER_PIN), countRightEncoderPulse, RISING);

    digitalWrite(MOTOR_STANDBY_PIN, HIGH);
    startNewStep();
}

void loop() {
    if (currentStep == STEP_FINISHED) {
        driveMotors(0, 0);
        return;
    }

    if (isStepPaused) {
        waitBeforeNextStep();
        return;
    }

    if (currentStep == STEP_FORWARD_FIRST || currentStep == STEP_FORWARD_SECOND) {
        runDistanceStep(50.0F, FAST_PWM, FAST_PWM);
    } else if (currentStep == STEP_TURN_RIGHT) {
        // 原地右转时，左右轮走相同距离，但方向相反。
        float turnWheelDistanceCm = calculateTurnWheelDistanceCm(90.0F);
        runDistanceStep(turnWheelDistanceCm, FAST_PWM, -FAST_PWM);
    }
}

void runDistanceStep(float targetDistanceCm, int16_t leftDirectionPwm, int16_t rightDirectionPwm) {
    float traveledDistanceCm = getAverageStepDistanceCm();
    float remainingDistanceCm = targetDistanceCm - traveledDistanceCm;

    if (remainingDistanceCm <= 0.0F) {
        finishCurrentStep();
        return;
    }

    // 离目标近时减速，帮助停得更准。
    uint8_t drivePwm = remainingDistanceCm < SLOW_DOWN_DISTANCE_CM ? SLOW_PWM : FAST_PWM;
    int16_t leftPwm = leftDirectionPwm > 0 ? drivePwm : -drivePwm;
    int16_t rightPwm = rightDirectionPwm > 0 ? drivePwm : -drivePwm;
    driveMotors(leftPwm, rightPwm);

    Serial.print("step=");
    Serial.print((int)currentStep);
    Serial.print(" traveled_cm=");
    Serial.print(traveledDistanceCm);
    Serial.print(" remaining_cm=");
    Serial.println(remainingDistanceCm);

    delay(80);
}

void startNewStep() {
    // 记录本步骤开始时的编码器脉冲数，后面用差值算距离。
    noInterrupts();
    stepStartLeftPulseCount = leftEncoderPulseCount;
    stepStartRightPulseCount = rightEncoderPulseCount;
    interrupts();

    isStepPaused = false;
    Serial.print("start_step=");
    Serial.println((int)currentStep);
}

void finishCurrentStep() {
    driveMotors(0, 0);
    isStepPaused = true;
    stepFinishedTimeMs = millis();

    Serial.print("finish_step=");
    Serial.println((int)currentStep);
}

void waitBeforeNextStep() {
    if (millis() - stepFinishedTimeMs < STEP_PAUSE_MS) {
        driveMotors(0, 0);
        return;
    }

    currentStep = (MotionStep)((int)currentStep + 1);
    startNewStep();
}

float getAverageStepDistanceCm() {
    // 读取编码器计数时关中断，避免读数过程中发生变化。
    noInterrupts();
    uint32_t currentLeftPulseCount = leftEncoderPulseCount;
    uint32_t currentRightPulseCount = rightEncoderPulseCount;
    interrupts();

    uint32_t leftPulseDelta = currentLeftPulseCount - stepStartLeftPulseCount;
    uint32_t rightPulseDelta = currentRightPulseCount - stepStartRightPulseCount;
    float leftDistanceCm = calculateDistanceCm(leftPulseDelta);
    float rightDistanceCm = calculateDistanceCm(rightPulseDelta);

    return (leftDistanceCm + rightDistanceCm) / 2.0F;
}

float calculateDistanceCm(uint32_t pulseCount) {
    // 脉冲数 -> 圈数 -> 距离。
    float revolutions = (float)pulseCount / PULSES_PER_REVOLUTION;
    float wheelCircumferenceCm = WHEEL_DIAMETER_CM * 3.14159F;
    return revolutions * wheelCircumferenceCm;
}

float calculateTurnWheelDistanceCm(float turnAngleDegree) {
    // 原地转弯时，每个轮子走的是以轮距为直径的圆弧。
    float robotCircleCm = WHEEL_BASE_CM * 3.14159F;
    return robotCircleCm * turnAngleDegree / 360.0F;
}

void driveMotors(int16_t leftPwm, int16_t rightPwm) {
    setMotorSpeed(LEFT_MOTOR_IN1_PIN, LEFT_MOTOR_IN2_PIN, LEFT_MOTOR_PWM_PIN, leftPwm);
    setMotorSpeed(RIGHT_MOTOR_IN1_PIN, RIGHT_MOTOR_IN2_PIN, RIGHT_MOTOR_PWM_PIN, rightPwm);
}

void setMotorSpeed(uint8_t in1Pin, uint8_t in2Pin, uint8_t pwmPin, int16_t motorPwm) {
    int16_t limitedPwm = constrain(motorPwm, -255, 255);

    if (limitedPwm > 0) {
        digitalWrite(in1Pin, HIGH);
        digitalWrite(in2Pin, LOW);
    } else if (limitedPwm < 0) {
        digitalWrite(in1Pin, LOW);
        digitalWrite(in2Pin, HIGH);
    } else {
        digitalWrite(in1Pin, LOW);
        digitalWrite(in2Pin, LOW);
    }

    analogWrite(pwmPin, abs(limitedPwm));
}

void countLeftEncoderPulse() {
    leftEncoderPulseCount++;
}

void countRightEncoderPulse() {
    rightEncoderPulseCount++;
}
