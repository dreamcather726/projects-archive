/*
  程序功能：读取按键状态，每按一次按键切换一次 LED 的亮灭状态。
  使用方法：LED 连接 GPIO 4，按键连接 GPIO 5 和 GND，上传后按下按键测试。
  运行结果：每次有效按键后，LED 的亮灭状态发生一次切换。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

// 普通 LED 连接 GPIO 4
const int LED_PIN = 4;

// 按键连接 GPIO 5 和 GND
const int BUTTON_PIN = 5;

// 保存 LED 当前的状态，开始时为关闭
bool ledState = false;

// 保存按键上一次的状态，开始时为松开
int lastButton = HIGH;

void setup() {
  // LED 引脚负责输出电平
  pinMode(LED_PIN, OUTPUT);

  // 开启内部上拉电阻，不需要外接上拉电阻
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // 程序启动时关闭 LED
  digitalWrite(LED_PIN, LOW);
}

void loop() {
  // 调用按键读取函数
  
   digitalWrite(LED_PIN, button_read());

}
int button_read(){
  // 读取按键的当前状态
  int buttonState = digitalRead(BUTTON_PIN);
  delay(50); // 延时 50 毫秒，消抖
  // 检测按键是否从松开变为按下
  if (lastButton == HIGH && buttonState == LOW) {
    // 切换 LED 的状态
    ledState = !ledState;
   
  }

  // 更新上一次的按键状态
  lastButton = buttonState;

  // 返回切换后的 LED 状态
  return ledState;
}
