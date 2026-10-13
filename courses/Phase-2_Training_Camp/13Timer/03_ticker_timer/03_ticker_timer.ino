/*
  程序功能：使用 Ticker 定时触发电机控制和编码器读取两个任务。
  使用方法：电机方向/PWM 接 GPIO 1/41，编码器 A/B 接 GPIO 4/5；
           抬起车轮后上传程序，串口波特率设为 115200。
  运行结果：Ticker 每 1 秒置位任务标志；主循环切换电机启停并输出编码器累计计数。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include <Ticker.h>

const int MOTOR_DIR_PIN = 1;
const int MOTOR_PWM_PIN = 41;
const int ENCODER_A_PIN = 4;
const int ENCODER_B_PIN = 5;

const int PWM_FREQUENCY = 10000;
const int PWM_CHANNEL = 0;
const int PWM_RESOLUTION = 8;
const int HIGH_SPEED = 80;
const int LOW_SPEED = 0;

Ticker taskTicker;

volatile long encoderCount = 0;
volatile bool motorTaskReady = false;
volatile bool encoderTaskReady = false;
bool motorRunning = false;

void IRAM_ATTR countEncoderPulse() {
  encoderCount++;
}

void onTimerTick() {
  motorTaskReady = true;
  encoderTaskReady = true;
}

void writeMotorPwm(int duty) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWriteChannel(PWM_CHANNEL, duty);
#else
  ledcWrite(PWM_CHANNEL, duty);
#endif
}

void setupMotorPwm() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttachChannel(
    MOTOR_PWM_PIN,
    PWM_FREQUENCY,
    PWM_RESOLUTION,
    PWM_CHANNEL
  );
#else
  ledcSetup(PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(MOTOR_PWM_PIN, PWM_CHANNEL);
#endif
  writeMotorPwm(LOW_SPEED);
}

void runMotorTask() {
  motorRunning = !motorRunning;

  if (motorRunning == false) {
    writeMotorPwm(LOW_SPEED);
  } else {
    writeMotorPwm(HIGH_SPEED);
  }
}

void runEncoderTask() {
  noInterrupts();
  long countCopy = encoderCount;
  interrupts();

  Serial.print("Encoder count: ");
  Serial.println(countCopy);
}

void setup() {
  Serial.begin(115200);

  pinMode(MOTOR_DIR_PIN, OUTPUT);
  digitalWrite(MOTOR_DIR_PIN, HIGH);
  setupMotorPwm();

  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);
  attachInterrupt(
    digitalPinToInterrupt(ENCODER_A_PIN),
    countEncoderPulse,
    RISING
  );

  taskTicker.attach_ms(1000, onTimerTick);
}

void loop() {
  if (motorTaskReady) {
    motorTaskReady = false;
    runMotorTask();
  }

  if (encoderTaskReady) {
    encoderTaskReady = false;
    runEncoderTask();
  }
}
