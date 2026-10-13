#include "center_storage.h"
#include "dog_control.h"
#include "pca9685_servo.h"
#include "wifi_control.h"

void setup()
{
  // 串口只用于查看 WiFi 连接状态和路由器分配的 IP。
  Serial.begin(115200);

  initPca9685Servo();
  initCenterStorage();
  loadCenterTrimFromFile();
  loadStandPoseFromFile();
  connectWifi();
  setupWebServer();
  moveDogHome();
}

void loop()
{
  // 主循环只处理 WiFi 网页请求，控制逻辑放在其它文件中。
  handleWifiClient();
}
