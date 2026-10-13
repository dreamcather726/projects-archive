#ifndef CENTER_STORAGE_H
#define CENTER_STORAGE_H

// 中位微调会保存到 ESP32 LittleFS 文件中，上电后自动读取。
void initCenterStorage();
bool loadCenterTrimFromFile();
bool saveCenterTrimToFile();
bool loadStandPoseFromFile();
bool saveStandPoseToFile();
bool loadGaitConfigFromFile();
bool saveGaitConfigToFile();

#endif
