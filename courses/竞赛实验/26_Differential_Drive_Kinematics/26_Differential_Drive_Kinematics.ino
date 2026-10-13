const uint8_t MOTOR_STANDBY_PIN = 8;

// 第26课开始使用两个电机，左右轮分别控制。
const uint8_t LEFT_MOTOR_IN1_PIN = 9;
const uint8_t LEFT_MOTOR_IN2_PIN = 10;
const uint8_t LEFT_MOTOR_PWM_PIN = 11;

const uint8_t RIGHT_MOTOR_IN1_PIN = 12;
const uint8_t RIGHT_MOTOR_IN2_PIN = 13;
const uint8_t RIGHT_MOTOR_PWM_PIN = 14;

// 轮径和轮距会影响运动学计算，实物底盘需要按实际测量修改。
const float WHEEL_DIAMETER_CM = 6.5F;
const float WHEEL_BASE_CM = 14.0F;
const float MAX_WHEEL_SPEED_CM_PER_SECOND = 35.0F;

const unsigned long ACTION_TIME_MS = 2500;
const unsigned long PRINT_INTERVAL_MS = 500;

struct WheelSpeed {
    // 左右轮各自的线速度。
    float leftCmPerSecond;
    float rightCmPerSecond;
};

struct RobotSpeed {
    // 机器人整体前进速度和转弯角速度。
    float linearCmPerSecond;
    float angularRadPerSecond;
};

uint8_t actionIndex = 0;
unsigned long actionStartTimeMs = 0;
unsigned long lastPrintTimeMs = 0;

void setup() {
    Serial.begin(115200);

    pinMode(MOTOR_STANDBY_PIN, OUTPUT);
    pinMode(LEFT_MOTOR_IN1_PIN, OUTPUT);
    pinMode(LEFT_MOTOR_IN2_PIN, OUTPUT);
    pinMode(LEFT_MOTOR_PWM_PIN, OUTPUT);
    pinMode(RIGHT_MOTOR_IN1_PIN, OUTPUT);
    pinMode(RIGHT_MOTOR_IN2_PIN, OUTPUT);
    pinMode(RIGHT_MOTOR_PWM_PIN, OUTPUT);

    digitalWrite(MOTOR_STANDBY_PIN, HIGH);  // 拉高 STBY，两个电机通道才会工作。
    actionStartTimeMs = millis();
}

void loop() {
    unsigned long currentTimeMs = millis();
    if (currentTimeMs - actionStartTimeMs >= ACTION_TIME_MS) {
        actionIndex = (actionIndex + 1) % 4;
        actionStartTimeMs = currentTimeMs;
    }

    RobotSpeed targetRobotSpeed = getTargetRobotSpeed(actionIndex);
    // 逆运动学：把机器人目标速度换算成左右轮速度。
    WheelSpeed targetWheelSpeed = calculateWheelSpeed(targetRobotSpeed);

    int16_t leftPwm = convertWheelSpeedToPwm(targetWheelSpeed.leftCmPerSecond);
    int16_t rightPwm = convertWheelSpeedToPwm(targetWheelSpeed.rightCmPerSecond);
    driveMotors(leftPwm, rightPwm);

    if (currentTimeMs - lastPrintTimeMs >= PRINT_INTERVAL_MS) {
        printKinematics(targetRobotSpeed, targetWheelSpeed, leftPwm, rightPwm);
        lastPrintTimeMs = currentTimeMs;
    }
}

RobotSpeed getTargetRobotSpeed(uint8_t currentActionIndex) {
    if (currentActionIndex == 0) {
        return {18.0F, 0.0F};
    }

    if (currentActionIndex == 1) {
        return {0.0F, 1.2F};
    }

    if (currentActionIndex == 2) {
        return {16.0F, -0.8F};
    }

    return {0.0F, 0.0F};
}

WheelSpeed calculateWheelSpeed(RobotSpeed robotSpeed) {
    // 转弯时，左右轮速度差由角速度和轮距决定。
    float halfTurnSpeed = robotSpeed.angularRadPerSecond * WHEEL_BASE_CM / 2.0F;
    WheelSpeed wheelSpeed;
    wheelSpeed.leftCmPerSecond = robotSpeed.linearCmPerSecond - halfTurnSpeed;
    wheelSpeed.rightCmPerSecond = robotSpeed.linearCmPerSecond + halfTurnSpeed;
    return wheelSpeed;
}

RobotSpeed calculateRobotSpeed(WheelSpeed wheelSpeed) {
    // 正运动学：由左右轮速度反推机器人整体速度。
    RobotSpeed robotSpeed;
    robotSpeed.linearCmPerSecond = (wheelSpeed.leftCmPerSecond + wheelSpeed.rightCmPerSecond) / 2.0F;
    robotSpeed.angularRadPerSecond = (wheelSpeed.rightCmPerSecond - wheelSpeed.leftCmPerSecond) / WHEEL_BASE_CM;
    return robotSpeed;
}

int16_t convertWheelSpeedToPwm(float wheelSpeedCmPerSecond) {
    // 这里用简单比例换算，真实电机并不是完全线性的。
    float pwm = wheelSpeedCmPerSecond / MAX_WHEEL_SPEED_CM_PER_SECOND * 255.0F;
    return constrain((int16_t)pwm, -255, 255);
}

void printKinematics(
    RobotSpeed robotSpeed,
    WheelSpeed wheelSpeed,
    int16_t leftPwm,
    int16_t rightPwm) {
    Serial.print("linear_cm_s=");
    Serial.print(robotSpeed.linearCmPerSecond);
    Serial.print(" angular_rad_s=");
    Serial.print(robotSpeed.angularRadPerSecond);
    Serial.print(" left_cm_s=");
    Serial.print(wheelSpeed.leftCmPerSecond);
    Serial.print(" right_cm_s=");
    Serial.print(wheelSpeed.rightCmPerSecond);
    Serial.print(" left_pwm=");
    Serial.print(leftPwm);
    Serial.print(" right_pwm=");
    Serial.println(rightPwm);
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
