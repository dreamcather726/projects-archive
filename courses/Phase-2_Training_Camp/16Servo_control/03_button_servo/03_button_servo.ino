/*
  程序功能：使用按键消抖控制舵机角度，每次有效按键让目标角度增加 15°。
  使用方法：舵机信号接 GPIO4，按键接 GPIO5 和 GND；上传后逐次按下并松开按键。
  运行结果：舵机从 0°开始每次增加 15°，超过 180°后回到 0°，串口显示当前角度。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#define SERVO_PIN 4
#define SERVO_CHANNEL 0

const int BUTTON_PIN = 5;
const int ANGLE_STEP = 15;

int currentAngle = 0;
int lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  ledcAttachChannel(SERVO_PIN, 50, 14, SERVO_CHANNEL);
  setServoAngle(currentAngle);
}

void loop() {
  int buttonState = digitalRead(BUTTON_PIN);
  unsigned long currentTime = millis();

  // 只在松开变为按下，并且距离上次有效动作超过 50ms 时处理
  if (lastButtonState == HIGH && buttonState == LOW
      && currentTime - lastDebounceTime >= 50) {
    lastDebounceTime = currentTime;
    currentAngle = currentAngle + ANGLE_STEP;

    if (currentAngle > 180) {
      currentAngle = 0;
    }

    setServoAngle(currentAngle);
  }

  lastButtonState = buttonState;
}

void setServoAngle(int angle) {
  int duty = map(angle, 0, 180, 410, 2048);
  ledcWriteChannel(SERVO_CHANNEL, duty);

  Serial.print("Angle:");
  Serial.println(angle);
}
