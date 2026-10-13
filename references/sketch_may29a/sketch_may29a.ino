#include <Servo.h>

// 舵机引脚定义（保持不变）
#define BASE_SERVO_PIN 7
#define SHOULDER_SERVO_PIN 6
#define ELBOW_SERVO_PIN 4
#define WRIST_SERVO_PIN 5
#define HAND_SERVO_PIN 3
#define EXTRA_SERVO_PIN 2

// 按钮引脚定义（保持不变）
#define BUTTON_RECORD_PIN 10
#define BUTTON_PLAY_PIN 9
#define BUTTON_EXTRA_PIN 8

// 电位器引脚定义
#define POT_BASE_PIN A0
#define POT_SHOULDER_PIN A1
#define POT_WRIST_PIN A2
#define POT_ELBOW_PIN A3
#define POT_HAND_PIN A4

// 系统参数
const int MAX_STEPS = 10;          // 最大记录步数
const int DEBOUNCE_DELAY = 400;    // 按钮消抖时间(ms)
const int PLAY_INTERVAL = 50;      // 回放间隔时间(ms)
const int REAL_TIME_SPEED = 5;     // 实时控制速度
const int PLAY_SPEED = 3;          // 回放速度

// 舵机对象
Servo baseServo;
Servo shoulderServo;
Servo elbowServo;
Servo wristServo;
Servo handServo;
Servo extraServo;

// 系统状态
struct {
    int recordedSteps[MAX_STEPS][5];  // 5个主舵机角度
    bool extraServoState[MAX_STEPS];  // 额外舵机状态
    int currentStep = 0;              // 当前记录步数
    bool extraServoIsOpen = false;    // 额外舵机当前状态
    bool isPlaying = false;           // 回放状态标志
    int playStepIndex = 0;            // 当前回放步数
    unsigned long lastDebounceTime = 0;
    unsigned long lastPlayUpdateTime = 0;
} systemState;

// 平滑移动舵机
bool smoothMoveServo(Servo &servo, int targetAngle, int stepSize) {
    int currentAngle = servo.read();
    
    if (currentAngle < targetAngle) {
        currentAngle = min(currentAngle + stepSize, targetAngle);
    } else if (currentAngle > targetAngle) {
        currentAngle = max(currentAngle - stepSize, targetAngle);
    }
    
    servo.write(currentAngle);
    return (currentAngle == targetAngle);
}

// 实时控制舵机
void controlServo(int potPin, Servo &servo, int stepSize, bool reverse = false) {
    int potValue = analogRead(potPin);
    int targetAngle = reverse ? map(potValue, 0, 1023, 270, 0) 
                              : map(potValue, 0, 1023, 0, 270);
    smoothMoveServo(servo, targetAngle, stepSize);
}

// 记录当前状态
void recordCurrentState() {
    if (systemState.currentStep >= MAX_STEPS) return;
    
    systemState.recordedSteps[systemState.currentStep][0] = baseServo.read();
    systemState.recordedSteps[systemState.currentStep][1] = shoulderServo.read();
    systemState.recordedSteps[systemState.currentStep][2] = elbowServo.read();
    systemState.recordedSteps[systemState.currentStep][3] = wristServo.read();
    systemState.recordedSteps[systemState.currentStep][4] = handServo.read();
    systemState.extraServoState[systemState.currentStep] = systemState.extraServoIsOpen;
    
    Serial.print("记录动作 ");
    Serial.println(systemState.currentStep + 1);
    systemState.currentStep++;
}

// 回放记录
void playbackRecordedSteps() {
    if (millis() - systemState.lastPlayUpdateTime < PLAY_INTERVAL) return;
    systemState.lastPlayUpdateTime = millis();

    if (systemState.playStepIndex >= systemState.currentStep) {
        systemState.isPlaying = false;
        systemState.playStepIndex = 0;
        Serial.println("回放完成");
        return;
    }

    bool allReached = true;
    // 控制5个主舵机
    for (int i = 0; i < 5; i++) {
        Servo *servo;
        switch(i) {
            case 0: servo = &baseServo; break;
            case 1: servo = &shoulderServo; break;
            case 2: servo = &elbowServo; break;
            case 3: servo = &wristServo; break;
            case 4: servo = &handServo; break;
        }
        bool reached = smoothMoveServo(*servo, 
            systemState.recordedSteps[systemState.playStepIndex][i], 
            PLAY_SPEED);
        allReached &= reached;
    }
    
    // 控制额外舵机
    bool extraReached = smoothMoveServo(extraServo, 
        systemState.extraServoState[systemState.playStepIndex] ? 90 : 0, 
        PLAY_SPEED);
    allReached &= extraReached;

    if (allReached) {
        Serial.print("完成动作 ");
        Serial.println(systemState.playStepIndex + 1);
        systemState.playStepIndex++;
    }
}

// 处理按钮输入
void handleButtonInputs() {
    unsigned long currentTime = millis();
    if (currentTime - systemState.lastDebounceTime < DEBOUNCE_DELAY) return;
    
    if (digitalRead(BUTTON_EXTRA_PIN) == LOW) {
        systemState.lastDebounceTime = currentTime;
        systemState.extraServoIsOpen = !systemState.extraServoIsOpen;
        extraServo.write(systemState.extraServoIsOpen ? 90 : 0);
        Serial.print("额外舵机: ");
        Serial.println(systemState.extraServoIsOpen ? "开" : "关");
    }
    
    if (digitalRead(BUTTON_RECORD_PIN) == LOW) {
        systemState.lastDebounceTime = currentTime;
        recordCurrentState();
    }
    
    if (digitalRead(BUTTON_PLAY_PIN) == LOW && !systemState.isPlaying) {
        systemState.lastDebounceTime = currentTime;
        systemState.isPlaying = true;
        systemState.playStepIndex = 0;
        Serial.println("开始回放");
    }
}

void setup() {
    Serial.begin(9600);
    
    // 初始化舵机
    baseServo.attach(BASE_SERVO_PIN);
    shoulderServo.attach(SHOULDER_SERVO_PIN);
    elbowServo.attach(ELBOW_SERVO_PIN);
    wristServo.attach(WRIST_SERVO_PIN);
    handServo.attach(HAND_SERVO_PIN);
    extraServo.attach(EXTRA_SERVO_PIN);
    
    // 初始化按钮
    pinMode(BUTTON_RECORD_PIN, INPUT_PULLUP);
    pinMode(BUTTON_PLAY_PIN, INPUT_PULLUP);
    pinMode(BUTTON_EXTRA_PIN, INPUT_PULLUP);
}

void loop() {
    if (!systemState.isPlaying) {
        // 实时控制模式
        controlServo(POT_BASE_PIN, baseServo, REAL_TIME_SPEED, true);
        controlServo(POT_SHOULDER_PIN, shoulderServo, REAL_TIME_SPEED, true);
        controlServo(POT_WRIST_PIN, wristServo, REAL_TIME_SPEED, true);
        controlServo(POT_ELBOW_PIN, elbowServo, REAL_TIME_SPEED, true);
        controlServo(POT_HAND_PIN, handServo, REAL_TIME_SPEED, true);
    } else {
        // 回放模式
        playbackRecordedSteps();
    }
    
    // 处理按钮输入
    handleButtonInputs();
}