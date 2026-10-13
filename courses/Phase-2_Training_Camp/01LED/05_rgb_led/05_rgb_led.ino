/*
  程序功能：分别控制 RGB 灯的红、绿、蓝通道，显示不同颜色。
  使用方法：公共阴极 RGB 灯的 R、G、B 分别连接 GPIO 6、7、8，公共端接 GND。
  运行结果：RGB 灯依次显示红、绿、蓝、白，然后熄灭，每种状态保持 1 秒。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

// RGB 灯的红、绿、蓝三个引脚
const int RED_PIN = 6;
const int GREEN_PIN = 7;
const int BLUE_PIN = 8;

void setup() {
  // 三个颜色引脚都设置为输出模式
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
}

void loop() {
  // 红色最大，绿色和蓝色关闭
  setColor(255, 0, 0);
  delay(1000);

  // 显示绿色
  setColor(0, 255, 0);
  delay(1000);

  // 显示蓝色
  setColor(0, 0, 255);
  delay(1000);

  // 三种颜色同时最亮，混合成白色
  setColor(255, 255, 255);
  delay(1000);

  // 三种颜色全部关闭
  setColor(0, 0, 0);
  delay(1000);
}

// 自定义函数：同时设置红、绿、蓝的亮度
// 本例适用于公共阴极 RGB 灯
void setColor(int red, int green, int blue) {
  analogWrite(RED_PIN, red);
  analogWrite(GREEN_PIN, green);
  analogWrite(BLUE_PIN, blue);
}
