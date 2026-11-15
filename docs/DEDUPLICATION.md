# Project deduplication

Date: 2026-11-15

Eight duplicate project copies were consolidated into the curated `projects/` directories. All tracked files, including licenses, matched byte-for-byte by Git blob ID and relative path; only `_ARCHIVE.md` provenance notes were excluded from comparison. No partial matches or differing versions were merged.

| Previous directory | Canonical source | Source files |
| --- | --- | ---: |
| [applications/iot-seedling-graduation](../applications/iot-seedling-graduation/) | [projects/10-AgroSense-IoT](../projects/10-AgroSense-IoT/) | 98 |
| [courses/dexterous-hand-online](../courses/dexterous-hand-online/) | [projects/12-DexterousLab-Hand](../projects/12-DexterousLab-Hand/) | 109 |
| [embedded/esp32ROS2RTOS](../embedded/esp32ROS2RTOS/) | [projects/14-EdgeLink-ESP32](../projects/14-EdgeLink-ESP32/) | 24 |
| [robotics/JXB2.0](../robotics/JXB2.0/) | [projects/04-KineArm-Robotics](../projects/04-KineArm-Robotics/) | 127 |
| [robotics/ROS-CAR](../robotics/ROS-CAR/) | [projects/03-NexusMobile-ROS2](../projects/03-NexusMobile-ROS2/) | 570 |
| [robotics/VMGUI](../robotics/VMGUI/) | [projects/11-RoboSim-Cockpit](../projects/11-RoboSim-Cockpit/) | 26 |
| [robotics/so-arm100-control-experiments](../robotics/so-arm100-control-experiments/) | [projects/05-ServoLab-SOARM](../projects/05-ServoLab-SOARM/) | 162 |
| [vision/handpose-x](../vision/handpose-x/) | [projects/13-HandPose-CV](../projects/13-HandPose-CV/) | 12 |

Old directories retain navigation and provenance. The manifest now points to canonical sources and records `original_directory`; redaction paths follow the same mapping. Manifest source counts describe the same archive selection, not every file in the repository.

Versions of Xiaozhi and HandTracking with different contents remain separate. Existing unrelated local deletions are excluded from this commit.

## 后续整理

上文描述第一轮完全重复项目整理。第二轮已进一步归并部分近似版本，当前状态以 [近似项目整理记录](NEAR_DUPLICATES.md) 和 manifest 为准。
