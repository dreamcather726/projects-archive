/*
  程序功能：使用 CHANGE 同时统计编码器 A 相的上升沿和下降沿，实现双边沿2倍频计数。
  使用方法：编码器 A、B 相接 GPIO 6、7，上传后打开 115200 串口并正反转动编码器。
  运行结果：串口显示总计数、上升沿次数和下降沿次数；每个 A 相周期计数2次。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

const int ENCODER_A_PIN = 6;
const int ENCODER_B_PIN = 7;

volatile long encoderCount = 0;
volatile long risingCount = 0;
volatile long fallingCount = 0;

// CHANGE 会在 A 相上升沿和下降沿都执行
void IRAM_ATTR countDualEdge() {
  int levelA = digitalRead(ENCODER_A_PIN);
  int levelB = digitalRead(ENCODER_B_PIN);

  // 进入中断时为 HIGH，说明刚刚出现上升沿
  if (levelA == HIGH) {
    risingCount = risingCount + 1;
  } else {
    fallingCount = fallingCount + 1;
  }

  // 双边沿先判断当前是上升沿还是下降沿，再读取B相方向
  // 上升沿：读取B相，B为高记作正转
  if (levelA == HIGH) {
    if (levelB == HIGH) {
      encoderCount = encoderCount + 1;
    } else {
      encoderCount = encoderCount - 1;
    }
  }

  // 下降沿：同一正转方向下，B相应为低电平
  if (levelA == LOW) {
    if (levelB == LOW) {
      encoderCount = encoderCount + 1;
    } else {
      encoderCount = encoderCount - 1;
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);

  attachInterrupt(
    digitalPinToInterrupt(ENCODER_A_PIN),
    countDualEdge,
    CHANGE
  );
}

void loop() {
  noInterrupts();
  long countCopy = encoderCount;
  long risingCopy = risingCount;
  long fallingCopy = fallingCount;
  interrupts();

  Serial.print("2X Count:");
  Serial.print(countCopy);
  Serial.print("  RISING:");
  Serial.print(risingCopy);
  Serial.print("  FALLING:");
  Serial.println(fallingCopy);

  delay(200);
}
