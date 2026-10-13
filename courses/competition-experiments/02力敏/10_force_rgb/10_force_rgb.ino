/*
  程序功能：根据力敏传感器测得的压力大小改变 WS2812 彩灯颜色。
  使用方法：力敏输出接 GPIO 15、WS2812 接 GPIO 48，打开 115200 串口后逐渐加压。
  运行结果：无压力时灯灭，轻按显示蓝色，中等压力显示绿色，重按显示红色。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

// 导入板载 WS2812 彩灯控制库
#include <Adafruit_NeoPixel.h>

// 力敏传感器连接 GPIO 15
#define PIN_FORCE 15

// 板载 WS2812 连接 GPIO 48
#define PIN_RGB 48

// 板上只有一颗 WS2812 灯珠
#define RGB_COUNT 1

Adafruit_NeoPixel rgb(RGB_COUNT, PIN_RGB, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);



  rgb.begin();// 启动彩灯

  // 限制彩灯整体亮度，避免过亮
  rgb.setBrightness(50);
  rgb.clear();
  rgb.show();
}

void loop() {
  int force = analogRead(PIN_FORCE);
  Serial.println(force);

  // 没有明显压力时关闭彩灯
  if (force < 300) {
    rgb.clear();
  }

  // 轻按显示蓝色
  else if (force < 1200) {
    rgb.setPixelColor(0, rgb.Color(0, 0, 255));
  }

  // 中等压力显示绿色
  else if (force < 2400) {
    rgb.setPixelColor(0, rgb.Color(0, 255, 0));
  }

  // 重按显示红色
  else {
    rgb.setPixelColor(0, rgb.Color(255, 0, 0));
  }

  // 把颜色数据发送给 WS2812
  rgb.show();
  delay(50);
}
