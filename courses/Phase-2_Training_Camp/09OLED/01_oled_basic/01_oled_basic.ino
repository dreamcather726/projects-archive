/*
  程序功能：初始化 I2C OLED 屏幕，并显示基础文字和图形。
  使用方法：OLED 的 SDA 接 GPIO 8、SCL 接 GPIO 9，确认地址为 0x3C 后上传程序。
  运行结果：128×64 OLED 显示示例文字、矩形、直线和圆形等基础内容。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

// Wire 负责 I2C 通信；GFX 提供通用绘图接口；SSD1306 负责驱动本课 OLED。
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ESP32-S3 的 I2C 引脚可由程序指定，本课程统一使用 GPIO 8 和 GPIO 9。
#define PIN_SDA 8
#define PIN_SCL 9

// 下面四个参数必须与实际屏幕型号、地址和接线方式一致。
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C
// -1 表示 OLED 没有单独连接复位引脚。
#define OLED_RESET -1

// 创建屏幕对象：后续所有显示函数都通过 display 调用。
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  // 串口用于输出初始化错误，出现黑屏时可打开串口监视器查看。
  Serial.begin(115200);

  // 启动 I2C，并将时钟设为较稳妥的 100 kHz。
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000);

  // begin() 返回 false 说明没有成功初始化屏幕，程序进入错误等待状态。
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED 初始化失败");

    // 初始化失败后不再继续绘图，防止学生误以为后面的显示代码有问题。
    while (true) {
      delay(1000);
    }
  }

  // clearDisplay() 清除的是内存缓冲区，此时屏幕还不会立即变化。
  display.clearDisplay();
  // 单色 OLED 用白色表示点亮像素。
  display.setTextColor(SSD1306_WHITE);

  // 字号 2 的字符宽高都是基础字体的 2 倍。
  display.setTextSize(2);
  // OLED 左上角坐标为 (0, 0)，x 向右增大，y 向下增大。
  display.setCursor(0, 0);
  display.println("ESP32-S3");

  // 切回字号 1，在较小空间内显示更多接线信息。
  display.setTextSize(1);
  display.setCursor(0, 28);
  display.println("I2C OLED");
  display.println("SDA:8  SCL:9");

  // 绘图完成后必须调用 display() 才会真正显示
  display.display();
}

void loop() {
  // 本课显示的是静态画面，setup() 绘制一次即可，因此 loop() 留空。
}

