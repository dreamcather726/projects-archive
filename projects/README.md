# Robotics · Embedded · AI | 精选工程源码集合

> **私人技术资产库，不是可直接售卖的发行包。** 截至 2026-11-05，本合集收录 **14 项高价值工程/实验项目**，采用编号 + 工程代号命名。每个项目目录都包含实际源码文件，不是 Git Submodule、快捷方式或仅外部链接。

## 技术方向

**嵌入式实时系统与通信**：01 AegisMotion、04 EdgeLink、08 BlueForge、10 MotionForge

**机器人控制与仿真**：02 NexusMobile、03 KineArm、11 RoboSim、12 ServoLab

**AI 与计算机视觉**：06 HandPose、07 CogniFlow、09 DepthSight、14 DexterousLab

**物联网与工程教学**：05 AgroSense、13 ROS2 Engineering Labs

## 项目总览

| 编号 | 工程代号 | 技术项目 | 收录版本 |
|:---:|---|---|---|
| 01 | [**AegisMotion RTOS**](./01-AegisMotion-RTOS/) | STM32F407 + FreeRTOS 实时麦克纳姆底盘 | 2026-10-13 历史快照 |
| 02 | [**NexusMobile ROS2**](./02-NexusMobile-ROS2/) | ROS2 移动机器人多模块集成工程 | 2026-10-13 历史快照 |
| 03 | [**KineArm Robotics**](./03-KineArm-Robotics/) | 机械臂运动学与控制实验 | 2026-10-13 历史快照 |
| 04 | [**EdgeLink ESP32**](./04-EdgeLink-ESP32/) | ESP32-S3 串口协议及基础驱动 | 2026-10-13 历史快照 |
| 05 | [**AgroSense IoT**](./05-AgroSense-IoT/) | 农业物联网多节点控制系统 | 2026-10-13 历史快照 |
| 06 | [**HandPose CV**](./06-HandPose-CV/) | 手部姿态视觉识别实验 | 2026-10-13 历史快照 |
| 07 | [**CogniFlow ROS2**](./07-CogniFlow-ROS2/) | 本地 AI 语音交互与机器人任务编排 | 2026-11-05 个人仓库复制快照 |
| 08 | [**BlueForge BLE**](./08-BlueForge-BLE/) | Windows BLE GATT 通信与诊断工具 | 2026-11-05 从个人仓库 main 复制（10 文件） |
| 09 | [**DepthSight Vision**](./09-DepthSight-Vision/) | RGB 摄像头单目深度估计与可视化 | 2026-11-05 从个人仓库 main 复制（7 文件） |
| 10 | [**MotionForge Servo**](./10-MotionForge-Servo/) | 多舵机控制与动作脚本编排工具 | 2026-11-05 从个人仓库 main 复制（8 文件，已排除 Python 缓存） |
| 11 | [**RoboSim Cockpit**](./11-RoboSim-Cockpit/) | ROS2 移动机器人遥测仿真与上位机面板 | 2026-10-13 历史快照 |
| 12 | [**ServoLab SO-ARM**](./12-ServoLab-SOARM/) | 机械臂舵机校准、遥操作与轨迹规划实验 | 2026-10-13 历史快照 |
| 13 | [**ROS2 Engineering Labs**](./13-ROS2-Engineering-Labs/) | ROS2 Humble 课堂实验与工程教学案例 | 2026-10-13 历史快照；较新课程见独立仓库 ros2-jiangyi |
| 14 | [**DexterousLab Hand**](./14-DexterousLab-Hand/) | 视觉手势与灵巧手控制系列实验 | 2026-10-13 历史快照 |

## 各项目的源码与技术说明

### 01 · AegisMotion RTOS

- **工程名称**：STM32F407 + FreeRTOS 实时麦克纳姆底盘
- **实际源码**：[`projects/01-AegisMotion-RTOS/`](./01-AegisMotion-RTOS/)
- **关键技术**：FreeRTOS、四轮逆运动学、Emm_V5、IMU、故障停车
- **来源**：`embedded/bugrtoscar`（2026-10-13 历史快照）
- **个人仓库/更新参照**：https://github.com/dreamcather726/bugrtoscar
- **商业化前提**：待权属及实车验收

### 02 · NexusMobile ROS2

- **工程名称**：ROS2 移动机器人多模块集成工程
- **实际源码**：[`projects/02-NexusMobile-ROS2/`](./02-NexusMobile-ROS2/)
- **关键技术**：ROS2、ESP32、串口、视觉控制实验
- **来源**：`robotics/ROS-CAR`（2026-10-13 历史快照）
- **个人仓库/更新参照**：https://github.com/dreamcather726/ROS-CAR
- **商业化前提**：待源代码整理与硬件联调

### 03 · KineArm Robotics

- **工程名称**：机械臂运动学与控制实验
- **实际源码**：[`projects/03-KineArm-Robotics/`](./03-KineArm-Robotics/)
- **关键技术**：机械臂、Python 控制、PyBullet、运动学
- **来源**：`robotics/JXB2.0`（2026-10-13 历史快照）
- **个人仓库/更新参照**：https://github.com/dreamcather726/JXB2.0
- **商业化前提**：需审查原有许可证及第三方源码

### 04 · EdgeLink ESP32

- **工程名称**：ESP32-S3 串口协议及基础驱动
- **实际源码**：[`projects/04-EdgeLink-ESP32/`](./04-EdgeLink-ESP32/)
- **关键技术**：ESP-IDF、CMake、CRC16、UART、PWM
- **来源**：`embedded/esp32ROS2RTOS`（2026-10-13 历史快照）
- **商业化前提**：功能仍属基础实验，非完整 ROS2 固件

### 05 · AgroSense IoT

- **工程名称**：农业物联网多节点控制系统
- **实际源码**：[`projects/05-AgroSense-IoT/`](./05-AgroSense-IoT/)
- **关键技术**：Arduino、树莓派、MQTT、云平台、远程控制
- **来源**：`applications/iot-seedling-graduation`（2026-10-13 历史快照）
- **个人仓库/更新参照**：https://github.com/dreamcather726/IoT-Based-Seedling-Control-System
- **商业化前提**：必须复核密钥、云服务和毕业设计归属

### 06 · HandPose CV

- **工程名称**：手部姿态视觉识别实验
- **实际源码**：[`projects/06-HandPose-CV/`](./06-HandPose-CV/)
- **关键技术**：Python、计算机视觉、手部姿态推理
- **来源**：`vision/handpose-x`（2026-10-13 历史快照）
- **商业化前提**：待模型授权和性能验证

### 07 · CogniFlow ROS2

- **工程名称**：本地 AI 语音交互与机器人任务编排
- **实际源码**：[`projects/07-CogniFlow-ROS2/`](./07-CogniFlow-ROS2/)
- **关键技术**：FunASR、Ollama、ROS2 Action、PySide6、任务模拟器
- **来源**：`个人独立仓库 ai-interaction-ui`（2026-11-05 个人仓库复制快照）
- **个人仓库/更新参照**：https://github.com/dreamcather726/ai-interaction-ui
- **商业化前提**：模拟链路已有源码；真实机器人需另行联调

### 08 · BlueForge BLE

- **工程名称**：Windows BLE GATT 通信与诊断工具
- **实际源码**：[`projects/08-BlueForge-BLE/`](./08-BlueForge-BLE/)
- **关键技术**：Bleak、Tkinter、异步通信、GATT 读写、通知订阅、单元测试
- **来源**：`个人独立仓库 bluetooth`（2026-11-05 从个人仓库 main 复制（10 文件））
- **个人仓库/更新参照**：https://github.com/dreamcather726/bluetooth
- **商业化前提**：适合工具型产品；需跨设备兼容性测试

### 09 · DepthSight Vision

- **工程名称**：RGB 摄像头单目深度估计与可视化
- **实际源码**：[`projects/09-DepthSight-Vision/`](./09-DepthSight-Vision/)
- **关键技术**：YOLO26 Depth、PyTorch、OpenCV、CUDA/CPU、实测记录、单元测试
- **来源**：`个人独立仓库 vision-interaction`（2026-11-05 从个人仓库 main 复制（7 文件））
- **个人仓库/更新参照**：https://github.com/dreamcather726/vision-interaction
- **商业化前提**：模型精度未校准；商业化先确认 Ultralytics 许可

### 10 · MotionForge Servo

- **工程名称**：多舵机控制与动作脚本编排工具
- **实际源码**：[`projects/10-MotionForge-Servo/`](./10-MotionForge-Servo/)
- **关键技术**：Python 串口 GUI、ESP32 舵机示例、动作脚本、单元测试
- **来源**：`个人独立仓库 dog`（2026-11-05 从个人仓库 main 复制（8 文件，已排除 Python 缓存））
- **个人仓库/更新参照**：https://github.com/dreamcather726/dog
- **商业化前提**：可做工具与教学专题；先验证目标硬件

### 11 · RoboSim Cockpit

- **工程名称**：ROS2 移动机器人遥测仿真与上位机面板
- **实际源码**：[`projects/11-RoboSim-Cockpit/`](./11-RoboSim-Cockpit/)
- **关键技术**：ROS2 模拟地图/雷达/IMU/里程计/相机 + Cockpit GUI
- **来源**：`robotics/VMGUI`（2026-10-13 历史快照）
- **商业化前提**：适合作为无实车验证课程；需构建和运行验收

### 12 · ServoLab SO-ARM

- **工程名称**：机械臂舵机校准、遥操作与轨迹规划实验
- **实际源码**：[`projects/12-ServoLab-SOARM/`](./12-ServoLab-SOARM/)
- **关键技术**：舵机 ID、标定、控制接口、遥操作、轨迹规划实验
- **来源**：`robotics/so-arm100-control-experiments`（2026-10-13 历史快照）
- **商业化前提**：可能含第三方内容；仅作私有研究归档，商用前审查许可证

### 13 · ROS2 Engineering Labs

- **工程名称**：ROS2 Humble 课堂实验与工程教学案例
- **实际源码**：[`projects/13-ROS2-Engineering-Labs/`](./13-ROS2-Engineering-Labs/)
- **关键技术**：ROS2 Topic/Service/Action、TF、QoS、URDF、Gazebo
- **来源**：`courses/ros2-lessons`（2026-10-13 历史快照；较新课程见独立仓库 ros2-jiangyi）
- **个人仓库/更新参照**：https://github.com/dreamcather726/ros2-jiangyi
- **商业化前提**：商业发布前核对讲义版权、课件和引用材料

### 14 · DexterousLab Hand

- **工程名称**：视觉手势与灵巧手控制系列实验
- **实际源码**：[`projects/14-DexterousLab-Hand/`](./14-DexterousLab-Hand/)
- **关键技术**：手部识别、Arduino/Python、课程阶段实验
- **来源**：`courses/dexterous-hand-online`（2026-10-13 历史快照）
- **商业化前提**：课程可能涉及团队成果；确认授权后才能出售

## 使用、版本与安全边界

1. **来源严格限于个人账号的独立仓库与现有个人私有归档**；没有读取、修改或复制组织仓库。
2. **01–06、11–14** 保留 2026-10-13 已归档源码快照；**07** 是之前收录的 2026-11-05 AI 交互仓库快照；**08–10** 是从个人仓库主分支复制的源码快照，按 Git blob SHA 核验。所有副本均**不会**自动同步源仓库。
3. **08–10 不包含生成缓存和环境文件**：08 完整收录 10 个源文件；09 完整收录 7 个源文件；10 收录 8 个源码及必要配置文件，排除了原仓库的 `__pycache__` 文件。
4. **11–14** 通过复用本仓库已归档 Git tree 对象收录，保留历史原文件，避免逐项重复上传。不能将这些历史快照视为最新开发成果。
5. 各项目原有独立仓库、所有原归档目录与 Git 提交历史均未删除、覆盖或重写。创建精选副本不改变任何第三方版权归属。
6. 不保证各项目已经重新编译、实机运行或具备商业授权。特别需要复核公司/团队权属、第三方许可证、模型权重、密钥及设备连接配置。详见 [商品化审核清单](COMMERCIALIZATION_CHECKLIST.md)。
7. 原归档有 65 个项目/版本目录，不在本次精选范围的资料仍可通过 [完整归档目录](../docs/PROJECTS.md) 查找。
