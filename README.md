# Projects Archive

## Robotics · Embedded · AI | 按含金量排序的精选工程集合

[**查看全部 14 项工程源码（按技术深度排序） →**](projects/README.md)。排名综合技术实现、系统完整度和可复现教学价值；不代表已取得商业使用权或全部项目已完成实机验证。

- **[AegisMotion RTOS](projects/01-AegisMotion-RTOS/)** — STM32F407 + FreeRTOS 实时麦克纳姆底盘
- **[CogniFlow ROS2](projects/02-CogniFlow-ROS2/)** — 本地 AI 语音交互与机器人任务编排
- **[NexusMobile ROS2](projects/03-NexusMobile-ROS2/)** — ROS2 移动机器人多模块集成工程
- **[KineArm Robotics](projects/04-KineArm-Robotics/)** — 机械臂运动学与控制实验
- **[ServoLab SO-ARM](projects/05-ServoLab-SOARM/)** — 机械臂舵机校准、遥操作与轨迹规划实验
- **[ROS2 Engineering Labs](projects/06-ROS2-Engineering-Labs/)** — ROS2 Humble 课堂实验与工程教学案例
- **[BlueForge BLE](projects/07-BlueForge-BLE/)** — Windows BLE GATT 通信与诊断工具
- **[DepthSight Vision](projects/08-DepthSight-Vision/)** — RGB 摄像头单目深度估计与可视化
- **[MotionForge Servo](projects/09-MotionForge-Servo/)** — 多舵机控制与动作脚本编排工具
- **[AgroSense IoT](projects/10-AgroSense-IoT/)** — 农业物联网多节点控制系统
- **[RoboSim Cockpit](projects/11-RoboSim-Cockpit/)** — ROS2 移动机器人遥测仿真与上位机面板
- **[DexterousLab Hand](projects/12-DexterousLab-Hand/)** — 视觉手势与灵巧手控制系列实验
- **[HandPose CV](projects/13-HandPose-CV/)** — 手部姿态视觉识别实验
- **[EdgeLink ESP32](projects/14-EdgeLink-ESP32/)** — ESP32-S3 串口协议及基础驱动

> 本仓库为私有技术资产库。本次只调整精选项目的目录编号及说明文档，**源码内容、原始归档、个人独立仓库及组织仓库均保持不变**。商业发布前请查看 [商品化审核清单](projects/COMMERCIALIZATION_CHECKLIST.md)。

个人项目源码归档，来自 C 盘和 D 盘的开发目录。整理于 **2026-10-13**。

当前收录 **65 个项目/版本目录**、**10066 个源码与资源文件**。

- [完整项目目录](docs/PROJECTS.md)
- [文件清单与 SHA-256 校验值](docs/manifest.json)
- [后续添加项目](docs/ADDING_PROJECTS.md)
- [整理范围与未收录项](docs/ARCHIVE_SCOPE.md)

| 分类 | 内容 | 目录数 |
| --- | --- | ---: |
| [embedded](embedded/) | 嵌入式与单片机 | 22 |
| [robotics](robotics/) | 机器人与机械臂 | 12 |
| [vision](vision/) | 视觉与灵巧手 | 6 |
| [ai](ai/) | AI 与语音 | 8 |
| [applications](applications/) | 应用与毕业设计 | 2 |
| [courses](courses/) | 课程与实验 | 10 |
| [references](references/) | 参考示例 | 5 |

## 使用项目

每个项目保持自身目录结构；打开该项目的原 README、依赖清单或构建入口开始使用。各项目可能需要不同版本的 Python、ESP-IDF、PlatformIO、ROS 或 STM32 工具链，仓库根目录不提供统一构建。

整理过程中在 **50 个文件**中替换了凭据或连接配置候选值。`REDACTED_CONFIGURE_LOCALLY` 表示需要在本地填写配置。原文件保留在原盘目录。

这是源码快照归档。目录中的第三方代码仍适用各自原始许可证；保留了原有许可证文件，没有为所有项目统一授予新许可证。

后续新增项目建议放入相应分类的独立目录，并更新项目目录和 README。
