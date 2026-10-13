/*
  程序功能：使用 FALLING 下降沿中断统计编码器 A 相脉冲，并通过 B 相判断方向。
  使用方法：编码器 A、B 相接 GPIO 6、7，上传后打开 115200 串口并缓慢正反转动编码器。
  运行结果：A 相每出现一次下降沿，Count 增加或减少 1；停止转动后计数保持不变。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

const int ENCODER_A_PIN = 6;
const int ENCODER_B_PIN = 7;

volatile long encoderCount = 0;

// FALLING 只在 A 相从 HIGH 变为 LOW 时执行
void IRAM_ATTR encoderFalling() {
  // A 相下降沿到来时，读取 B 相判断方向
  if (digitalRead(ENCODER_B_PIN) == LOW) {
    encoderCount = encoderCount + 1;
  } else {
    encoderCount = encoderCount - 1;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);

  attachInterrupt(
    digitalPinToInterrupt(ENCODER_A_PIN),
    encoderFalling,
    FALLING
  );
}

void loop() {
  noInterrupts();
  long countCopy = encoderCount;
  interrupts();

  Serial.print("FALLING Count:");
  Serial.println(countCopy);
  delay(200);
}
