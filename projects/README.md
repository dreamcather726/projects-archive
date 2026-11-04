# 个人工程项目集合

这里的每个 `projects/编号-项目名/` 都包含可浏览、可克隆的**实际源码文件**，不是仅有说明文档或 Git 子模块链接。

该目录是从本仓库已存在、已筛选的归档源码中复用 Git 树对象整理而来：**没有搬动或删除原目录，没有访问或修改任何组织仓库，也没有改动个人独立仓库。**

> **重要：** 本次收录的是 `projects-archive` 已保存的历史快照（原归档日期 2026-10-13），不能视为各个人仓库 2026-11-05 的最新代码。新入口和归档原目录今后不会自动同步；更新某个项目时须单独审查并提交。

## 项目入口

| 编号 | 项目 | 归档源码位置 |
| --- | --- | --- |
| 1 | [STM32F407ZGT6 + FreeRTOS 麦克纳姆底盘](./01-stm32f407-freertos-mecanum/) | `embedded/bugrtoscar/otherWork/STM32F407ZGT6` |
| 2 | [ROS-CAR 机器人开发工程](./02-ros-car/) | `robotics/ROS-CAR` |
| 3 | [JXB2.0 机械臂开发工程](./03-jxb-robot-arm/) | `robotics/JXB2.0` |
| 4 | [ESP32 ROS2 RTOS 嵌入式实验](./04-esp32-ros2-rtos/) | `embedded/esp32ROS2RTOS` |
| 5 | [物联网智能育苗控制系统](./05-iot-seedling-control/) | `applications/iot-seedling-graduation` |
| 6 | [手部姿态视觉实验](./06-handpose-vision/) | `vision/handpose-x` |

## 项目说明

### STM32F407ZGT6 + FreeRTOS 麦克纳姆底盘

- 项目源码：[`projects/01-stm32f407-freertos-mecanum/`](./01-stm32f407-freertos-mecanum/)
- 归档原始位置：[`embedded/bugrtoscar/otherWork/STM32F407ZGT6`](../embedded/bugrtoscar/otherWork/STM32F407ZGT6/)
- 内容概要：Keil 工程、FreeRTOS、四轮控制、IMU、上位机工具
- 个人源码仓库（可检查是否有较新版本）：https://github.com/dreamcather726/bugrtoscar

### ROS-CAR 机器人开发工程

- 项目源码：[`projects/02-ros-car/`](./02-ros-car/)
- 归档原始位置：[`robotics/ROS-CAR`](../robotics/ROS-CAR/)
- 内容概要：ROS、ESP32 和机器人系统工程资料
- 个人源码仓库（可检查是否有较新版本）：https://github.com/dreamcather726/ROS-CAR

### JXB2.0 机械臂开发工程

- 项目源码：[`projects/03-jxb-robot-arm/`](./03-jxb-robot-arm/)
- 归档原始位置：[`robotics/JXB2.0`](../robotics/JXB2.0/)
- 内容概要：机械臂控制、Python、ROS 与相关实验
- 个人源码仓库（可检查是否有较新版本）：https://github.com/dreamcather726/JXB2.0

### ESP32 ROS2 RTOS 嵌入式实验

- 项目源码：[`projects/04-esp32-ros2-rtos/`](./04-esp32-ros2-rtos/)
- 归档原始位置：[`embedded/esp32ROS2RTOS`](../embedded/esp32ROS2RTOS/)
- 内容概要：ESP-IDF / CMake 嵌入式工程

### 物联网智能育苗控制系统

- 项目源码：[`projects/05-iot-seedling-control/`](./05-iot-seedling-control/)
- 归档原始位置：[`applications/iot-seedling-graduation`](../applications/iot-seedling-graduation/)
- 内容概要：物联网控制与毕业设计项目源码
- 个人源码仓库（可检查是否有较新版本）：https://github.com/dreamcather726/IoT-Based-Seedling-Control-System

### 手部姿态视觉实验

- 项目源码：[`projects/06-handpose-vision/`](./06-handpose-vision/)
- 归档原始位置：[`vision/handpose-x`](../vision/handpose-x/)
- 内容概要：Python 视觉与手部姿态处理实验

## 保护原则

1. **仅收录个人账号下或本私有归档已有且明确选择的项目源码**；组织账号下的仓库不参与。
2. 保留源码原路径、原有独立仓库及其完整 Git 历史，不删除、不强制推送、不覆盖。
3. 原归档做过敏感内容筛查与占位替换，见 [归档范围和检查记录](../docs/ARCHIVE_SCOPE.md)；新增或更新源码仍需重新检查凭据、密钥、个人数据和大文件。
4. 此处用于个人备份和工程展示。商用、开源或在闲鱼售卖前，需核对每个项目的作者归属、雇主知识产权、第三方许可证和依赖许可。
5. 本目录中重复呈现的源码与 [原归档](../docs/PROJECTS.md)共享现有 Git 对象，并不是自动更新的镜像。不要直接改两个位置后假定它们会同步。

## 其他已归档项目

原仓库还有多项嵌入式、机器人、视觉、AI、课程和工具代码，仍可在 [全部 65 个项目或版本目录](../docs/PROJECTS.md) 中查找。未逐一确认归属或独立来源的项目暂不复制到本精选目录。
