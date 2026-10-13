/*
  程序功能：扩展学习编码器 4 倍频，对 A、B 两相的全部边沿进行计数。
  使用方法：完成 2 倍频实验后再使用；编码器 A、B 相接 GPIO 6、7并打开115200串口。
  运行结果：每个完整 A/B 周期最多计数 4 次，串口显示扩展实验的 4X Count。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

const int ENCODER_A_PIN = 6;
const int ENCODER_B_PIN = 7;

volatile long encoderCount = 0;
volatile int lastEncoderState = 0;

// 扩展内容：同时处理 A、B 两相的全部边沿
void IRAM_ATTR countEncoder4x() {
  int levelA = digitalRead(ENCODER_A_PIN);
  int levelB = digitalRead(ENCODER_B_PIN);
  int currentState = levelA * 2 + levelB;

  bool forward =
    (lastEncoderState == 0 && currentState == 1) ||
    (lastEncoderState == 1 && currentState == 3) ||
    (lastEncoderState == 3 && currentState == 2) ||
    (lastEncoderState == 2 && currentState == 0);

  bool reverse =
    (lastEncoderState == 0 && currentState == 2) ||
    (lastEncoderState == 2 && currentState == 3) ||
    (lastEncoderState == 3 && currentState == 1) ||
    (lastEncoderState == 1 && currentState == 0);

  if (forward) {
    encoderCount = encoderCount + 1;
  } else if (reverse) {
    encoderCount = encoderCount - 1;
  }

  lastEncoderState = currentState;
}

void setup() {
  Serial.begin(115200);

  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);

  lastEncoderState = digitalRead(ENCODER_A_PIN) * 2
                   + digitalRead(ENCODER_B_PIN);

  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), countEncoder4x, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_B_PIN), countEncoder4x, CHANGE);

  Serial.println("这是 4 倍频扩展实验");
}

void loop() {
  noInterrupts();
  long countCopy = encoderCount;
  interrupts();

  Serial.print("4X Count:");
  Serial.println(countCopy);
  delay(200);
}
