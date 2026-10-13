#include "center_storage.h"
#include "dog_control.h"
#include "pca9685_servo.h"
#include "wifi_control.h"

#define AUTO_WALK_SETTLE_MS 1000

void setup()
{
  // 串口只用于查看 WiFi 连接状态和路由器分配的 IP。
  Serial.begin(115200);
  initPca9685Servo();
  initCenterStorage();
  loadCenterTrimFromFile();
  loadStandPoseFromFile();
  loadGaitConfigFromFile();
  connectWifi();
  setupWebServer();
  // 先输出最终零位，等待舵机稳定后再开始自动前进。
  moveDogCenter();
  delay(AUTO_WALK_SETTLE_MS);

  // 上电后持续使用对角步态向前走；网页“停止”可退出循环。
  walkDogContinuously(
    gaitFrameDelayMs,
    gaitThighAmplitudeUs,
    gaitCalfAmplitudeUs
  );
}

void loop()
{
  // 主循环只处理 WiFi 网页请求，控制逻辑放在其它文件中。
  handleWifiClient();
}
