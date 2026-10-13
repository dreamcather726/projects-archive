/*
  程序功能：使用最基础的 RISING 上升沿中断统计编码器 A 相脉冲，并通过 B 相判断方向。
  使用方法：编码器 A、B 相接 GPIO 6、7，上传后打开 115200 串口并缓慢正反转动编码器。
  运行结果：A 相每出现一次上升沿，Count 增加或减少 1；停止转动后计数保持不变。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

const int ENCODER_A_PIN = 6;
const int ENCODER_B_PIN = 7;

volatile long encoderCount = 0;

// 中断函数没有参数，也没有返回值
void IRAM_ATTR encoderRising() {
  // A 相上升沿到来时，读取 B 相判断方向
  if (digitalRead(ENCODER_B_PIN) == HIGH) {
    encoderCount = encoderCount + 1;
  } else {
    encoderCount = encoderCount - 1;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);

  // RISING 表示只在 LOW 变为 HIGH 时进入中断函数
  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN),encoderRising,RISING);
}

void loop() {
  // 复制共享变量时暂时关闭中断，复制完成后立即恢复
  noInterrupts();
  long countCopy = encoderCount;
  interrupts();

  Serial.print("RISING Count:");
  Serial.println(countCopy);
  delay(200);
}
