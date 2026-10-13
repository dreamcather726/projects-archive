   程序功能：读取力敏传感器数值，压力超过设定值时启动有源蜂鸣器报警。
  使用方法：力敏输出接 GPIO 15、蜂鸣器接 GPIO 21，打开 115200 串口后按压力敏片。
  运行结果：串口显示压力值；数值超过 1200 时蜂鸣器鸣叫，松开后停止。

```c++

// 力敏传感器的分压输出连接 GPIO 15
#define PIN_FORCE 15

// 有源蜂鸣器模块的控制端连接 GPIO 21
#define PIN_BUZZER 21

// 压力超过这个数值时报警
// 可以根据串口显示的数值进行修改
#define FORCE_LIMIT 1200

void setup() {
  Serial.begin(115200);
  // 蜂鸣器引脚设置为输出模式
  pinMode(PIN_BUZZER, OUTPUT);
}

void loop() {
  // 读取力敏传感器，范围通常是 0～4095
  int force = analogRead(PIN_FORCE);

  // 在串口监视器中观察压力数值
  Serial.println(force);

  // 压力超过设定值，蜂鸣器响
  if (force > FORCE_LIMIT) {
    digitalWrite(PIN_BUZZER, HIGH);
  } else {
    digitalWrite(PIN_BUZZER, LOW);
  }

  // 每隔 100 毫秒读取一次
  delay(100);
}
```

  程序功能：检测压力大小，超过设定值后启动 LED 呼吸灯效果。
  使用方法：力敏输出接 GPIO 15、LED 接 GPIO 2，上传程序后按压力敏片。
  运行结果：压力值超过 1200 时 LED 产生呼吸效果，压力不足时 LED 熄灭。

```c++
// 力敏传感器连接 GPIO 15
#define PIN_FORCE 15

// 板载 LED_ON 连接 GPIO 2
#define PIN_LED 2

// 超过这个数值时启动呼吸灯
#define FORCE_LIMIT 1200

void setup() {

  // 把 LED 引脚设置为输出模式
  pinMode(PIN_LED, OUTPUT);
}

void loop() {
  int force = analogRead(PIN_FORCE);

  if (force > FORCE_LIMIT) {
    // PWM 从 0 增加到 255，LED 逐渐变亮
    for (int light = 0; light <= 255; light = light + 5) {
      analogWrite(PIN_LED, light);
      delay(20);
    }

    // PWM 从 255 减少到 0，LED 逐渐变暗
    for (int light = 255; light >= 0; light = light - 5) {
      analogWrite(PIN_LED, light);
      delay(20);
    }
  } else {
    // 没有达到设定压力时关闭 LED
    analogWrite(PIN_LED, 0);
  }
}
```

  程序功能：把力敏传感器的压力值转换成 LED 亮度，实现压力越大灯越亮。
  使用方法：力敏输出接 GPIO 1、LED 接 GPIO 2，打开 115200 串口后改变按压力度。
  运行结果：LED 随压力增大而变亮，串口同时显示 force 和 light 数值。

```c++
// 力敏传感器连接 GPIO 1
#define PIN_FORCE 1

// 板载 LED_ON 连接 GPIO 2
#define PIN_LED 2

// 达到这个压力数值时，LED 为最亮
#define FORCE_MAX 3000

void setup() {
  Serial.begin(115200);


  // 把 LED 引脚设置为输出模式
  pinMode(PIN_LED, OUTPUT);
}

void loop() {
  int force = analogRead(PIN_FORCE);

  // 防止压力数值超过设定范围
  force = constrain(force, 0, FORCE_MAX);

  // 把压力转换为 0～255 的亮度
  int light = map(force, 0, FORCE_MAX, 0, 255);

  // 输出 PWM，控制 LED 亮度
  analogWrite(PIN_LED, light);

  Serial.print("force: ");
  Serial.print(force);
  Serial.print("  light: ");
  Serial.println(light);

  delay(50);
}
```

  程序功能：根据力敏传感器测得的压力大小改变 WS2812 彩灯颜色。
  使用方法：力敏输出接 GPIO 15、WS2812 接 GPIO 48，打开 115200 串口后逐渐加压。
  运行结果：无压力时灯灭，轻按显示蓝色，中等压力显示绿色，重按显示红色。

```c++
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
```

