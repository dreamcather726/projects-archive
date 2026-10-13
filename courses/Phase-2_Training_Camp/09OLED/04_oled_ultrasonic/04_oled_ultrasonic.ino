/*
  程序功能：使用超声波模块测量距离，并实时显示在 OLED 屏幕上。
  使用方法：OLED 接 GPIO 8/9，TRIG 接 GPIO 12、ECHO 经分压接 GPIO 13，然后移动障碍物。
  运行结果：OLED 实时显示距离和 cm 单位；无有效回波时显示 NO ECHO。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

// Wire 负责 OLED 的 I2C 通信，GFX 和 SSD1306 负责绘制画面。
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED 与超声波模块使用不同引脚，所有模块仍需共地。
#define PIN_SDA 8
#define PIN_SCL 9
#define PIN_TRIG 12
#define PIN_ECHO 13

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C
#define OLED_RESET -1

// 创建 OLED 对象，屏幕尺寸、I2C 对象和复位方式在此确定。
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);

  // TRIG 由控制器发出触发脉冲，ECHO 由控制器测量返回脉冲。
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  // 上电后先保持 TRIG 为低电平，避免产生意外触发。
  digitalWrite(PIN_TRIG, LOW);

  // OLED 使用独立的 I2C 总线初始化流程。
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000);

  // 如果 OLED 初始化失败，就停止测距显示并等待接线排查。
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED 初始化失败");

    while (true) {
      delay(1000);
    }
  }
}

void loop() {
  // 把“触发—计时—换算”封装为函数，主循环只关心最终距离。
  float distance = readDistance();

  // 动态数值长度会改变，每轮先清除缓冲区可避免字符残留。
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("ULTRASONIC");

  display.setTextSize(2);
  display.setCursor(0, 24);

  // pulseIn() 超时会得到 0，因此本程序用距离 0 表示没有有效回波。
  if (distance == 0) {
    display.println("NO ECHO");
  } else {
    display.print(distance, 1);
    display.println(" cm");
  }

  // 完成整帧排版后再统一刷新到 OLED。
  display.display();
  // HC-SR04 不宜过快连续触发，保留 200 ms 观察间隔。
  delay(200);
}

float readDistance() {
  // 先拉低 2 微秒，保证本次触发脉冲有清晰的起点。
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  // 按模块要求发送至少 10 微秒的高电平触发脉冲。
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  // 测量 ECHO 高电平持续时间；30 ms 超时可避免无回波时长期阻塞。
  unsigned long echoTime = pulseIn(PIN_ECHO, HIGH, 30000);
  // 声速约为 0.0343 cm/us；除以 2 是因为声音经历了往返路程。
  float distance = echoTime * 0.0343 / 2.0;

  return distance;
}
