#include "center_storage.h"

#include <Arduino.h>
#include <LittleFS.h>

#include "dog_control.h"

// 使用新的文件名，确保本次最终零位不会被旧版左右映射下的校准文件覆盖。
static const char* CENTER_TRIM_FILE = "/center_trim_v3.txt";
static const char* STAND_POSE_FILE = "/stand_pose.txt";
static const char* GAIT_CONFIG_FILE = "/gait_config.txt";
static const char* STORAGE_FORMAT_V2 = "v2";

// 旧版文件的四行依次对应 CH0、CH4、CH8、CH12；修正左右命名后，
// 需要按物理通道把旧数据迁移到当前的腿索引。
static const int LEGACY_TO_CURRENT_LEG[LEG_COUNT] = {
  LEG_LF, LEG_RF, LEG_LB, LEG_RB
};

static int splitCsvLine(const String& line, int values[], int maxValueCount)
{
  int valueCount = 0;
  int startIndex = 0;
  while (valueCount < maxValueCount && startIndex <= line.length()) {
    int commaIndex = line.indexOf(',', startIndex);
    String valueText = commaIndex < 0 ? line.substring(startIndex) : line.substring(startIndex, commaIndex);
    valueText.trim();
    if (valueText.length() == 0) break;
    values[valueCount++] = valueText.toInt();
    if (commaIndex < 0) break;
    startIndex = commaIndex + 1;
  }
  return valueCount;
}

static void loadJointValues(const String& line, int target[JOINT_COUNT], int limitUs)
{
  int values[JOINT_COUNT] = {0, 0, 0};
  int valueCount = splitCsvLine(line, values, JOINT_COUNT);

  if (valueCount == 2) {
    // 兼容旧版 8 自由度文件：旧数据依次为大腿、小腿，新髋关节保持 0。
    target[JOINT_HIP] = 0;
    target[JOINT_THIGH] = clampValue(values[0], -limitUs, limitUs);
    target[JOINT_CALF] = clampValue(values[1], -limitUs, limitUs);
    return;
  }

  if (valueCount == JOINT_COUNT) {
    for (int jointIndex = 0; jointIndex < JOINT_COUNT; jointIndex++) {
      target[jointIndex] = clampValue(values[jointIndex], -limitUs, limitUs);
    }
  }
}

static void saveJointValues(File& file, int values[JOINT_COUNT])
{
  for (int jointIndex = 0; jointIndex < JOINT_COUNT; jointIndex++) {
    if (jointIndex > 0) file.print(',');
    file.print(values[jointIndex]);
  }
  file.println();
}

static void loadJointMatrix(File& file, int target[LEG_COUNT][JOINT_COUNT], int limitUs)
{
  if (!file.available()) return;

  String firstLine = file.readStringUntil('\n');
  firstLine.trim();
  bool isCurrentFormat = firstLine == STORAGE_FORMAT_V2;

  if (!isCurrentFormat) {
    loadJointValues(firstLine, target[LEGACY_TO_CURRENT_LEG[0]], limitUs);
  }

  int firstDataLine = isCurrentFormat ? 0 : 1;
  for (int lineIndex = firstDataLine; lineIndex < LEG_COUNT && file.available(); lineIndex++) {
    String line = file.readStringUntil('\n');
    line.trim();
    int legIndex = isCurrentFormat ? lineIndex : LEGACY_TO_CURRENT_LEG[lineIndex];
    loadJointValues(line, target[legIndex], limitUs);
  }
}

static void saveJointMatrix(File& file, int values[LEG_COUNT][JOINT_COUNT])
{
  file.println(STORAGE_FORMAT_V2);
  for (int legIndex = 0; legIndex < LEG_COUNT; legIndex++) {
    saveJointValues(file, values[legIndex]);
  }
}

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

  loadJointMatrix(trimFile, centerTrimUs, CENTER_TRIM_LIMIT_US);

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

  saveJointMatrix(trimFile, centerTrimUs);

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

  loadJointMatrix(poseFile, standPoseUs, STAND_POSE_LIMIT_US);

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

  saveJointMatrix(poseFile, standPoseUs);

  poseFile.close();
  Serial.println("Stand pose saved");
  return true;
}

bool loadGaitConfigFromFile()
{
  if (!LittleFS.exists(GAIT_CONFIG_FILE)) {
    Serial.println("No gait config file, use default gait");
    return false;
  }

  File configFile = LittleFS.open(GAIT_CONFIG_FILE, "r");
  if (!configFile) {
    Serial.println("Open gait config file failed");
    return false;
  }

  String line = configFile.readStringUntil('\n');
  configFile.close();
  int values[3] = {0, 0, 0};
  if (splitCsvLine(line, values, 3) != 3) {
    Serial.println("Invalid gait config file");
    return false;
  }

  setGaitParameters(values[0], values[1], values[2]);
  Serial.println("Gait config loaded");
  return true;
}

bool saveGaitConfigToFile()
{
  File configFile = LittleFS.open(GAIT_CONFIG_FILE, "w");
  if (!configFile) {
    Serial.println("Save gait config file failed");
    return false;
  }

  configFile.print(gaitFrameDelayMs);
  configFile.print(',');
  configFile.print(gaitThighAmplitudeUs);
  configFile.print(',');
  configFile.println(gaitCalfAmplitudeUs);
  configFile.close();
  Serial.println("Gait config saved");
  return true;
}
