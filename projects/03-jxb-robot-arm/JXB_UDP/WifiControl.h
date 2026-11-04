#ifndef WIFI_CONTROL_H
#define WIFI_CONTROL_H

#include <Arduino.h>

void startWifiControl(
  const char* ssid,
  const char* password,
  int servoCount,
  int currentAngles[],
  int targetAngles[],
  const int positionA[],
  const int positionB[],
  bool* stopFlag
);

void handleWifiControlClient();

#endif
