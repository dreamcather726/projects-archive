/*
  程序功能：演示 Arduino 的 setup()、loop() 和串口输出等基础语法。
  使用方法：上传程序后，打开波特率为 115200 的串口监视器，无需发送数据。
  运行结果：串口每隔 3 秒显示变量、if 判断、while 循环和 for 循环的示例结果。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

// setup 只在开发板启动时运行一次
void setup() {
  // 启动串口，115200 是通信速度
  Serial.begin(115200);
}

// loop 会一直重复运行
void loop() {
  // int 用来保存整数
  int age = 12;

  // float 用来保存小数
  float voltage = 3.3;

  // char 用来保存一个字符，字符使用单引号
  char grade = 'A';

  // bool 只有 true 和 false 两种值
  bool ready = true;

  // println 输出内容后会自动换行
  Serial.println("ESP32-S3 start");
  Serial.println(age);
  Serial.println(voltage);
  Serial.println(grade);

  // if 判断条件是否成立
  if (ready == true) {
    Serial.println("Ready!");
  }

  // while 会在条件成立时重复运行
  int count = 0;
  while (count < 3) {
    Serial.println("while");
    count = count + 1;
  }

  // for 把循环需要的三个部分写在同一行
  // 初始值；循环条件；每次循环后的变化
  for (int i = 0; i < 3; i = i + 1) {
    Serial.println("for");
  }

  // 等待 3 秒，避免串口输出得太快
  delay(3000);
}
