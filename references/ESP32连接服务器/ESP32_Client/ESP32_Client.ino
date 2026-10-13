/*
 * ESP32-S3 本地大模型对话系统
 * 功能：通过串口接收文本 -> 发送到服务器 -> 显示AI回复
 * 硬件：ESP32-S3 + ST7789 LCD
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_GFX_Library.h>

// ==================== 调试设置 ====================
#define DEBUG_MODE false  // 设为true开启调试输出

// ==================== 颜色定义 (RGB565格式) ====================
#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800
#define GREEN   0x07E0
#define BLUE    0x001F
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define ORANGE  0xFD20

// ==================== WiFi 配置 ====================
#define WIFI_SSID "REDACTED_CONFIGURE_LOCALLY"
#define WIFI_PASSWORD "REDACTED_CONFIGURE_LOCALLY"

// ==================== 服务器配置 ====================
// 启动服务器后，将服务器IP地址填写在这里
// 例如："http://192.168.1.100:5000/chat"
String serverURL = "http://192.168.52.102:5000/chat";

// ==================== LCD引脚配置 ====================
#define LCD_BL   3
#define LCD_DC   46
#define LCD_SCL  9
#define LCD_MOSI 10
#define LCD_RST  11
#define LCD_CS   -1

// ==================== 显示配置 ====================
#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 240
#define TEXT_SIZE     2
#define LINE_HEIGHT   16

// ==================== 创建LCD对象 ====================
Arduino_DataBus *bus = new Arduino_HWSPI(LCD_DC, LCD_CS, LCD_SCL, LCD_MOSI);
Arduino_GFX *gfx = new Arduino_ST7789(bus, LCD_RST, 0 /* rotation */, true /* IPS */, SCREEN_WIDTH, SCREEN_HEIGHT);

// ==================== 全局变量 ====================
String inputBuffer = "";
int scrollOffset = 0;
const int maxScrollLines = 100;
int messageCount = 0;  // 对话计数器

// ==================== 初始化函数 ====================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n========================================");
  Serial.println("ESP32-S3 本地大模型对话系统");
  Serial.println("========================================");
  
  // 初始化LCD
  initLCD();
  
  // 连接WiFi
  connectWiFi();
  
  // 显示就绪信息
  displayWelcome();
  
  Serial.println("\n系统就绪！请输入文本并按回车发送...");
  Serial.println("========================================\n");
}

// ==================== 主循环 ====================
void loop() {
  // 检查串口输入
  if (Serial.available()) {
    char c = Serial.read();
    
    if (c == '\n' || c == '\r') {
      if (inputBuffer.length() > 0) {
        processInput(inputBuffer);
        inputBuffer = "";
      }
    } else {
      inputBuffer += c;
    }
  }
  
  delay(10);
}

// ==================== LCD初始化 ====================
void initLCD() {
  Serial.println("正在初始化LCD...");
  
  // 初始化背光
  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, HIGH);
  
  // 初始化显示屏
  gfx->begin();
  gfx->fillScreen(BLACK);
  gfx->setTextSize(TEXT_SIZE);
  gfx->setTextColor(WHITE);
  gfx->setCursor(0, 0);
  
  Serial.println("LCD初始化完成");
}

// ==================== WiFi连接 ====================
void connectWiFi() {
  Serial.print("正在连接WiFi: ");
  Serial.println(WIFI_SSID);
  
  // LCD显示连接状态
  gfx->fillScreen(BLACK);
  gfx->setCursor(10, 10);
  gfx->setTextColor(CYAN);
  gfx->setTextSize(2);
  gfx->println("WiFi...");
  gfx->setTextSize(1);
  gfx->println(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi连接成功！");
    Serial.print("IP地址: ");
    Serial.println(WiFi.localIP());
    
    // LCD显示成功
    gfx->fillScreen(BLACK);
    gfx->setCursor(10, 60);
    gfx->setTextColor(GREEN);
    gfx->setTextSize(2);
    gfx->println(" Connected!");
    gfx->println();
    gfx->setTextColor(WHITE);
    gfx->setTextSize(1);
    gfx->print(" IP:");
    gfx->println(WiFi.localIP());
    delay(2000);
  } else {
    Serial.println("\nWiFi连接失败！");
    gfx->fillScreen(BLACK);
    gfx->setCursor(10, 60);
    gfx->setTextColor(RED);
    gfx->setTextSize(3);
    gfx->println(" WiFi");
    gfx->println(" Failed!");
    while(1) delay(1000);
  }
}

// ==================== 欢迎界面 ====================
void displayWelcome() {
  gfx->fillScreen(BLACK);
  gfx->setCursor(10, 40);
  gfx->setTextColor(YELLOW);
  gfx->setTextSize(3);
  gfx->println(" AI");
  gfx->println(" Ready!");
  gfx->println();
  gfx->setTextColor(WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(10, 140);
  gfx->println(" Type in Serial");
  gfx->println(" to chat");
  delay(2000);
}

// ==================== 处理用户输入 ====================
void processInput(String userText) {
  messageCount++;  // 增加对话计数
  
  Serial.println("\n========================================");
  Serial.print("对话 #");
  Serial.println(messageCount);
  Serial.println("========================================");
  Serial.print("你: ");
  Serial.println(userText);
  
  // 显示用户输入
  displayMessage("You: " + userText, CYAN);
  
  // 发送到服务器
  Serial.println("\n正在请求AI回复...");
  String aiResponse = sendToServer(userText);
  
  if (aiResponse.length() > 0) {
    Serial.println("\nAI回复:");
    Serial.println("----------------------------------------");
    Serial.println(aiResponse);
    Serial.println("----------------------------------------");
    
    // 显示AI回复
    displayMessage("AI: " + aiResponse, GREEN);
  } else {
    Serial.println("\n❌ 错误: 未收到服务器响应");
    displayMessage("Error: No response", RED);
  }
  
  Serial.println("\n========================================\n");
}

// ==================== 发送HTTP请求 ====================
String sendToServer(String message) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi未连接！");
    return "";
  }
  
  HTTPClient http;
  http.begin(serverURL);
  http.addHeader("Content-Type", "application/json");
  
  // 构建JSON请求
  String jsonPayload = "{\"message\":\"" + message + "\"}";
  
  int httpResponseCode = http.POST(jsonPayload);
  
  String response = "";
  if (httpResponseCode > 0) {
    #if DEBUG_MODE
    Serial.print("HTTP响应码: ");
    Serial.println(httpResponseCode);
    #endif
    
    if (httpResponseCode == 200) {
      String rawResponse = http.getString();
      
      // 调试：打印原始响应（前200字符）
      #if DEBUG_MODE
      Serial.println("\n--- 原始响应 ---");
      Serial.println(rawResponse.substring(0, min(200, (int)rawResponse.length())));
      Serial.println("--- 响应结束 ---\n");
      #endif
      
      // 解析JSON响应（改进版 - 支持空格）
      int start = rawResponse.indexOf("\"response\"");
      if (start != -1) {
        start += 10; // 跳过 "response"
        
        // 跳过冒号和可能的空格
        while (start < rawResponse.length() && 
               (rawResponse.charAt(start) == ':' || 
                rawResponse.charAt(start) == ' ')) {
          start++;
        }
        
        // 跳过开始的引号
        if (start < rawResponse.length() && rawResponse.charAt(start) == '"') {
          start++;
        }
        
        // 查找response字段的结束引号（需要跳过转义的引号）
        int end = start;
        bool escaped = false;
        while (end < rawResponse.length()) {
          char c = rawResponse.charAt(end);
          if (escaped) {
            escaped = false;
          } else if (c == '\\') {
            escaped = true;
          } else if (c == '"') {
            break; // 找到未转义的引号
          }
          end++;
        }
        
        if (end > start) {
          response = rawResponse.substring(start, end);
          // 处理转义字符
          response.replace("\\n", "\n");
          response.replace("\\\"", "\"");
          response.replace("\\\\", "\\");
          
          #if DEBUG_MODE
          Serial.print("解析结果长度: ");
          Serial.println(response.length());
          Serial.print("前50字符: ");
          Serial.println(response.substring(0, min(50, (int)response.length())));
          #endif
        } else {
          #if DEBUG_MODE
          Serial.println("警告: 未能解析response字段");
          #endif
        }
      } else {
        #if DEBUG_MODE
        Serial.println("警告: 未找到\"response\":\"字段");
        #endif
      }
    }
  } else {
    Serial.print("请求失败，错误码: ");
    Serial.println(httpResponseCode);
  }
  
  http.end();
  return response;
}

// ==================== LCD显示消息 ====================
void displayMessage(String message, uint16_t color) {
  gfx->fillScreen(BLACK);
  
  // 检测是否包含中文（非ASCII字符）
  bool hasChinese = false;
  for (int i = 0; i < message.length(); i++) {
    if ((unsigned char)message[i] > 127) {
      hasChinese = true;
      break;
    }
  }
  
  if (hasChinese) {
    // ========== 中文对话显示（图形化界面）==========
    if (message.startsWith("You:")) {
      // 顶部标题栏
      gfx->fillRect(0, 0, SCREEN_WIDTH, 30, CYAN);
      gfx->setTextColor(BLACK);
      gfx->setTextSize(2);
      gfx->setCursor(60, 8);
      gfx->println("USER");
      
      // 消息计数
      gfx->setTextSize(1);
      gfx->setCursor(10, 40);
      gfx->setTextColor(WHITE);
      gfx->print("Message #");
      gfx->println(messageCount);
      
      // 状态图标（简易）
      gfx->setCursor(20, 70);
      gfx->setTextSize(4);
      gfx->setTextColor(CYAN);
      gfx->println(">>>");
      
      // 提示信息
      gfx->setTextSize(1);
      gfx->setTextColor(WHITE);
      gfx->setCursor(10, 140);
      gfx->println("Sending to server...");
      gfx->setCursor(10, 160);
      gfx->println("Check Serial Monitor");
      gfx->setCursor(10, 180);
      gfx->println("for full content");
      
    } else if (message.startsWith("AI:")) {
      // 顶部标题栏
      gfx->fillRect(0, 0, SCREEN_WIDTH, 30, GREEN);
      gfx->setTextColor(BLACK);
      gfx->setTextSize(2);
      gfx->setCursor(80, 8);
      gfx->println("AI");
      
      // 消息计数
      gfx->setTextSize(1);
      gfx->setCursor(10, 40);
      gfx->setTextColor(WHITE);
      gfx->print("Reply #");
      gfx->println(messageCount);
      
      // 状态图标（简易）
      gfx->setCursor(20, 70);
      gfx->setTextSize(4);
      gfx->setTextColor(GREEN);
      gfx->println("<<<");
      
      // 提示信息
      gfx->setTextSize(1);
      gfx->setTextColor(WHITE);
      gfx->setCursor(10, 140);
      gfx->println("Reply received!");
      gfx->setCursor(10, 160);
      gfx->println("Check Serial Monitor");
      gfx->setCursor(10, 180);
      gfx->println("for full reply");
      
    } else if (message.startsWith("Error")) {
      // 错误显示
      gfx->fillRect(0, 0, SCREEN_WIDTH, 30, RED);
      gfx->setTextColor(WHITE);
      gfx->setTextSize(2);
      gfx->setCursor(50, 8);
      gfx->println("ERROR");
      
      gfx->setCursor(50, 100);
      gfx->setTextSize(3);
      gfx->setTextColor(RED);
      gfx->println("X");
      
      gfx->setTextSize(1);
      gfx->setTextColor(WHITE);
      gfx->setCursor(10, 160);
      gfx->println("Connection failed");
      gfx->setCursor(10, 180);
      gfx->println("Check server status");
    }
    
  } else {
    // ========== 纯英文显示（完整内容）==========
    gfx->setTextColor(color);
    gfx->setTextSize(1);
    int lineWidth = SCREEN_WIDTH - 20;
    int charWidth = 6;
    int charsPerLine = lineWidth / charWidth;
    
    int yPos = 10;
    int startIdx = 0;
    
    while (startIdx < message.length() && yPos < SCREEN_HEIGHT - 10) {
      int endIdx = min(startIdx + charsPerLine, (int)message.length());
      
      // 查找换行符
      int newlineIdx = message.indexOf('\n', startIdx);
      if (newlineIdx != -1 && newlineIdx < endIdx) {
        endIdx = newlineIdx;
      }
      
      String line = message.substring(startIdx, endIdx);
      gfx->setCursor(10, yPos);
      gfx->println(line);
      
      yPos += 10;
      startIdx = endIdx;
      if (newlineIdx != -1 && newlineIdx == endIdx) {
        startIdx++; // 跳过换行符
      }
    }
  }
}
