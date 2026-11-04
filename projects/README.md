# Robotics · Embedded · AI | 精选工程源码集合

> **按技术含金量从高到低排列，共 14 个工程项目。** 这是私人源码资产库，不是可以直接售卖的发行包。每个目录包含实际源码，项目的原始归档与个人独立仓库保留不变。

## 排名说明

排序综合考虑 **技术实现深度（约 45%）、系统工程完整性（约 30%）、可复现和教学复用性（约 25%）**。评估依赖已检查的关键源码、测试目录和项目说明；目前未逐项目重新编译或实机验收。排名是主观工程判断，不代表售卖收入、版权归属或商业可发行状态。跨等级差异比同等级内部的细微顺序更重要。

等级：**S 核心代表作、A 高价值工程、B 专题实战项目、C 基础/待完善项目**。

> 2026-11-05 本次仅按排名**重编号精选目录**，没有修改各项目源码内容。目录链接会随编号调整，但 Git 仓库历史与源码对象不会丢失。

## 项目总览（从高到低）

| 排名 | 工程项目 | 技术方向 | 原收录版本 | 等级 |
|:---:|---|---|---|:---:|
| 01 | [**AegisMotion RTOS**](./01-AegisMotion-RTOS/) | STM32F407 + FreeRTOS 实时麦克纳姆底盘 | 2026-10-13 历史快照 | S |
| 02 | [**CogniFlow ROS2**](./02-CogniFlow-ROS2/) | 本地 AI 语音交互与机器人任务编排 | 2026-11-05 个人仓库复制快照 | S |
| 03 | [**NexusMobile ROS2**](./03-NexusMobile-ROS2/) | ROS2 移动机器人多模块集成工程 | 2026-10-13 历史快照 | A |
| 04 | [**KineArm Robotics**](./04-KineArm-Robotics/) | 机械臂运动学与控制实验 | 2026-10-13 历史快照 | A |
| 05 | [**ServoLab SO-ARM**](./05-ServoLab-SOARM/) | 机械臂舵机校准、遥操作与轨迹规划实验 | 2026-10-13 历史快照 | A |
| 06 | [**ROS2 Engineering Labs**](./06-ROS2-Engineering-Labs/) | ROS2 Humble 课堂实验与工程教学案例 | 2026-10-13 历史快照；较新课程见独立仓库 ros2-jiangyi | A |
| 07 | [**BlueForge BLE**](./07-BlueForge-BLE/) | Windows BLE GATT 通信与诊断工具 | 2026-11-05 从个人仓库 main 复制（10 文件） | A |
| 08 | [**DepthSight Vision**](./08-DepthSight-Vision/) | RGB 摄像头单目深度估计与可视化 | 2026-11-05 从个人仓库 main 复制（7 文件） | A |
| 09 | [**MotionForge Servo**](./09-MotionForge-Servo/) | 多舵机控制与动作脚本编排工具 | 2026-11-05 从个人仓库 main 复制（8 文件，已排除 Python 缓存） | B |
| 10 | [**AgroSense IoT**](./10-AgroSense-IoT/) | 农业物联网多节点控制系统 | 2026-10-13 历史快照 | B |
| 11 | [**RoboSim Cockpit**](./11-RoboSim-Cockpit/) | ROS2 移动机器人遥测仿真与上位机面板 | 2026-10-13 历史快照 | B |
| 12 | [**DexterousLab Hand**](./12-DexterousLab-Hand/) | 视觉手势与灵巧手控制系列实验 | 2026-10-13 历史快照 | B |
| 13 | [**HandPose CV**](./13-HandPose-CV/) | 手部姿态视觉识别实验 | 2026-10-13 历史快照 | C |
| 14 | [**EdgeLink ESP32**](./14-EdgeLink-ESP32/) | ESP32-S3 串口协议及基础驱动 | 2026-10-13 历史快照 | C |

## 排名依据

| 排名 | 项目 | 等级 | 主要理由 |
|:---:|---|:---:|---|
| 01 | [**AegisMotion RTOS**](./01-AegisMotion-RTOS/) | S | 实时任务、运动学、通信状态机与安全机制 |
| 02 | [**CogniFlow ROS2**](./02-CogniFlow-ROS2/) | S | ASR + LLM + ROS2 任务编排与模拟执行 |
| 03 | [**NexusMobile ROS2**](./03-NexusMobile-ROS2/) | A | 机器人软硬件多模块集成（尚需整体验收） |
| 04 | [**KineArm Robotics**](./04-KineArm-Robotics/) | A | 机械臂运动学、仿真与控制实验 |
| 05 | [**ServoLab SOARM**](./05-ServoLab-SOARM/) | A | 校准、遥操作与轨迹规划；需核对第三方来源 |
| 06 | [**ROS2 Engineering Labs**](./06-ROS2-Engineering-Labs/) | A | ROS2 核心通信、仿真与系列实验教学 |
| 07 | [**BlueForge BLE**](./07-BlueForge-BLE/) | A | BLE GATT 通信、异步 GUI 与测试 |
| 08 | [**DepthSight Vision**](./08-DepthSight-Vision/) | A | 单目深度估计、CUDA/CPU 实测与测试 |
| 09 | [**MotionForge Servo**](./09-MotionForge-Servo/) | B | 舵机 GUI、动作脚本与 ESP32 示例 |
| 10 | [**AgroSense IoT**](./10-AgroSense-IoT/) | B | Arduino + 树莓派 + 云端多节点控制 |
| 11 | [**RoboSim Cockpit**](./11-RoboSim-Cockpit/) | B | ROS2 遥测数据模拟与 Cockpit 框架 |
| 12 | [**DexterousLab Hand**](./12-DexterousLab-Hand/) | B | 手势识别与灵巧手阶段实验 |
| 13 | [**HandPose CV**](./13-HandPose-CV/) | C | 视觉推理实验，尚需性能与依赖验证 |
| 14 | [**EdgeLink ESP32**](./14-EdgeLink-ESP32/) | C | ESP-IDF 基础驱动与串口协议，尚非完整系统 |

## 技术方向导航

- **实时控制、嵌入式与通信**：01 AegisMotion、07 BlueForge、09 MotionForge、14 EdgeLink
- **机器人平台、仿真与机械臂**：03 NexusMobile、04 KineArm、05 ServoLab、11 RoboSim
- **AI 与机器视觉**：02 CogniFlow、08 DepthSight、12 DexterousLab、13 HandPose
- **工程课程与物联网**：06 ROS2 Engineering Labs、10 AgroSense

## 各项目的源码与技术说明

### 01 · AegisMotion RTOS

- **工程名称**：STM32F407 + FreeRTOS 实时麦克纳姆底盘
- **实际源码**：[`projects/01-AegisMotion-RTOS/`](./01-AegisMotion-RTOS/)
- **关键技术**：FreeRTOS、四轮逆运动学、Emm_V5、IMU、故障停车
- **来源**：`embedded/bugrtoscar`（2026-10-13 历史快照）
- **个人仓库/更新参照**：https://github.com/dreamcather726/bugrtoscar
- **商业化前提**：待权属及实车验收

### 02 · CogniFlow ROS2

- **工程名称**：本地 AI 语音交互与机器人任务编排
- **实际源码**：[`projects/02-CogniFlow-ROS2/`](./02-CogniFlow-ROS2/)
- **关键技术**：FunASR、Ollama、ROS2 Action、PySide6、任务模拟器
- **来源**：`个人独立仓库 ai-interaction-ui`（2026-11-05 个人仓库复制快照）
- **个人仓库/更新参照**：https://github.com/dreamcather726/ai-interaction-ui
- **商业化前提**：模拟链路已有源码；真实机器人需另行联调

### 03 · NexusMobile ROS2

- **工程名称**：ROS2 移动机器人多模块集成工程
- **实际源码**：[`projects/03-NexusMobile-ROS2/`](./03-NexusMobile-ROS2/)
- **关键技术**：ROS2、ESP32、串口、视觉控制实验
- **来源**：`robotics/ROS-CAR`（2026-10-13 历史快照）
- **个人仓库/更新参照**：https://github.com/dreamcather726/ROS-CAR
- **商业化前提**：待源代码整理与硬件联调

### 04 · KineArm Robotics

- **工程名称**：机械臂运动学与控制实验
- **实际源码**：[`projects/04-KineArm-Robotics/`](./04-KineArm-Robotics/)
- **关键技术**：机械臂、Python 控制、PyBullet、运动学
- **来源**：`robotics/JXB2.0`（2026-10-13 历史快照）
- **个人仓库/更新参照**：https://github.com/dreamcather726/JXB2.0
- **商业化前提**：需审查原有许可证及第三方源码

### 05 · ServoLab SO-ARM

- **工程名称**：机械臂舵机校准、遥操作与轨迹规划实验
- **实际源码**：[`projects/05-ServoLab-SOARM/`](./05-ServoLab-SOARM/)
- **关键技术**：舵机 ID、标定、控制接口、遥操作、轨迹规划实验
- **来源**：`robotics/so-arm100-control-experiments`（2026-10-13 历史快照）
- **商业化前提**：可能含第三方内容；仅作私有研究归档，商用前审查许可证

### 06 · ROS2 Engineering Labs

- **工程名称**：ROS2 Humble 课堂实验与工程教学案例
- **实际源码**：[`projects/06-ROS2-Engineering-Labs/`](./06-ROS2-Engineering-Labs/)
- **关键技术**：ROS2 Topic/Service/Action、TF、QoS、URDF、Gazebo
- **来源**：`courses/ros2-lessons`（2026-10-13 历史快照；较新课程见独立仓库 ros2-jiangyi）
- **个人仓库/更新参照**：https://github.com/dreamcather726/ros2-jiangyi
- **商业化前提**：商业发布前核对讲义版权、课件和引用材料

### 07 · BlueForge BLE

- **工程名称**：Windows BLE GATT 通信与诊断工具
- **实际源码**：[`projects/07-BlueForge-BLE/`](./07-BlueForge-BLE/)
- **关键技术**：Bleak、Tkinter、异步通信、GATT 读写、通知订阅、单元测试
- **来源**：`个人独立仓库 bluetooth`（2026-11-05 从个人仓库 main 复制（10 文件））
- **个人仓库/更新参照**：https://github.com/dreamcather726/bluetooth
- **商业化前提**：适合工具型产品；需跨设备兼容性测试

### 08 · DepthSight Vision

- **工程名称**：RGB 摄像头单目深度估计与可视化
- **实际源码**：[`projects/08-DepthSight-Vision/`](./08-DepthSight-Vision/)
- **关键技术**：YOLO26 Depth、PyTorch、OpenCV、CUDA/CPU、实测记录、单元测试
- **来源**：`个人独立仓库 vision-interaction`（2026-11-05 从个人仓库 main 复制（7 文件））
- **个人仓库/更新参照**：https://github.com/dreamcather726/vision-interaction
- **商业化前提**：模型精度未校准；商业化先确认 Ultralytics 许可

### 09 · MotionForge Servo

- **工程名称**：多舵机控制与动作脚本编排工具
- **实际源码**：[`projects/09-MotionForge-Servo/`](./09-MotionForge-Servo/)
- **关键技术**：Python 串口 GUI、ESP32 舵机示例、动作脚本、单元测试
- **来源**：`个人独立仓库 dog`（2026-11-05 从个人仓库 main 复制（8 文件，已排除 Python 缓存））
- **个人仓库/更新参照**：https://github.com/dreamcather726/dog
- **商业化前提**：可做工具与教学专题；先验证目标硬件

### 10 · AgroSense IoT

- **工程名称**：农业物联网多节点控制系统
- **实际源码**：[`projects/10-AgroSense-IoT/`](./10-AgroSense-IoT/)
- **关键技术**：Arduino、树莓派、MQTT、云平台、远程控制
- **来源**：`applications/iot-seedling-graduation`（2026-10-13 历史快照）
- **个人仓库/更新参照**：https://github.com/dreamcather726/IoT-Based-Seedling-Control-System
- **商业化前提**：必须复核密钥、云服务和毕业设计归属

### 11 · RoboSim Cockpit

- **工程名称**：ROS2 移动机器人遥测仿真与上位机面板
- **实际源码**：[`projects/11-RoboSim-Cockpit/`](./11-RoboSim-Cockpit/)
- **关键技术**：ROS2 模拟地图/雷达/IMU/里程计/相机 + Cockpit GUI
- **来源**：`robotics/VMGUI`（2026-10-13 历史快照）
- **商业化前提**：适合作为无实车验证课程；需构建和运行验收

### 12 · DexterousLab Hand

- **工程名称**：视觉手势与灵巧手控制系列实验
- **实际源码**：[`projects/12-DexterousLab-Hand/`](./12-DexterousLab-Hand/)
- **关键技术**：手部识别、Arduino/Python、课程阶段实验
- **来源**：`courses/dexterous-hand-online`（2026-10-13 历史快照）
- **商业化前提**：课程可能涉及团队成果；确认授权后才能出售

### 13 · HandPose CV

- **工程名称**：手部姿态视觉识别实验
- **实际源码**：[`projects/13-HandPose-CV/`](./13-HandPose-CV/)
- **关键技术**：Python、计算机视觉、手部姿态推理
- **来源**：`vision/handpose-x`（2026-10-13 历史快照）
- **商业化前提**：待模型授权和性能验证

### 14 · EdgeLink ESP32

- **工程名称**：ESP32-S3 串口协议及基础驱动
- **实际源码**：[`projects/14-EdgeLink-ESP32/`](./14-EdgeLink-ESP32/)
- **关键技术**：ESP-IDF、CMake、CRC16、UART、PWM
- **来源**：`embedded/esp32ROS2RTOS`（2026-10-13 历史快照）
- **商业化前提**：功能仍属基础实验，非完整 ROS2 固件

## 使用、版本与安全边界

1. **来源严格限于个人账号的独立仓库与现有个人私有归档**；没有读取、修改或复制组织仓库。
2. **01–06、11–14** 保留 2026-10-13 已归档源码快照；**07** 是之前收录的 2026-11-05 AI 交互仓库快照；**08–10** 是从个人仓库主分支复制的源码快照，按 Git blob SHA 核验。所有副本均**不会**自动同步源仓库。
3. **08–10 不包含生成缓存和环境文件**：08 完整收录 10 个源文件；09 完整收录 7 个源文件；10 收录 8 个源码及必要配置文件，排除了原仓库的 `__pycache__` 文件。
4. **11–14** 通过复用本仓库已归档 Git tree 对象收录，保留历史原文件，避免逐项重复上传。不能将这些历史快照视为最新开发成果。
5. 各项目原有独立仓库、所有原归档目录与 Git 提交历史均未删除、覆盖或重写。创建精选副本不改变任何第三方版权归属。
6. 不保证各项目已经重新编译、实机运行或具备商业授权。特别需要复核公司/团队权属、第三方许可证、模型权重、密钥及设备连接配置。详见 [商品化审核清单](COMMERCIALIZATION_CHECKLIST.md)。
7. 原归档有 65 个项目/版本目录，不在本次精选范围的资料仍可通过 [完整归档目录](../docs/PROJECTS.md) 查找。
