// 导入所需库
#include <Arduino.h>
#include <SoftwareSerial.h>
#include <DHT.h>
#include <U8x8lib.h>  // OLED显示库
#include <Wire.h>     // I2C通信库（OLED依赖）
#include <Adafruit_NeoPixel.h> // WS2812B灯带控制库

// OLED初始化 - 适配SSD1306 128x64屏幕，I2C默认地址0x3C
U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(U8X8_PIN_NONE);

// 设备ID
#define device_id 0x01

// 常量定义
#define temp_threshold 30.0 // 温度阈值
char Command[] = {0X55, 0X01, 0X00, 0X01, 0XFF}; // 55 设备号 指令 数据 FF
char ASK[] = {0XAA, 0X01, 0XFF}; // AA 设备号 FF 请求数据

// 引脚定义
#define DHTPIN 2       // DHT11 数据引脚
#define yellow_pump_pin 3 // 黄色按钮引脚(控制水泵)
#define red_fan_pin 4  // 红色按钮引脚(控制风扇)
#define TQ_PIN 5       // 模式切换按钮引脚
#define RGB_PIN 6      // RGB引脚
#define Light_pin A1   // 光敏传感器引脚
#define fan_D1 7       // 风扇控制引脚1
#define fan_D2 8       // 风扇控制引脚2
#define Soild_pin A3   // 土壤湿度传感器
#define LED 13         // LED引脚
#define DHTTYPE DHT11  // DHT11 传感器类型
#define pump_PIN 12    // 水泵控制引脚
#define NUM_PIXELS 7   // RGB灯珠数量
#define LORA_RX 9      // Lora接收引脚
#define LORA_TX 10     // Lora发送引脚
DHT dht(DHTPIN, DHTTYPE);
// 初始化WS2812B灯带
Adafruit_NeoPixel pixels(NUM_PIXELS, RGB_PIN, NEO_GRB + NEO_KHZ800);
SoftwareSerial Lora(LORA_RX, LORA_TX);
//—————————————常量定义———————————————————————/
const long int Lora_BaudRate = 115200;//Lora模块波特率
float hum = 0;
// 删除自动模式相关的目标湿度常量
float temp = 0;
float Light = 0;
float soild_h = 0;
int TQ = 0;//天气状态
// 删除自动模式相关的目标土壤湿度常量
int yellow_pump_state = LOW;//黄色水泵状态(默认关闭)
int red_fan_state = LOW;//红色风扇状态(默认关闭)
bool pump_is_on = false;
bool fan_is_on = false;
void setup() {
  Serial.begin(Lora_BaudRate);//初始化串口通信
  Lora.begin(Lora_BaudRate);//初始化Lora模块
  dht.begin();//初始化DHT11传感器
  // 设置按钮引脚为输入模式并启用内部上拉电阻
  pinMode(yellow_pump_pin, INPUT_PULLUP);
  pinMode(red_fan_pin, INPUT_PULLUP);
  // 设置其他引脚
  pinMode(Light_pin, INPUT);//设置光敏传感器引脚为输入模式
  pinMode(fan_D1, OUTPUT);//设置风扇D1引脚为输出模式
  pinMode(fan_D2, OUTPUT);//设置风扇D2引脚为输出模式
  pinMode(Soild_pin, INPUT);//设置土壤湿度传感器引脚为输入模式
  pinMode(pump_PIN, OUTPUT);//设置黄色水泵引脚为输出模式
  pinMode(LED, OUTPUT);//设置LED引脚为输出模式
  pinMode(TQ_PIN, INPUT);//天气输入模式
  
  // 初始化WS2812B灯带
  pixels.begin();
  pixels.setBrightness(50); // 设置亮度(0-255)
  
  // 初始化输出状态
  digitalWrite(LED, LOW);//初始化LED引脚为低电平(熄灭)
  digitalWrite(fan_D1, LOW);
  digitalWrite(fan_D2, LOW);
  digitalWrite(pump_PIN, LOW); // 默认关闭水泵（修正为LOW关闭）

  // 初始化OLED
  if (!u8x8.begin()) {
    Serial.println("ERROR: OLED初始化失败！");
    while (1);  // 初始化失败则停止
  }
  u8x8.setContrast(200);  // 调整OLED亮度（0-255）
  u8x8.setFont(u8x8_font_chroma48medium8_r);  // 设置显示字体
  u8x8.clearDisplay();  // 清空屏幕
  Serial.println("初始化成功！");
}

// 在loop函数中添加呼吸灯效果
void loop() {
   if(Lora.available()){//如果Lora模块接收到数据
    // 读取接收到的字节
    char receivedByte = Lora.read();
    if(receivedByte == ASK[0]){
      byte temp[2];
      Lora.readBytes(temp,2);
      if(temp[0]==device_id){
        if(temp[1]==0XFF){
          sendJSONData();
        }
      }
    }//接收问询，且发送数据    
   else if(receivedByte == Command[0]){          
      byte temp[4];
      Lora.readBytes(temp,4);
      byte allbytes[]={receivedByte,temp[0],temp[1],temp[2],temp[3],Command[4]};
      if(allbytes[1]==device_id){        
        if(allbytes[5]==0XFF){
          //解析数据
          byte command_type = allbytes[2];
          byte command_data = allbytes[3];
          executeCommand(command_type, command_data);          
        }
      }
    } 
    }
    // 读取环境数据
   readEnv();
   processButtonControls(); //始终执行手动控制 
   // 调用非阻塞式显示函数
   displayEnvData();   
   // 调用RGB灯效果控制函数
   updateRGBEffects();
}



// 更新RGB灯效果的函数
void updateRGBEffects() {
  // 添加RGB灯效果
  static unsigned long lastLightUpdate = 0;
  static int lightMode = 0;
  static int currentPixel = 0;
  static int colorIndex = 0;
  
  // 每50毫秒更新一次灯光效果
  if (millis() - lastLightUpdate > 50) {
    lastLightUpdate = millis();
    
    // 根据系统状态切换不同的灯光效果
    if (pump_is_on && fan_is_on) {
      // 水泵和风扇都开启 - 彩虹流动效果
      rainbowFlow();
    } else if (pump_is_on) {
      // 只有水泵开启 - 蓝色效果
      setAllPixels(0, 0, 255);
    } else if (fan_is_on) {
      // 只有风扇开启 - 红色效果
      setAllPixels(255, 0, 0);
    } else {
      // 都关闭 - 绿色呼吸效果
      setAllPixels(0, 255, 0);
    }
  }
}
// 设置所有像素为同一颜色
void setAllPixels(uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < NUM_PIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(r, g, b));
  }
  pixels.show();
}

// 彩虹流动效果
void rainbowFlow() {
  static int rainbowIndex = 0;
  for (int i = 0; i < NUM_PIXELS; i++) {
    int pixelIndex = (i * 256 / NUM_PIXELS + rainbowIndex) & 255;
    pixels.setPixelColor(i, Wheel(pixelIndex));
  }
  pixels.show();
  rainbowIndex = (rainbowIndex + 1) % 256;
}

// 颜色渐变函数
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

// 呼吸灯效果
void breatheEffect(uint8_t r, uint8_t g, uint8_t b) {
  static int brightness = 0;
  static int fadeAmount = 2;
  
  // 更新亮度
  brightness += fadeAmount;
  
  // 反转亮度变化方向
  if (brightness <= 0 || brightness >= 255) {
    fadeAmount = -fadeAmount;
  }
  
  // 应用亮度到所有像素
  for (int i = 0; i < NUM_PIXELS; i++) {
    // 根据亮度比例调整RGB值
    uint8_t r_adjusted = map(brightness, 0, 255, 0, r);
    uint8_t g_adjusted = map(brightness, 0, 255, 0, g);
    uint8_t b_adjusted = map(brightness, 0, 255, 0, b);
    pixels.setPixelColor(i, pixels.Color(r_adjusted, g_adjusted, b_adjusted));
  }
  pixels.show();
}
// 读取环境数据
void readEnv() {
  hum = dht.readHumidity();
  temp = dht.readTemperature();
  Light = analogRead(Light_pin);//读取光敏传感器值
  Light =100- map(Light, 50, 700, 0, 100);//将光敏传感器值映射到0-100
  soild_h = analogRead(Soild_pin);//读取土壤湿度传感器值 
  // soild_h = map(soild_h, 0, 1023, 0, 100);//将土壤湿度传感器值映射到0-100
  TQ = digitalRead(TQ_PIN);//读取天气传感器值
  if(temp > temp_threshold) {
    digitalWrite(LED, HIGH);//打开LED
  }else{
    digitalWrite(LED, LOW);//关闭LED
  }
  delay(200);
}
// 控制风扇
void controlFan(int state) {
  if(state == 1) {
    digitalWrite(fan_D1, LOW);//打开风扇D1
    digitalWrite(fan_D2, HIGH);//关闭风扇D2
  }
  else if(state == 0) {
    digitalWrite(fan_D1, LOW);//关闭风扇D1
    digitalWrite(fan_D2, LOW);//打开风扇D2
  }
}
// 控制水泵
void controlpump(int state) {
  if(state == 1) {
    digitalWrite(pump_PIN, HIGH);//打开黄色水泵
    
  }
  else if(state == 0) {
    digitalWrite(pump_PIN, LOW);//关闭黄色水泵
    
  }
}
// 处理按钮控制
void processButtonControls() {
  // 存储按钮的前一次状态
  static int last_yellow_pump_state = HIGH;
  static int last_red_fan_state = HIGH;
  
  // 读取当前按钮状态
  int current_yellow_pump_state = digitalRead(yellow_pump_pin);
  int current_red_fan_state = digitalRead(red_fan_pin);
  
  // 检测黄色按钮的下降沿（从HIGH变为LOW，表示按下）
  if (last_yellow_pump_state == HIGH && current_yellow_pump_state == LOW) {
    // 切换水泵状态
    pump_is_on = !pump_is_on;
    controlpump(pump_is_on ? 1 : 0);
    // 添加短暂延迟以消除抖动
    delay(50);
  }
  
  // 检测红色按钮的下降沿（从HIGH变为LOW，表示按下）
  if (last_red_fan_state == HIGH && current_red_fan_state == LOW) {
    // 切换风扇状态
    fan_is_on = !fan_is_on;
    controlFan(fan_is_on ? 1 : 0);  
    // 添加短暂延迟以消除抖动
    delay(50);
  }
  
  // 更新按钮的前一次状态
  last_yellow_pump_state = current_yellow_pump_state;
  last_red_fan_state = current_red_fan_state;
}

void displayEnvData() {
  // 非阻塞式更新 - 使用millis()记录上次更新时间
  static unsigned long lastUpdateTime = 0;
  const unsigned long updateInterval = 1000; // 更新间隔1000毫秒(1秒)
  
  // 检查是否到了更新时间
  if (millis() - lastUpdateTime >= updateInterval) {
    // 更新上次更新时间
    lastUpdateTime = millis();
    
    // 使用sprintf格式化环境数据并显示在OLED上
    char buffer[20];
    
    // 显示温度
    sprintf(buffer, "Tem:%d`C", (int)temp);
    u8x8.drawString(0, 0, buffer);
   
    // 显示湿度
    sprintf(buffer, "Hum:%d%%", (int)hum);
    u8x8.drawString(9, 0, buffer);
   
    // 显示光照强度
    sprintf(buffer, "Li:%d", (int)Light);
    u8x8.drawString(0, 2, buffer);
   
    // 显示土壤湿度
    sprintf(buffer, "So:%d", (int)soild_h);
    u8x8.drawString(9, 2, buffer);
   
    // 显示天气状态
    u8x8.drawString(0, 4, TQ ? "weather:Sunny" : "weather:Cloudy");
    
    // 显示设备状态
    u8x8.drawString(0, 5, pump_is_on ? "Pump: ON" : "Pump:OFF");
    
    // 显示风扇状态
    u8x8.drawString(0, 6, fan_is_on ? "Fan: ON" : "Fan:OFF");
    
  }
}

// 发送JSON格式数据的函数
// 格式：{"id":1,"t":25.5,"h":60.0,"l":50,"w":1,"s":400,"p":1,"f":0}
//t:温度,h:湿度,l:光照强度,w:天气状态,s:土壤湿度,p:水泵状态,f:风扇状态
void sendJSONData() {
  String jsonString = "{\"id\":" + String(device_id);
  jsonString += ",\"t\":" + String(temp, 1);
  jsonString += ",\"h\":" + String(hum, 1);
  jsonString += ",\"l\":" + String(Light, 1);
  jsonString += ",\"w\":" + String(TQ ? 1 : 0);
  jsonString += ",\"s\":" + String(soild_h, 1);
  jsonString += ",\"p\":" + String(pump_is_on ? 1 : 0);
  jsonString += ",\"f\":" + String(fan_is_on ? 1 : 0);
  jsonString += "}";
 
  Lora.println(jsonString);
}
void executeCommand(byte command_type, byte command_data) {
  switch (command_type) {
    case 0x01: // 控制水泵
      if (command_data == 0x01) {
        controlpump(1);
        pump_is_on = true;
      } else if (command_data == 0x00) {
        controlpump(0);
        pump_is_on = false;
      }
      break;
    case 0x02: // 控制红色风扇
      if (command_data == 0x01) {
        controlFan(1);
        fan_is_on = true;
      } else if (command_data == 0x00) {
        controlFan(0);
        fan_is_on = false;
      }
      break;
    case 0xFF: // 控制全部设备
      if (command_data == 0x01) {
        controlpump(1);
        controlFan(1);
        pump_is_on = true;
        fan_is_on = true;
      } else if (command_data == 0x00) {
        controlpump(0);
        controlFan(0);
        pump_is_on = false;
        fan_is_on = false;
      }
      break;
    default:
      Serial.println("未知指令类型");
      break;
  }
}