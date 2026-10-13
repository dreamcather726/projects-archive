#include "center_storage.h"

#include <Arduino.h>
#include <LittleFS.h>

#include "dog_control.h"

static const char* CENTER_TRIM_FILE = "/center_trim.txt";
static const char* STAND_POSE_FILE = "/stand_pose.txt";

void initCenterStorage()
{
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed");
    return;
  }
  Serial.println("LittleFS ready");
}

bool loadCenterTrimFromFile()
{
  if (!LittleFS.exists(CENTER_TRIM_FILE)) {
    Serial.println("No center trim file, use default center");
    return false;
  }

  File trimFile = LittleFS.open(CENTER_TRIM_FILE, "r");
  if (!trimFile) {
    Serial.println("Open center trim file failed");
    return false;
  }

  for (int legIndex = 0; legIndex < LEG_COUNT && trimFile.available(); legIndex++) {
    String line = trimFile.readStringUntil('\n');
    line.trim();
    int commaIndex = line.indexOf(',');
    if (commaIndex <= 0) continue;

    int thighTrimUs = line.substring(0, commaIndex).toInt();
    int calfTrimUs = line.substring(commaIndex + 1).toInt();
    centerTrimUs[legIndex][JOINT_THIGH] = clampValue(thighTrimUs, -CENTER_TRIM_LIMIT_US, CENTER_TRIM_LIMIT_US);
    centerTrimUs[legIndex][JOINT_CALF] = clampValue(calfTrimUs, -CENTER_TRIM_LIMIT_US, CENTER_TRIM_LIMIT_US);
  }

  trimFile.close();
  Serial.println("Center trim loaded");
  return true;
}

bool saveCenterTrimToFile()
{
  File trimFile = LittleFS.open(CENTER_TRIM_FILE, "w");
  if (!trimFile) {
    Serial.println("Save center trim file failed");
    return false;
  }

  for (int legIndex = 0; legIndex < LEG_COUNT; legIndex++) {
    trimFile.print(centerTrimUs[legIndex][JOINT_THIGH]);
    trimFile.print(",");
    trimFile.println(centerTrimUs[legIndex][JOINT_CALF]);
  }

  trimFile.close();
  Serial.println("Center trim saved");
  return true;
}

bool loadStandPoseFromFile()
{
  if (!LittleFS.exists(STAND_POSE_FILE)) {
    Serial.println("No stand pose file, use default stand pose");
    return false;
  }

  File poseFile = LittleFS.open(STAND_POSE_FILE, "r");
  if (!poseFile) {
    Serial.println("Open stand pose file failed");
    return false;
  }

  for (int legIndex = 0; legIndex < LEG_COUNT && poseFile.available(); legIndex++) {
    String line = poseFile.readStringUntil('\n');
    line.trim();
    int commaIndex = line.indexOf(',');
    if (commaIndex <= 0) continue;

    int thighPoseUs = line.substring(0, commaIndex).toInt();
    int calfPoseUs = line.substring(commaIndex + 1).toInt();
    standPoseUs[legIndex][JOINT_THIGH] = clampValue(thighPoseUs, -STAND_POSE_LIMIT_US, STAND_POSE_LIMIT_US);
    standPoseUs[legIndex][JOINT_CALF] = clampValue(calfPoseUs, -STAND_POSE_LIMIT_US, STAND_POSE_LIMIT_US);
  }

  poseFile.close();
  Serial.println("Stand pose loaded");
  return true;
}

bool saveStandPoseToFile()
{
  File poseFile = LittleFS.open(STAND_POSE_FILE, "w");
  if (!poseFile) {
    Serial.println("Save stand pose file failed");
    return false;
  }

  for (int legIndex = 0; legIndex < LEG_COUNT; legIndex++) {
    poseFile.print(standPoseUs[legIndex][JOINT_THIGH]);
    poseFile.print(",");
    poseFile.println(standPoseUs[legIndex][JOINT_CALF]);
  }

  poseFile.close();
  Serial.println("Stand pose saved");
  return true;
}
