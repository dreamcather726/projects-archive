#define PIN_BASE 19
// 分配 LEDC 通道
#define CHANNEL_BASE 0


void setup() {
  Serial.begin(115200);

  // 设置引脚、频率、精度和通道
  ledcAttachChannel(PIN_BASE, 50, 14, CHANNEL_BASE);

  // 三个舵机转到 90 度
  ledcWriteChannel(CHANNEL_BASE, 1225);


  Serial.println("输入：b90、a90 或 g90");
}

void loop() {
  if (Serial.available() == 0) return;

  String command = Serial.readStringUntil('\n');
  command.trim();

  if (command.length() < 2) return;

  char name = command.charAt(0);
  int angle = command.substring(1).toInt();

  angle = constrain(angle, 0, 180);
  int duty = map(angle, 0, 180, 410, 2048);

  if (name == 'b') ledcWriteChannel(CHANNEL_BASE, duty);
  if (name == 'a') ledcWriteChannel(CHANNEL_ARM, duty);
  if (name == 'g') ledcWriteChannel(CHANNEL_GRIP, duty);

  Serial.println(command);
}
