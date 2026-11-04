# Robotics & Embedded Systems | Engineering Portfolio

**机器人系统 · 嵌入式实时控制 · 机器人运动学 · 计算机视觉 · 物联网**

这里是个人精选工程源代码集合。项目采用「编号 + 工程代号 + 技术领域」命名，并在下方附真实技术说明，便于长期管理、作品展示与专业检索。目录名称是作品品牌名，不代表额外实现了源码中尚不存在的功能。

> 本目录全部源码来自本仓库已经筛选的 **2026-10-13 历史归档快照**。这里不是独立个人仓库最新版的自动镜像，不能据此声称已完成编译、实机验收或商业许可审核。只重命名精选源码目录与入口文档；原始归档、个人独立仓库及组织仓库均未更改。

## 精选工程 / Featured Projects

| No. | 工程代号 | 专业项目名称 |
|:---:|---|---|
| 01 | [**AegisMotion RTOS**](./01-AegisMotion-RTOS/) | STM32F407 实时麦克纳姆底盘控制系统 |
| 02 | [**NexusMobile ROS2**](./02-NexusMobile-ROS2/) | ROS2 移动机器人软硬件集成工程 |
| 03 | [**KineArm Robotics**](./03-KineArm-Robotics/) | 机器人机械臂运动学与控制实验平台 |
| 04 | [**EdgeLink ESP32**](./04-EdgeLink-ESP32/) | ESP32-S3 嵌入式串口协议与外设控制工程 |
| 05 | [**AgroSense IoT**](./05-AgroSense-IoT/) | 智能农业多节点感知与远程控制系统 |
| 06 | [**HandPose CV**](./06-HandPose-CV/) | 计算机视觉手部姿态估计实验工程 |

## 工程说明 / Technical Overview

### AegisMotion RTOS · STM32F407 实时麦克纳姆底盘控制系统

- **源码入口：** [`projects/01-AegisMotion-RTOS/`](./01-AegisMotion-RTOS/)
- **技术要点：** FreeRTOS 静态任务调度、四轮逆运动学、Emm_V5 串口驱动、JY61P IMU、通信超时与紧急停车
- **历史归档：** [`embedded/bugrtoscar/otherWork/STM32F407ZGT6/`](../embedded/bugrtoscar/otherWork/STM32F407ZGT6/)
- **个人独立仓库：** https://github.com/dreamcather726/bugrtoscar（可能有更近期版本）

### NexusMobile ROS2 · ROS2 移动机器人软硬件集成工程

- **源码入口：** [`projects/02-NexusMobile-ROS2/`](./02-NexusMobile-ROS2/)
- **技术要点：** ROS 相关程序、ESP32S3 固件与机器人系统工程资料；各功能集成情况需实机验证
- **历史归档：** [`robotics/ROS-CAR/`](../robotics/ROS-CAR/)
- **个人独立仓库：** https://github.com/dreamcather726/ROS-CAR（可能有更近期版本）

### KineArm Robotics · 机器人机械臂运动学与控制实验平台

- **源码入口：** [`projects/03-KineArm-Robotics/`](./03-KineArm-Robotics/)
- **技术要点：** 机械臂控制、运动学相关实现及 Python / ROS 实验
- **历史归档：** [`robotics/JXB2.0/`](../robotics/JXB2.0/)
- **个人独立仓库：** https://github.com/dreamcather726/JXB2.0（可能有更近期版本）

### EdgeLink ESP32 · ESP32-S3 嵌入式串口协议与外设控制工程

- **源码入口：** [`projects/04-EdgeLink-ESP32/`](./04-EdgeLink-ESP32/)
- **技术要点：** ESP-IDF / CMake、UART 二进制协议、CRC16、PWM 和 LED 基础模块；不宣称已实现 ROS2 完整通信
- **历史归档：** [`embedded/esp32ROS2RTOS/`](../embedded/esp32ROS2RTOS/)

### AgroSense IoT · 智能农业多节点感知与远程控制系统

- **源码入口：** [`projects/05-AgroSense-IoT/`](./05-AgroSense-IoT/)
- **技术要点：** Arduino 传感节点、树莓派中控、MQTT / 云平台通信与远程监控相关源码
- **历史归档：** [`applications/iot-seedling-graduation/`](../applications/iot-seedling-graduation/)
- **个人独立仓库：** https://github.com/dreamcather726/IoT-Based-Seedling-Control-System（可能有更近期版本）

### HandPose CV · 计算机视觉手部姿态估计实验工程

- **源码入口：** [`projects/06-HandPose-CV/`](./06-HandPose-CV/)
- **技术要点：** Python 手部姿态推理相关模块与模型组件；准确率和实时性能尚未验收
- **历史归档：** [`vision/handpose-x/`](../vision/handpose-x/)

## 版本与资产保护

1. 源码本体保留原有目录结构，精选入口采用 Git 树对象复用归档代码；本次只变更精选目录名称与说明文档。
2. 原有归档目录、个人独立仓库、历史提交全部保留；**组织仓库不在整理范围内**。
3. 刚整理的项目入口是历史快照，今后不会自动同步，更新时需单独核对个人源仓库版本、文件差异、凭据和许可证。
4. 已归档源码曾做过敏感数据筛查与占位处理，详见 [归档检查说明](../docs/ARCHIVE_SCOPE.md)；商业发布前还需核对著作权归属和第三方许可证。
5. 完整历史归档项目见 [项目目录（65 个项目/版本目录）](../docs/PROJECTS.md)。
