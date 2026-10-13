#include "speed_pid.h"
#include "motor_driver.h"
#include "encoder.h"
#include "button_control.h"

const int CONTROL_TIME_MS = 50;       // PID计算间隔
const int PRINT_TIME_MS = 500;        // 串口打印间隔
const float MAX_TARGET_SPEED = 50.0f; // 最大目标速度
const float SPEED_STEP = 5.0f;        // 每次按键改变的速度
const float DEFAULT_TARGET_SPEED = 5.0f;

// PID参数
const float KP = 1.0f;
const float KI = 0.5f;
const float KD = 0.0f;

PID pidA;
PID pidB;

float targetSpeed = DEFAULT_TARGET_SPEED; // 两个轮子使用同一个目标速度
float pwmA = 0.0f;
float pwmB = 0.0f;
bool isRunning = false;

unsigned long lastControlTime = 0;
unsigned long lastPrintTime = 0;

void stopMotors()
{
  PID_Clear(&pidA);
  PID_Clear(&pidB);
  pwmA = 0;
  pwmB = 0;
  motor_set_pwm(0, 0);
}

void readKeys()
{
  // 停止键优先级最高，并立即关闭两个电机
  if (button_read(buttonPin3)) {
    isRunning = false;
    stopMotors();
    Serial.println("按钮状态：停止");
    return;
  }

  // 按下开始键后才允许电机运行
  if (button_read(buttonPin4)) {
    if (targetSpeed == 0) targetSpeed = DEFAULT_TARGET_SPEED;
    PID_Clear(&pidA);
    PID_Clear(&pidB);
    isRunning = true;
    Serial.printf("按钮状态：开始，目标速度：%.1f\n", targetSpeed);
    return;
  }

  // 加速键每次增加5cm/s，减速键每次减少5cm/s
  if (button_read(buttonPin1)) {
    targetSpeed += SPEED_STEP;
    Serial.println("按钮状态：加速");
  } else if (button_read(buttonPin2)) {
    // 速度逐步减小到0
    if (targetSpeed > 0) targetSpeed -= SPEED_STEP;
    Serial.println("按钮状态：减速");
  } else {
    return;
  }

  targetSpeed = constrain(targetSpeed, -MAX_TARGET_SPEED, MAX_TARGET_SPEED);
  Serial.printf("目标速度：%.1f\n", targetSpeed);
}

void controlMotors()
{
  encoder_update(CONTROL_TIME_MS / 1000.0f);

  if (!isRunning || targetSpeed == 0) {
    stopMotors();
  } else {
    pwmA = PID_IncPIDCal(&pidA, speedA, targetSpeed);
    pwmB = PID_IncPIDCal(&pidB, speedB, targetSpeed);
    // PWM会在电机库中限制为-150～150
    motor_set_pwm((int)pwmA, (int)pwmB);
  }
}

void setup()
{
  Serial.begin(115200);

  button_init();
  motor_init();
  encoder_init();

  PID_Init(&pidA, KP, KI, KD, MOTOR_MAX_PWM, -MOTOR_MAX_PWM);
  PID_Init(&pidB, KP, KI, KD, MOTOR_MAX_PWM, -MOTOR_MAX_PWM);
  stopMotors();
}

void loop()
{
  readKeys();

  unsigned long currentTime = millis();

  // 每50ms进行一次测速和PID计算
  if (currentTime - lastControlTime >= CONTROL_TIME_MS) {
    lastControlTime = currentTime;
    controlMotors();
  }

  // 输出：目标速度,A轮速度,A轮PWM,目标速度,B轮速度,B轮PWM
  if (currentTime - lastPrintTime >= PRINT_TIME_MS) {
    lastPrintTime = currentTime;
    float runningTargetSpeed = isRunning ? targetSpeed : 0.0f;
    Serial.printf("%.1f,%.2f,%.1f,%.1f,%.2f,%.1f\n",
                  runningTargetSpeed, speedA, pwmA,
                  runningTargetSpeed, speedB, pwmB);
  }
}
