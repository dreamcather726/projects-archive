/*
  程序功能：使用 millis() 非阻塞执行电机控制和编码器读取两个定时任务。
  使用方法：电机方向/PWM 接 GPIO 1/41，编码器 A/B 接 GPIO 4/5；
           抬起车轮后上传程序，串口波特率设为 115200。
  运行结果：每 1 秒切换一次电机启停状态，同时每 1 秒输出一次编码器累计计数。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

unsigned long lastMotorTime = 0;
unsigned long lastEncoderTime = 0;

const int MOTOR_DIR_PIN = 1;
const int MOTOR_PWM_PIN = 41;
const int ENCODER_A_PIN = 4;
const int ENCODER_B_PIN = 5;

const int PWM_FREQUENCY = 10000;
const int PWM_CHANNEL = 0;
const int PWM_RESOLUTION = 8;
const int HIGH_SPEED = 80;
const int LOW_SPEED = 0;

volatile long encoderCount = 0;
bool motorRunning = false;

void IRAM_ATTR countEncoderPulse() {
  encoderCount++;
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
}

void loop() {
  unsigned long currentTime = millis();

  // 任务1：每 1 秒切换一次电机启停状态。
  if (currentTime - lastMotorTime >= 1000) {
    lastMotorTime = currentTime;
    motorRunning = !motorRunning;

    if (motorRunning == false) {
      writeMotorPwm(LOW_SPEED);
    } else {
      writeMotorPwm(HIGH_SPEED);
    }
  }

  // 任务2：每 1 秒安全读取并输出一次编码器累计计数。
  if (currentTime - lastEncoderTime >= 1000) {
    lastEncoderTime = currentTime;

    noInterrupts();
    long countCopy = encoderCount;
    interrupts();

    Serial.print("Encoder count: ");
    Serial.println(countCopy);
  }
}
