/*
  程序功能：使用 DMP 解算 MPU6050 姿态，并把姿态、红外、距离和 RGB 状态显示在 OLED。
  使用方法：按课程引脚连接全部模块；上电校准时保持 MPU6050 静止，之后移动和遮挡传感器。
  运行结果：OLED 同时显示 Y/P/R、红外状态、超声波距离和 RGB 颜色，彩灯随距离变色。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

// Wire、I2Cdev、MotionApps20 负责 I2C 与姿态解算；
// GFX、SSD1306 负责 OLED；NeoPixel 负责 WS2812。
#include <Wire.h>
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>

// 集中定义全部引脚，联调时可快速对照接线表排查。
#define PIN_SDA 8
#define PIN_SCL 9
#define PIN_IR 40
#define PIN_TRIG 12
#define PIN_ECHO 13
#define PIN_RGB 48

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C
#define OLED_RESET -1

// 三个对象分别代表 OLED、单颗 WS2812 和 MPU6050。
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_NeoPixel rgb(1, PIN_RGB, NEO_GRB + NEO_KHZ800);
MPU6050 mpu;

// DMP 就绪标志可阻止程序在初始化完成前读取 FIFO。
bool dmpReady = false;
// FIFO 数据包经过四元数和重力向量计算，最终得到 ypr 姿态角。
uint8_t fifoBuffer[64];
Quaternion quaternion;
VectorFloat gravity;
float ypr[3];

// 用角度制全局变量保存结果，显示函数可以直接读取。
float yaw = 0;
float pitch = 0;
float roll = 0;

void setup() {
  Serial.begin(115200);

  // 先配置普通 GPIO：红外为输入，TRIG 为输出，ECHO 为输入。
  pinMode(PIN_IR, INPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  // OLED 与 MPU6050 共用 GPIO 8/9，但依靠 0x3C、0x68 两个地址区分。
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);

  // 先初始化 OLED，后续 MPU6050 错误才能直接显示在屏幕上。
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED 初始化失败");

    while (true) {
      delay(1000);
    }
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("DMP Calibrating...");
  // print() 只写缓冲区，display() 才把提示送到屏幕。
  display.display();

  // initialize() 配置 MPU6050，testConnection() 验证 I2C 通信。
  mpu.initialize();

  if (!mpu.testConnection()) {
    stopProgram("MPU6050 CONNECT ERROR");
  }

  // 返回值为 0 表示 DMP 初始化成功，其他值进入错误处理。
  byte dmpStatus = mpu.dmpInitialize();

  if (dmpStatus != 0) {
    stopProgram("DMP INIT ERROR");
  }

  // 校准期间模块必须静止、水平，否则姿态零点会产生明显偏差。
  mpu.setXGyroOffset(0);
  mpu.setYGyroOffset(0);
  mpu.setZGyroOffset(0);
  mpu.setXAccelOffset(0);
  mpu.setYAccelOffset(0);
  mpu.setZAccelOffset(0);
  mpu.CalibrateAccel(6);
  mpu.CalibrateGyro(6);

  // 校准完成后开启 DMP，并允许主循环读取 FIFO。
  mpu.setDMPEnabled(true);
  dmpReady = true;

  // 彩灯初始化后先关闭，避免上电保留未知颜色。
  rgb.begin();
  rgb.clear();
  rgb.show();
}

void loop() {
  // 本轮没有完整 DMP 数据包时不更新其他输出，避免姿态数据失真。
  if (readDmpAngles() == false) {
    delay(10);
    return;
  }

  // 先集中采集全部输入，再统一判断和输出，程序结构更清楚。
  bool irDetected = digitalRead(PIN_IR) == LOW;
  float distance = readDistance();
  // colorName 保存与实际灯色对应的屏幕文字。
  String colorName;

  // 无回波关闭；近距离红色；中距离黄色；远距离绿色。
  if (distance == 0) {
    rgb.setPixelColor(0, rgb.Color(0, 0, 0));
    colorName = "OFF";
  } else if (distance <= 20) {
    rgb.setPixelColor(0, rgb.Color(30, 0, 0));
    colorName = "RED";
  } else if (distance <= 50) {
    rgb.setPixelColor(0, rgb.Color(30, 20, 0));
    colorName = "YELLOW";
  } else {
    rgb.setPixelColor(0, rgb.Color(0, 30, 0));
    colorName = "GREEN";
  }

  // 所有颜色分支只设置数据，执行 show() 后灯珠才真正改变。
  rgb.show();

  // OLED 空间有限，使用字号 1 和简短标签集中显示四类信息。
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);

  display.print("Y:");
  display.print(yaw, 0);
  display.print(" P:");
  display.print(pitch, 0);
  display.print(" R:");
  display.println(roll, 0);

  display.print("IR:");

  // 把红外模块的有效电平翻译为 YES/NO。
  if (irDetected) {
    display.println("YES");
  } else {
    display.println("NO");
  }

  display.print("Distance:");

  // 距离 0 代表 pulseIn() 超时，而不是障碍物紧贴传感器。
  if (distance == 0) {
    display.println("NONE");
  } else {
    display.print(distance, 1);
    display.println("cm");
  }

  display.print("RGB:");
  display.println(colorName);

  // 完成整帧内容后统一提交，减少各行不同步的视觉现象。
  display.display();
  delay(100);
}

// 从 DMP 的 FIFO 中读取一组姿态角
bool readDmpAngles() {
  // 用返回值告诉 loop() 本轮姿态角是否有效。
  if (dmpReady == false) {
    return false;
  }

  // FIFO 没有完整数据包时直接返回，不使用旧数据冒充新数据。
  if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer) == false) {
    return false;
  }

  // DMP 解算顺序：数据包 → 四元数 → 重力向量 → Y/P/R。
  mpu.dmpGetQuaternion(&quaternion, fifoBuffer);
  mpu.dmpGetGravity(&gravity, &quaternion);
  mpu.dmpGetYawPitchRoll(ypr, &quaternion, &gravity);

  // 将库返回的弧度转换成便于课堂观察的角度。
  yaw = ypr[0] * 180.0 / PI;
  pitch = ypr[1] * 180.0 / PI;
  roll = ypr[2] * 180.0 / PI;

  return true;
}

float readDistance() {
  // 发送 10 微秒触发脉冲，开始一次超声波测量。
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  // 最多等待 30 ms；超时返回 0，防止程序长期停在测距处。
  unsigned long echoTime = pulseIn(PIN_ECHO, HIGH, 30000);
  // 声速为 0.0343 cm/us，除以 2 得到单程距离。
  float distance = echoTime * 0.0343 / 2.0;

  return distance;
}

// 显示错误信息并停止程序
void stopProgram(String message) {
  // 同时输出到串口和 OLED，方便从两种途径获取故障信息。
  Serial.println(message);

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(message);
  display.display();

  // 初始化失败后停止综合流程，等待学生按接线清单排查。
  while (true) {
    delay(1000);
  }
}
