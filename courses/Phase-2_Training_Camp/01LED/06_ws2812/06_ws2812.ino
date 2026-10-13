/*
  程序功能：使用 Adafruit_NeoPixel 库控制 WS2812 彩灯显示不同颜色。
  使用方法：WS2812 数据端连接 GPIO 48，上传程序后直接观察彩灯。
  运行结果：彩灯依次显示红、绿、蓝，然后熄灭，每种状态保持 1 秒。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

// 导入 WS2812 控制库
#include <Adafruit_NeoPixel.h>

// 灯带的数据输入端连接 GPIO 9
const int LED_PIN = 48;

// 灯带上共有 8 颗灯珠
const int LED_COUNT = 1;

// 创建灯带对象
// NEO_GRB 表示颜色数据顺序，NEO_KHZ800 表示通信速度
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  // 启动灯带
  strip.begin();

  // 清除内存中的颜色
  strip.clear();

  // show 才会把颜色数据真正发送给灯带
  strip.show();
}

void loop() {
  // fill 设置所有灯珠的颜色
  strip.fill(strip.Color(255, 0, 0));
  strip.show();
  delay(1000);

  // 所有灯珠显示绿色
  strip.fill(strip.Color(0, 255, 0));
  strip.show();
  delay(1000);

  // 所有灯珠显示蓝色
  strip.fill(strip.Color(0, 0, 255));
  strip.show();
  delay(1000);

  // 清除所有颜色，关闭灯带
  strip.clear();
  strip.show();
  delay(1000);
}
