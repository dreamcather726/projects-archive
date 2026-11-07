# ROS 2 底盘驱动技术：逐页教师讲义

对应课件：`移动机器人底盘驱动技术——理论学习课件.pptx`，共 48 页。



## 第 1 页｜底盘驱动技术

这页你可以把它讲成：

> **今天沿着两条数据流看底盘：速度指令怎样送到电机，硬件反馈怎样变成 ROS 2 可以使用的状态。**

### 讲解与例子

承接前课的通信、TF、模型与仿真。导航或遥控节点给出期望速度，底盘驱动把消息编码成串口数据，下位机执行控制并回传测量。主控再组织里程计、IMU 消息与 TF。

用“让机器人向前走，再判断它是否真的移动”贯穿本课。学习目标是看懂模块职责、追踪指令与反馈、解释运动估计，并掌握接口检查方法。封面先建立整体认识，实验随对应知识点安排，优先使用官方示例。

### 板书

```text
指令：导航/遥控 → ROS 2 驱动 → 串口 → 下位机 → 电机
反馈：编码器/IMU → 下位机 → 串口 → ROS 2 消息与 TF
```

### 课堂提问

> 发布速度消息成功，能证明底盘已运动吗？

参考答案：不能，还要检查接收、串口、下位机执行、反馈与实际运动。

这页最后可以总结：

> **沿着数据流，才能把通信接口和硬件行为联系起来。**

---

## 第 2 页｜课程目录

这页你可以把它讲成：

> **这节课分为八个部分。先认识系统和 ROS 2 接口，再解释坐标、运动估计与传感器，最后看这些数据怎样通过串口协议传输。大家可以一直带着一个问题：一条速度指令怎样变成运动，运动结果又怎样反馈回来？**

### 讲解与例子

沿 PPT 的编号依次介绍八个部分，先交代每部分解决什么问题，具体公式和代码留到对应页面展开。

| 部分 | 本部分讲什么 | 希望学生能回答的问题 |
| --- | --- | --- |
| 01 系统总体架构 | 四层模型、驱动层职责、硬件平台 | 主控、驱动和下位机各负责什么？ |
| 02 ROS2 核心概念 | 节点、话题、服务、QoS、消息类型、定时器 | 速度指令与传感器数据怎样在节点间交换？ |
| 03 坐标系与 TF | REP-103、TF 树、四元数、欧拉角 | 一组位置、方向或速度相对哪个坐标系表达？ |
| 04 运动学与航位推算 | 旋转矩阵、离散积分、麦轮运动学 | 怎样由车体速度估计位置变化，全向运动需要哪些速度分量？ |
| 05 里程计与协方差 | Odometry 结构、协方差设计、统计基础 | 怎样描述估计的位姿、速度以及它们的不确定度？ |
| 06 IMU 传感器原理 | 陀螺仪、加速度计、姿态融合、定标 | IMU 测量什么，输出怎样换算和解释？ |
| 07 串口通信基础 | UART 原理、通信参数、帧结构、udev | 主控怎样找到设备，并按约定收发字节？ |
| 08 通信协议设计 | 自定义协议、环形缓冲区、CRC-8、定标 | 怎样从连续字节中识别数据帧并还原字段含义？ |

用“向前行驶一小段”的任务串起目录：先明确谁发指令、谁执行，再确定速度的单位与方向；随后讨论如何根据反馈估计运动，最后追问这些数值怎样编码、传输和校验。

提醒学生，课程讲解顺序与程序运行顺序有所不同。实际运行时，ROS 2 通信、串口收发和状态估计会配合工作；讲课时逐项拆开，便于检查每个环节。

### 板书

```text
系统：谁负责？          ROS 2：怎样交换数据？
坐标：相对哪里？        运动学：位置怎样变化？
协方差：有多大不确定度？ IMU：测到了什么？
串口：怎样传字节？      协议：字节表示什么？
```

### 课堂练习与实验衔接

让学生为下面四个现象选择优先检查的课程部分，并说明理由：

1. ROS 2 节点之间看不到速度话题。
2. 前进方向与消息中的正方向不一致。
3. 里程计位置逐渐偏离实际位置。
4. 串口已经读到字节，但无法解析出有效数据。

参考答案：分别优先检查 ROS 2 通信、坐标约定、运动估计与传感器误差、串口协议。实际故障可能跨越多个部分，选择优先检查点后，还需用数据验证。

本页先建立排查思路，实际操作安排在对应知识点处。ROS 2 通信与 TF 优先使用官方示例，实验写清操作步骤、预期现象和验收方法；项目专用协议则结合相应协议定义讲解。

### 课堂提问

> 为什么“串口能读到数据”还不足以说明底盘驱动正常？

参考答案：还要确认帧边界、校验、字段类型、缩放、单位、坐标方向，以及转换后的 ROS 2 消息是否正确。收到字节只是其中一个环节。

### 教师备注

图片页脚写“49 页正文”，当前项目中的课件文件实际为 48 页，讲义以实际页面顺序编号。本页目录中的 CRC-8 按图片记录，具体校验算法和参数在对应协议页面结合资料核实。

这页最后可以总结：

> **八个部分共同解释底盘的数据链路：每个环节都有明确含义，也都需要对应的检查方法。**

---

## 第 3 页｜移动机器人系统总体架构

这页你可以把它讲成：

> **我们先把机器人拆成几个有明确职责的层次。比如让配送机器人去取货：任务模块决定去哪里，算法模块计算怎样走，驱动把运动要求转换成硬件接口，下位机和电机完成执行。理解这些分工，才能说明底盘驱动在整个系统里负责什么。**

### 讲解与例子

这是第一章的引入页，围绕 PPT 的三个问题建立整体认识：机器人由哪些层次组成，驱动层承担什么职责，以及“翻译官”具体翻译什么。

**第一个问题：一个完整的移动机器人由哪些层次构成？**

本课采用四层模型理解系统：应用层组织任务，算法层进行感知、定位、规划与运动控制，驱动层适配设备接口，硬件层完成测量和执行。分层描述的是职责，不能简单把一个进程、一块电路板或一台电脑等同于一层。

以“向前行驶一小段”为例，任务模块提出运动任务，控制模块计算期望速度，驱动编码并发送指令，下位机控制电机，同时采集反馈。各模块共同完成任务，但处理的信息不同。

**第二个问题：驱动层在其中扮演什么角色？**

驱动连接 ROS 2 接口与设备接口。下行时，它接收运动要求，按约定检查、换算和编码后发送给下位机；上行时，它解析反馈，再组织成 ROS 2 消息。连接状态与通信异常也需要在驱动设计中处理。

**第三个问题：为什么说驱动层是“翻译官”？**

ROS 2 一侧使用有字段和类型的消息，设备一侧使用具体协议约定的字节。双方需要对齐字段含义、单位、正负方向与数据格式。这个比喻强调双向适配：既要把要求传下去，也要把测量结果正确传上来。

例如，ROS 2 接口约定用 m/s 表示线速度，下位机协议可能使用某种定标整数。驱动必须按协议换算，不能把消息中的浮点数直接当成整数原始值。具体比例与字节排列由实际协议决定，本页不预设。

### 分层设计的价值

明确接口后，上层可以通过统一的消息使用不同底盘，底层可以独立处理设备差异。排查问题时也能沿接口逐步检查：有没有产生运动要求，驱动有没有收到，下位机有没有响应，反馈是否合理。

更换硬件仍需核对运动能力、标定参数、坐标关系和协议，不能认为接口名称一致就一定可以直接替换。

### 板书

```text
应用层：完成什么任务？
算法层：怎样感知、定位、规划与控制？
驱动层：怎样把消息与设备协议互相转换？
硬件层：怎样测量和执行？

下行：运动要求 → ROS 2 消息 → 驱动编码 → 下位机执行
上行：硬件测量 → 协议反馈 → 驱动解析 → ROS 2 状态消息
```

### 课堂练习：追踪一次运动任务

请学生把下面四项工作分别放入四层模型，并画出指令与反馈的方向：

1. 接收“把物品送到指定地点”的任务。
2. 根据路径偏差计算期望速度。
3. 把速度消息编码成下位机约定的数据帧。
4. 驱动电机转动并采集编码器信号。

参考答案：依次对应应用层、算法层、驱动层和硬件层。这里“驱动电机”指电机驱动电路与下位机执行动作，区别于 ROS 2 的软件驱动层。

验收：学生能指出速度要求从哪里产生、经过哪个接口下发，以及测量反馈怎样返回。若把“发送成功”直接当成“运动完成”，请其补上反馈检查环节。

### 课堂提问

> 如果导航模块已经发出了速度要求，但串口设备打不开，应先检查哪一部分？

参考答案：先检查驱动与设备连接，包括设备路径、连接状态、权限和占用情况。可同时确认驱动是否收到消息，沿链路缩小范围，不应直接把问题归因于路径规划。

> 上层为什么通常不需要知道编码器数据位于串口帧的第几个字节？

参考答案：驱动负责解析具体协议，上层通过约定的状态消息使用反馈；但上层仍需了解消息的单位、坐标系、时间与测量含义。

这页最后可以总结：

> **先分清模块职责，再沿指令与反馈检查接口。底盘驱动承担的，就是 ROS 2 与硬件协议之间的双向适配。**

---

## 第 4 页｜机器人系统的四层模型

这页你可以把它讲成：

> **上一页我们提出了分层，这一页把每层的职责落到具体模块上。读图时先从上往下看：任务怎样变成运动要求；再从下往上看：测量怎样变成可以使用的状态。本课重点是中间的驱动层，它连接 ROS 2 接口与底盘设备。**

### 讲解与例子

**1. 应用层：面向用户的任务与功能。**

任务调度、语音交互、远程控制等功能，负责把用户需求组织成系统任务。例如用户说“把物品送到门口”，应用层需要识别目标、发起任务并显示执行状态。

PPT 把“视觉识别”列在应用层，可以按“面向用户的识别功能”理解；具体的视觉计算也可能属于算法模块。分层看职责，一个功能可以由多层共同完成。

**2. 算法层：根据环境与状态计算行动。**

沿图中的例子说明：SLAM 处理建图与定位，AMCL 在已有地图中进行定位，导航与路径跟踪根据目标和当前状态计算运动要求。这里先讲输入、输出与职责，不展开各算法的实现。

例如，“去门口”是任务要求，控制模块进一步算出的前进速度与转向速度才是底盘可以执行的运动要求。

**3. 驱动层：本课的重点。**

| 职责 | 处理的内容 | 需要检查什么 |
| --- | --- | --- |
| `cmd_vel` 下行 | 接收期望速度，按设备协议换算、编码与发送 | 是否收到新指令，单位、方向和范围是否符合约定 |
| 传感器上行 | 解析编码器、IMU、电池等反馈，组织 ROS 2 消息 | 字段、缩放、坐标系和时间是否正确 |
| 里程计推算 | 按本课实现，由反馈估计位姿与速度 | 使用的是反馈还是指令，时间间隔与坐标变换是否合理 |
| 设备管理 | 打开、关闭设备，处理连接与通信异常 | 设备路径、权限、占用、超时与恢复状态 |

里程计也可以由独立的状态估计模块提供；本图将其放在驱动层，是本课的实现分工。后续使用融合模块时，还应明确谁负责发布相应的里程计与 TF。

**4. 硬件层：测量与执行。**

电机把控制作用转为运动，编码器测量轴的运动，IMU 提供惯性测量，电池提供能量并可配合采样电路获取状态。MCU 下位机运行固件，连接这些设备并执行约定的控制与通信逻辑。

注意两个“驱动”的区别：图中的驱动层指 ROS 2 一侧的软件适配；硬件层的电机驱动器指控制电机电流或电压的电路。两者相互配合，但职责不同。

### 层间接口怎样解耦

上层使用约定的消息表达运动要求和读取状态，不需要自行解析设备的字节偏移。下层可以调整设备通信实现，但应保持接口含义一致。

接口除了话题名称，还包括消息类型、单位、坐标系、时间、QoS 与异常处理约定。不能只检查都叫 `/odom`，就认定两个底盘可以直接互换。

PPT 中“只与相邻层交互”表达的是主业务链路的设计原则。诊断工具可以跨层观察；各算法也按需要订阅数据，并非每个算法都必须读取 `odom`、`imu`、`battery` 全部话题。例如，电池信息还常供任务管理或状态显示使用。

### 板书

```text
应用层：去哪里、做什么任务
   ↓ 任务目标                         ↑ 执行状态
算法层：根据环境与状态计算运动要求
   ↓ cmd_vel                          ↑ odom、imu 等
驱动层：编码/解析、里程计、设备管理
   ↓ 设备控制帧                       ↑ 测量反馈帧
硬件层：MCU、电机、电机驱动器、编码器、IMU、电池
```

### 命令行实验：用官方示例观察指令与状态接口

**实验目标：**通过官方 turtlesim 看清“发送运动要求”和“读取状态”这两种接口职责。该示例只演示接口分工，不包含本项目的串口协议、真实驱动或完整四层系统。

来源：ROS 2 Humble 官方教程 [Using turtlesim, ros2, and rqt](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Beginner-CLI-Tools/Introducing-Turtlesim/Introducing-Turtlesim.rst) 与 [Understanding topics](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Beginner-CLI-Tools/Understanding-ROS2-Topics/Understanding-ROS2-Topics.rst)。

**准备条件：**已安装 ROS 2 Humble 的 Ubuntu 22.04 或 WSL Ubuntu 22.04，并具有可用的图形环境。以下命令在 Ubuntu Bash 中执行，目录不限；实验不连接实体机器人。

按需安装示例包：

```bash
sudo apt update
sudo apt install ros-humble-turtlesim
```

准备 A、B、C 三个终端，每个终端先执行：

```bash
source /opt/ros/humble/setup.bash
echo "${ROS_DOMAIN_ID:-0}"
```

确认三个终端的 Domain ID 相同，并停止之前的同名示例。持续命令占用当前终端，按下列终端分工执行。

**A：启动仿真节点并保持运行。**

```bash
ros2 run turtlesim turtlesim_node
```

**B：启动键盘控制节点。焦点保持在 B，按方向键。**

```bash
ros2 run turtlesim turtle_teleop_key
```

**C：查询接口，再观察速度要求。**

```bash
ros2 topic list -t
ros2 topic info /turtle1/cmd_vel --verbose
ros2 topic echo /turtle1/cmd_vel
```

预期查询结果显示 `/turtle1/cmd_vel` 的类型为 `geometry_msgs/msg/Twist`，键盘节点发布，仿真节点订阅。在 B 按方向键时，C 出现速度消息，窗口中的海龟移动。

在 C 按 `Ctrl+C` 停止观察指令，再执行：

```bash
ros2 topic echo /turtle1/pose
```

回到 B 控制海龟，观察 C 中的位姿变化。这里的反馈类型是 `turtlesim/msg/Pose`，与实体底盘使用的 `nav_msgs/msg/Odometry` 不同。

**验收：**学生能够指出指令发布者、指令接收者和状态发布者，并解释为何读取速度指令不能代替检查执行后的状态。进一步说明，如果换成实体底盘，还需要补上哪些设备通信与反馈处理环节。

**排查与结束：**无窗口时检查图形环境或 WSLg；按键无效时检查焦点是否在 B；收不到消息时检查节点是否运行、Domain ID 是否一致，以及是否实际按键。结束时 C 按 `Ctrl+C`，B 按 `q`，A 按 `Ctrl+C`。

### 课堂提问

> 更换电机控制板后，导航模块一定要跟着修改串口解析吗？

参考答案：串口适配应由驱动或相应设备模块处理。若上层接口含义保持兼容，导航通常无需了解新协议；但运动能力、时延、坐标关系与参数仍需核对。

> 硬件层有 MCU 和固件，为什么仍叫“硬件层”？

参考答案：这里按系统边界划分，把下位机及其底层控制作为设备侧整体。四层模型是本课的职责划分方式，不表示这一层只有电路而没有软件。

这页最后可以总结：

> **四层通过明确的接口协作。驱动层正确传递要求、解释反馈并管理设备，上层才能可靠地使用底盘能力。**

---

## 第 5 页｜驱动层核心职责与硬件平台

这页你可以把它讲成：

> **上一页确定了驱动层的位置，这一页进一步看它每天要做什么。我们从收到一条速度要求开始：驱动把要求发给下位机，再接收反馈、估计运动，并处理设备连接。右边的硬件表告诉我们，这些工作具体发生在哪些设备之间。**

### 讲解与例子

**1. 指令下行：把速度要求转换成设备能够理解的数据。**

驱动订阅 `cmd_vel`，读取约定的速度分量，检查范围与有效性，再按协议进行定标、编码和发送。下位机解析控制帧后，才进入电机控制环节。

例如，上层给出前进速度 0.2 m/s。驱动需要知道下位机字段的单位、缩放、数据类型与字节序，才能正确编码。这里的 0.2 是期望值，串口写入成功还需要结合执行反馈判断结果。

**2. 数据上行：把设备反馈转换成 ROS 2 消息。**

课件采用轮询下位机的描述，即主控按约定请求或读取数据，再解析成状态消息。另一些设备可能主动上报，接收方式取决于实际协议。

以编码器、IMU 和电池数据为例，驱动不仅拆出数值，还要核对单位、方向、时间与有效性。某次读取没有得到完整数据时，需要继续接收或处理超时，不能把缺失的反馈当作新的零值测量。

**3. 里程计推算：由反馈估计运动，并发布相应 TF。**

车体速度与航向共同决定位置变化。驱动按本课的分工计算里程计，并广播车体相对里程计参考系的变换。这里使用的速度应是反馈或估计的实际速度，不能直接用 `cmd_vel` 代替。

反馈也存在误差，例如轮子空转时编码器仍可能变化。因此里程计是运动估计，需要说明其不确定度；对应消息与 TF 还应保持坐标和时间的一致。

**4. 设备管理：保证连接与异常状态可处理。**

设备管理包括打开和关闭串口、识别设备路径、处理权限与占用，以及记录超时、断连等异常。发生通信中断时，还需要按系统约定处理过期指令与反馈，不能继续把旧状态当成新数据。

`udev` 可用于设备命名和权限规则；它不会自动修复协议不匹配或物理接线问题。

### 硬件平台逐项说明

以下参数按本页课件记录，实机操作前应与设备和配置核对。

| 部件 | 课件给出的型号或参数 | 讲解重点 |
| --- | --- | --- |
| 主控 | RK3588 / Orange Pi 5 / Python 3.10 | 主控运行 ROS 2 与驱动程序；芯片、板卡与软件版本是不同层面的信息 |
| 底盘 | NanoOmni 全向底盘，麦轮直径 64 mm | 按该直径计算轮半径为 0.032 m；轮速换算还要确认减速比、编码器定义及换算位置 |
| 下位机 | `move_base` 控制板，MCU，串口波特率 115200 | 下位机负责设备侧控制与通信；波特率之外还需约定数据位、校验位和停止位 |
| 传感器 | 六轴 IMU、电机编码器、电池采样 | 分别提供惯性信息、轴运动信息和电池状态；实际输出字段需查看协议 |
| 通信 | UART，设备节点 `/dev/move_base` | 这是项目约定的设备路径，应确认它实际指向哪个串口设备 |

主控与下位机可以分别运行不同软件，不必使用相同编程语言；双方通过明确的通信协议协作。主控 Python 版本也不代表下位机使用 Python。

### 怎样理解“串口协议黑盒”

“黑盒”表示 ROS 2 一侧通常通过协议使用底盘能力，不必直接实现电机 PWM 或底层电流控制。例如，下位机若已经提供车体速度反馈，主控可以按接口读取，无需重新处理每一个编码器脉冲。

但主控必须知道协议输入输出的含义。如果协议提供的是轮速或脉冲计数，主控仍可能需要轮径、减速比或编码器分辨率才能换算。黑盒抽象隐藏的是内部实现，接口定义与必要物理参数仍需明确。

PPT 中“更换底盘只需修改协议解析部分”需要加上前提：上层接口与运动能力兼容，并完成相关参数适配。实际更换时，下行编码、静态安装关系、里程计计算、量程和超时策略也可能需要调整。

### 板书

```text
下行：cmd_vel → 检查与换算 → 协议编码 → 串口写入
上行：串口读取 → 数据解析 → 状态消息
估计：反馈速度 + 航向 + 时间间隔 → 里程计与 TF
管理：设备路径、权限、连接、超时、退出清理

黑盒内部：电机与控制实现
黑盒接口：字段、单位、方向、时序、异常语义
```

### 命令行实验：使用官方串口回环机制检查收发与关闭

**实验目标：**观察打开、写入、读取、超时和关闭这几项设备操作。使用 pySerial 官方提供的 `loop://` 软件回环，不需要实体底盘，也不会向 `/dev/move_base` 发送数据。

来源：[pySerial 官方 loop:// 说明](https://pyserial.readthedocs.io/en/latest/url_handlers.html#loop)与[官方串口 API](https://pyserial.readthedocs.io/en/latest/pyserial_api.html)。下面是基于这些接口编写的最小课堂示例；测试字节不是 NanoOmni 控制协议帧。

**准备条件：**Ubuntu Bash、Python 3。执行目录不限，一个终端即可，不需要启动 ROS 2 节点。

```bash
sudo apt update
sudo apt install python3-serial
python3 -c "import serial; print(serial.__version__)"
```

执行收发实验：

```bash
python3 - <<'PY'
import serial

payload = b'course-demo'
with serial.serial_for_url('loop://', baudrate=115200, timeout=0.1) as port:
    print('opened:', port.is_open)
    written = port.write(payload)
    received = port.read(len(payload))
    print('written:', written)
    print('received:', received)
    print('match:', received == payload)
    print('empty read:', port.read(1))
print('closed:', not port.is_open)
PY
```

**预期结果：**`opened` 为 `True`，写入数量为 11，收到 `b'course-demo'`，`match` 为 `True`。没有新的数据时，最后一次读取等待超时后返回 `b''`；退出 `with` 后，`closed` 为 `True`。程序自行结束。

**验收：**学生指出哪一行打开连接、哪一行写入、哪一行读取，以及连接何时关闭；说明超时返回空字节为什么不代表设备反馈了数值零。

**结果边界：**软件回环验证的是本段程序的收发与资源管理，没有验证真实 UART 的波特率匹配、接线、权限、固件应答或电机执行。回环收到的字节就是自己发出的字节，不能当作硬件反馈。

**常见问题：**出现 `ModuleNotFoundError: No module named 'serial'` 时，检查 `python3-serial` 是否安装到当前使用的 Python 环境；如果修改了测试内容，应按新的字节长度读取，并相应调整预期数量。

### 课堂提问

> `/dev/move_base` 文件存在，是否就说明驱动能正常控制底盘？

参考答案：还需确认它指向正确设备、具备访问权限、没有冲突占用，并且通信参数和协议匹配。设备路径存在只是检查的一步。

> 为什么主控不需要知道 PWM 频率，却仍可能需要知道轮径？

参考答案：PWM 控制通常封装在下位机；如果主控需要把反馈轮速换算为线速度，轮径就属于接口使用所需的参数。若换算已在下位机完成，应确认反馈单位与标定是否正确。

> 回环读到了刚才发送的字节，能否证明下位机执行了指令？

参考答案：不能。该实验使用软件回环，没有下位机参与，只验证本地收发过程。

这页最后可以总结：

> **底盘驱动要把指令传对、把反馈读懂、把运动估计解释清楚，并管理设备状态。协议黑盒提供了实现边界，正确使用它仍需完整的接口约定。**

---

## 第 6 页｜ROS2 核心概念

这页你可以把它讲成：

> **上一章讲清了驱动的职责，现在看它怎样在 ROS 2 中实现。我们带着三个问题进入这一章：节点之间怎样交换数据，传输策略怎样选，以及速度、里程计和 IMU 消息分别表达什么。把这三个问题连起来，就能开始读懂驱动节点的接口。**

### 讲解与例子

这是第二章的引入页，围绕图片中的三个问题复习核心概念，具体编程细节留到对应知识点展开。

**第一个问题：节点之间如何通信，话题和服务有什么区别？**

节点是承担某项职责的运行对象，一个节点可以同时发布、订阅话题，也可以提供或调用服务。

话题采用发布与订阅模式，适合持续更新的数据。例如控制节点发布速度要求，底盘驱动订阅；驱动持续发布运动状态，其他节点按需要订阅。话题本身不为每条指令提供请求与响应式的执行结果。

服务采用请求与响应模式，适合一次查询或触发操作，例如读取某项设备状态、执行一次复位。是否提供这些服务由具体驱动决定。客户端可以同步等待，也可以异步处理响应，因此不能把“服务”直接等同于“必须阻塞调用”。

**第二个问题：为什么课件建议速度指令用 reliable，传感器数据用 best effort？**

Reliable 在通信层支持可靠交付和必要的重传；best effort 允许样本丢失，不要求补齐每个样本。对于持续产生新测量的传感器，有时可以接受少量丢帧，优先处理较新的状态。

但这是常见的选型思路，需要结合实际系统判断。速度指令通常也会周期性更新，过期的控制要求不应一直执行；传感器丢失过多也会影响估计。使用 reliable 不能证明电机已执行，使用 best effort 也不能免除对数据连续性的检查。

实际配置要兼顾发布与订阅端是否匹配、时延、队列深度及断流处理。驱动与下位机还需按约定处理指令超时。相关概念见 [ROS 2 Humble 官方 QoS 文档](https://github.com/ros2/ros2_documentation/blob/humble/source/Concepts/Intermediate/About-Quality-of-Service-Settings.rst)。

**第三个问题：Twist、Odometry、Imu 中分别装了什么？**

| 消息类型 | 主要内容 | 与底盘驱动的关系 |
| --- | --- | --- |
| `geometry_msgs/msg/Twist` | 三维线速度 `linear` 与角速度 `angular` | 常用于表达速度要求；线速度用 m/s，角速度用 rad/s，参考系需由接口约定 |
| `nav_msgs/msg/Odometry` | 时间、坐标系、估计位姿与速度，以及相应协方差 | 提供运动估计；pose 在 `header.frame_id` 中表达，twist 在 `child_frame_id` 中表达 |
| `sensor_msgs/msg/Imu` | 时间与坐标系、姿态四元数、角速度、线加速度及各自协方差 | 传递惯性测量或设备提供的姿态估计；角速度用 rad/s，加速度用 m/s² |

Twist 本身没有 Header，不能仅凭消息内容知道它属于哪个时刻或坐标系。Imu 也不保证每台设备都提供有效姿态；缺失的数据应按消息定义标记，不能把默认值当作测量结果。

本页先建立“速度要求、运动估计、惯性信息”三者的区别。定时器用于周期性触发工作，回调由执行器调度；订阅、采样与发布如何配合，需要结合后续代码理解。

### 板书

```text
话题：发布/订阅，适合持续更新的数据
服务：请求/响应，适合一次操作
QoS：怎样交付数据，双方能否匹配

Twist：要求怎样运动
Odometry：估计运动到了哪里、当前怎样运动
Imu：惯性测量与可用的姿态估计
```

### 命令行实验：用官方 turtlesim 区分话题与服务

**实验目标：**在同一个官方示例中观察持续话题和一次服务调用，再查看三种底盘常用消息的定义。这里不启动实体底盘，也不把海龟位姿当作真实里程计。

来源：[Humble 官方服务教程](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Beginner-CLI-Tools/Understanding-ROS2-Services/Understanding-ROS2-Services.rst)。消息字段可对照官方 [Odometry 定义](https://github.com/ros2/common_interfaces/blob/humble/nav_msgs/msg/Odometry.msg)与 [Imu 定义](https://github.com/ros2/common_interfaces/blob/humble/sensor_msgs/msg/Imu.msg)。

**准备条件：**Ubuntu 22.04 或 WSL Ubuntu 22.04，已安装 ROS 2 Humble，图形显示可用。以下命令在 Ubuntu Bash 中执行，目录不限。缺包时先安装：

```bash
sudo apt update
sudo apt install ros-humble-turtlesim ros-humble-geometry-msgs \
  ros-humble-nav-msgs ros-humble-sensor-msgs ros-humble-std-srvs
```

准备 A、B、C 三个终端，每个终端先执行：

```bash
source /opt/ros/humble/setup.bash
echo "${ROS_DOMAIN_ID:-0}"
```

各终端的 Domain ID 应一致。若第 4 页的同一套 turtlesim 仍在运行，可直接复用；否则按下面步骤启动，避免重复运行同名节点。

**A：启动仿真节点。**

```bash
ros2 run turtlesim turtlesim_node
```

**B：启动键盘控制，保持焦点在 B，按方向键移动海龟并画出轨迹。**

```bash
ros2 run turtlesim turtle_teleop_key
```

**C：观察话题接口和一条状态消息。**

```bash
ros2 topic info /turtle1/cmd_vel --verbose
ros2 topic echo /turtle1/pose --once
```

预期第一条命令显示速度话题的类型、端点与 QoS；第二条命令收到一条仿真位姿后退出。记录查询到的 reliability，说明 QoS 配置属于具体端点。

**C：查看并调用一次清除轨迹服务。调用前停止在 B 按键。**

```bash
ros2 service type /clear
ros2 interface show std_srvs/srv/Empty
ros2 service call /clear std_srvs/srv/Empty "{}"
```

预期 `/clear` 的类型为 `std_srvs/srv/Empty`，请求与响应均没有业务字段。调用后已有轨迹清除，海龟位置不因该服务而复位。空响应仍是一次请求与响应通信，不等于“没有服务响应”。

**C：查看本页三种消息的字段。**

```bash
ros2 interface show geometry_msgs/msg/Twist
ros2 interface show nav_msgs/msg/Odometry
ros2 interface show sensor_msgs/msg/Imu
```

**验收：**学生能够解释话题观察与服务调用的区别，指出 Twist 的两类速度向量、Odometry 的两个坐标系字段，以及 Imu 的角速度和加速度字段。查看接口定义不需要对应话题正在发布；本实验没有产生真实 `/odom` 或 `/imu` 数据。

**排查与结束：**服务找不到时检查 A 是否运行及名称是否正确；轨迹看起来没有变化时确认调用前已有轨迹且 B 没有继续画线；找不到消息类型时检查依赖与环境加载。结束时 B 按 `q`，A 按 `Ctrl+C`。

### 课堂提问

> 已经收到一条 Twist，能否据此填写“机器人已前进 1 m”？

参考答案：不能。Twist 表达速度，若用于 `cmd_vel` 则表示运动要求；位移需要反馈或运动估计支持，还涉及时间与坐标系。

> 将所有话题都改成 reliable，就能解决通信和控制问题吗？

参考答案：不能。还需检查端点匹配、数据时效、队列与系统负载，并分别验证设备通信和执行结果。

这页最后可以总结：

> **读驱动接口时，先判断通信模式，再检查传输策略，最后核对消息的字段、单位与参考系。**

---

## 第 7 页｜计算图：四种通信方式

这页你可以把它讲成：

> **这一页把节点之间的接口放到一张计算图里。连续更新速度和状态，用话题；发起一次查询或操作，用服务；配置某个节点，用参数；提交需要持续执行、观察进度的任务，用动作。同一个底盘节点可以同时使用这些接口，关键是根据操作的含义选择。**

### 讲解与例子

计算图描述运行中的节点及其通信关系。读图时先找到节点，再看它发布什么、订阅什么、提供哪些服务和动作，以及有哪些参数。一个节点可以同时承担多个通信角色。

| 方式 | 交互模式 | 怎样获得数据或结果 | 本项目中的典型用途 |
| --- | --- | --- | --- |
| 话题 | 发布与订阅 | 持续接收消息；没有逐条对应的业务响应 | 下发速度要求，上报里程计、IMU 与电池状态 |
| 服务 | 请求与响应 | 针对一次请求获得响应；客户端可同步等待或异步处理 | 一次查询或触发操作，具体接口由驱动提供 |
| 参数 | 节点配置的键值读写 | 读取配置值，或获得设置是否被接受的结果 | 串口名、波特率、发布频率等配置 |
| 动作 | 目标、反馈与结果 | 获得目标处理状态、执行反馈与最终结果，可请求取消 | 导航到目标点等需要跟踪执行过程的任务 |

**话题：持续交换数据。**

话题可以有多个发布者和多个订阅者。发布速度消息没有直接返回“电机执行成功”，需要通过状态、诊断或实际观察检查结果。多个控制来源同时发布速度还可能互相干扰，因此系统应明确指令来源与仲裁方式。

**服务：一次请求对应一次响应。**

PPT 把服务标成“同步”，应理解为图中列举了一种等待响应的使用方式。服务本身不要求客户端必须阻塞调用。同步等待可能因服务端不响应而长期占用调用线程；异步调用可以避免直接阻塞，但仍需处理未完成请求、超时与错误。参见[官方同步与异步服务说明](https://github.com/ros2/ros2_documentation/blob/humble/source/How-To-Guides/Sync-Vs-Async.rst)。

**参数：属于具体节点的配置。**

读取波特率时，要先说明读取哪个节点的哪个参数。两个节点即使具有同名参数，也各自维护取值。远程参数读写通常通过该节点的参数服务完成；是否允许运行时修改、修改后是否重新打开串口，需要节点代码支持。

**动作：管理有执行过程的任务。**

以导航为例，目标是到达某个位置，反馈用于报告执行进展，结果用于说明任务怎样结束。动作建立在话题和服务之上；目标接受、持续反馈和任务完成是不同阶段，接受目标不等于已经完成。取消也是请求，服务端需要按其实现处理。

因此，这四项是面向使用者的接口分类，不是四套完全独立的底层传输机制。

### 对照本项目的数据流

```text
导航或手柄
    │ 速度要求：cmd_vel
    ▼
base_control 节点
    ├─ 里程计 odom：课件标注 50 Hz
    ├─ 惯性信息 imu：课件标注 50 Hz
    ├─ 电池状态 battery：课件标注 10 Hz
    └─ 动态 TF：odom → base_footprint
```

50 Hz 对应约 20 ms 的更新间隔，10 Hz 对应约 100 ms。这些是课件给出的案例值，实际话题名称、频率与发布来源要在运行系统中查询，不能只凭图认定已经达到。

TF 中的 `odom` 是坐标系名称，数据流中的 `odom` 是话题名称，两者含义不同。TF 关系通过消息广播；发布了里程计话题，也不意味着对应的 TF 一定同时存在。

### 板书

```text
持续更新 → 话题           一次请求 → 服务
节点配置 → 参数           目标、进度、结果 → 动作

消息发出 ≠ 设备执行成功
参数名称相同 ≠ 属于同一个配置对象
目标接受 ≠ 任务完成
```

### 命令行实验：在官方 turtlesim 中观察四类接口

**实验目标：**在同一套官方示例中找到四类接口，并分别完成消息观察、服务调用、参数读写和动作执行。

来源：ROS 2 Humble 官方教程[话题](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Beginner-CLI-Tools/Understanding-ROS2-Topics/Understanding-ROS2-Topics.rst)、[服务](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Beginner-CLI-Tools/Understanding-ROS2-Services/Understanding-ROS2-Services.rst)、[参数](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Beginner-CLI-Tools/Understanding-ROS2-Parameters/Understanding-ROS2-Parameters.rst)与[动作](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Beginner-CLI-Tools/Understanding-ROS2-Actions/Understanding-ROS2-Actions.rst)。

**准备条件：**Ubuntu 22.04 或 WSL Ubuntu 22.04，ROS 2 Humble，图形环境可用。命令在 Ubuntu Bash 执行，目录不限。缺少示例包时安装：

```bash
sudo apt update
sudo apt install ros-humble-turtlesim
```

准备 A、B、C 三个终端，每个终端先执行：

```bash
source /opt/ros/humble/setup.bash
echo "${ROS_DOMAIN_ID:-0}"
```

确认 Domain ID 相同。可以复用上一页仍在运行的同一套 turtlesim；若已停止，再启动 A、B，不重复启动同名节点。

**A：运行仿真。**

```bash
ros2 run turtlesim turtlesim_node
```

**B：运行键盘控制。**

```bash
ros2 run turtlesim turtle_teleop_key
```

**C：先列出计算图中的接口。**

```bash
ros2 node list
ros2 node info /turtlesim
ros2 topic list -t
ros2 service list -t
ros2 param list /turtlesim
ros2 action list -t
```

预期能找到 `/turtle1/cmd_vel` 话题、`/spawn` 服务、背景颜色参数，以及 `/turtle1/rotate_absolute` 动作。注意同一个 `/turtlesim` 节点承担了多种接口角色。

**① 观察话题：C 运行，B 按方向键。**

```bash
ros2 topic echo /turtle1/cmd_vel
```

预期出现 Twist 消息。C 按 `Ctrl+C` 停止观察后，再执行后续步骤；随后先停止在 B 输入控制按键。

**② 调用服务：C 请求生成一只新海龟。**

```bash
ros2 interface show turtlesim/srv/Spawn
ros2 service call /spawn turtlesim/srv/Spawn "{x: 2.0, y: 2.0, theta: 0.0, name: ''}"
```

预期窗口出现新海龟，响应包含其名称。空名称要求服务端分配名称，因此不要把返回值固定写成某个名字。对照接口中 `---` 上方的请求字段与下方的响应字段。

**③ 读写参数：C 查询所属节点，再修改背景红色分量。**

```bash
ros2 param get /turtlesim background_r
ros2 param set /turtlesim background_r 150
ros2 param get /turtlesim background_r
ros2 param list /teleop_turtle
```

预期设置成功，查询值为 150，背景颜色随之改变；键盘节点的参数列表与仿真节点不同。本次设置仅影响当前节点会话。参数的运行时效果取决于节点实现，不可据此推断修改任意底盘参数都会立即生效。

**④ 执行动作：C 提交旋转目标，B 保持无按键输入。**

```bash
ros2 interface show turtlesim/action/RotateAbsolute
ros2 action send_goal /turtle1/rotate_absolute turtlesim/action/RotateAbsolute "{theta: 1.57}" --feedback
```

预期看到目标接受信息、旋转过程中的 `remaining` 反馈，以及结束时的 `delta` 结果与 `SUCCEEDED` 状态。角度单位为弧度；如果当前方向已接近目标，执行可能很快，可改成 `theta: -1.57` 再观察。接口定义中的两条 `---` 分隔目标、结果和反馈，定义顺序与运行时的到达顺序需要分别理解。

**验收：**学生各举出本实验的一类接口，并说明它属于持续数据、一次操作、节点配置还是可跟踪的执行任务；指出服务响应、参数设置结果和动作最终结果分别表达什么。

**排查与结束：**找不到接口时检查 A、B 是否运行、节点名和 Domain ID；参数设置失败时看返回原因；动作执行异常时先停止键盘输入并查看 A 的日志。C 若仍在执行持续观察，可按 `Ctrl+C` 结束；实验完成后 B 按 `q`，A 按 `Ctrl+C`。

本实验只演示官方示例的通信接口，没有创建 `base_control`、项目里程计或硬件 IMU 数据，也没有验证图中标注的发布频率。

### 课堂提问

> 为什么发布 `cmd_vel` 后，还需要检查里程计或其他反馈？

参考答案：话题发布没有逐条对应的执行响应。驱动可能未收到、通信可能失败、设备也可能无法按要求运动；状态与实际观察才能帮助判断执行情况。

> 两个底盘节点都声明了 `baudrate`，修改其中一个会自动修改另一个吗？

参考答案：不会。参数属于具体节点；除非系统额外实现联动，否则各节点独立管理配置。

> 导航动作已经接受目标，机器人是否已经到达？

参考答案：还没有这个结论。需要观察执行反馈及最终结果，并结合任务约定判断。

这页最后可以总结：

> **读计算图要同时看接口的使用方式和业务含义：数据怎样更新、配置属于谁、一次操作怎样响应、任务怎样结束。**

---

## 第 8 页｜话题通信与 QoS 服务质量

这页你可以把它讲成：

> **话题名和消息类型相同，还要检查双方怎样交付和接收数据。QoS 就是这部分约定。本页先看两个维度：可靠性决定如何对待样本丢失，队列深度影响暂存多少样本。选择时既要考虑能否收到，也要考虑收到的数据是否仍然及时。**

### 讲解与例子

**1. Reliability：可靠性。**

`RELIABLE` 支持可靠交付和必要的重传，`BEST_EFFORT` 不要求补齐每个丢失样本。PPT 用 TCP 与 UDP 帮助理解可靠性差异，但这是行为类比，不能据此认为 reliable 就是 TCP；ROS 2 的可靠交付也可以基于 UDP 实现。

可靠性交付属于通信环节，不代表接收回调一定及时，更不代表 MCU 或电机已经执行。断连、进程退出、资源限制和旧指令处理仍需由系统设计考虑。

**2. Depth：队列深度。**

深度与 History 配合使用。`KEEP_LAST` 保留最近 N 个样本，depth 指定 N；发布端和订阅端都可以配置历史缓存，并非只有订阅端有队列。`KEEP_ALL` 则受资源限制约束，不能直接按同样的 depth 解释。

队列较大可以容纳更多暂时来不及处理的数据，也可能积累旧样本；较小的队列更容易在积压时覆盖旧样本，但不能单独保证低时延。发布速率、回调耗时、网络与执行器调度都影响实际表现。

例如，50 Hz 数据每隔约 20 ms 更新一次。如果订阅者处理不过来，持续增大队列不会提高处理能力，需要同时检查处理耗时与数据时效要求。

### 本项目 QoS 选择怎样理解

| 话题 | 相对驱动的方向 | 图片中的配置 | 讲解时需要补充的条件 |
| --- | --- | --- | --- |
| `cmd_vel` | 订阅 | `system_default`，图中按可靠交付说明 | system default 依赖中间件默认值，不能仅凭名字认定可靠；控制端与驱动端还要匹配，并处理指令超时 |
| `odom`、`imu` | 发布 | `sensor_data`，图中标注 50 Hz | 尽力交付适用于可容忍一定丢帧的场景；下游若请求 reliable，将无法与 best-effort 发布端匹配 |
| `battery` | 发布 | `sensor_data`，图中标注 10 Hz | 应评估状态丢失、过期及晚加入订阅者的需求；更新较慢并不自动决定必须使用哪一种可靠性 |

Humble 的官方 `sensor_data` 预设采用 `KEEP_LAST`、depth 为 5、`BEST_EFFORT` 和 `VOLATILE`。`system_default` 与明确的默认发布订阅 profile 是不同预设：前者使用底层实现的默认策略，不能与后者混为一谈。预设定义见[官方 RMW QoS 源码](https://github.com/ros2/rmw/blob/humble/rmw/include/rmw/qos_profiles.h)。

本页列的是项目案例中的选型，需要用运行端点验证；50 Hz、10 Hz 是课件值，不是 QoS 自动保证的发布频率。

### 可靠性匹配规则

订阅者提出接收要求，发布者提供相应能力。只考虑可靠性、其他相关策略兼容时：

| 发布端提供 | 订阅端请求 | 能否匹配 |
| --- | --- | --- |
| `BEST_EFFORT` | `BEST_EFFORT` | 能 |
| `BEST_EFFORT` | `RELIABLE` | 不能 |
| `RELIABLE` | `BEST_EFFORT` | 能 |
| `RELIABLE` | `RELIABLE` | 能 |

因此，看到话题存在却没有数据时，不能只看节点是否启动，还应对照两端的 QoS。可靠性匹配之后，其他影响兼容的策略也需兼容。规则依据见[Humble 官方 QoS 文档](https://github.com/ros2/ros2_documentation/blob/humble/source/Concepts/Intermediate/About-Quality-of-Service-Settings.rst)。

### 怎样理解“里程计丢一帧，速度指令丢一帧”

里程计通常连续提供累计位姿估计。消费者漏收一条消息后，后续消息仍可带来新的累计状态；但中间时刻的信息并没有因此全部恢复，丢帧仍可能影响控制、插值或融合。如果驱动自己漏掉了底层测量，估计误差也可能已经产生。

`cmd_vel` 通常同样是周期更新的速度要求，并非天然的一次性事件。丢失一条停止或转向指令可能影响运动，长期执行旧指令也可能造成问题，因此要结合更新频率、接收时效与超时保护设计。

这段直觉用于比较数据用途，不能简化为“里程计随便丢都没关系”或“控制指令用了 reliable 就一定执行”。

### 板书

```text
Reliability：丢失的样本是否要求可靠交付
History + Depth：怎样保存尚未处理的样本

发布 BEST_EFFORT → 订阅 RELIABLE：不匹配
发布 RELIABLE → 订阅 BEST_EFFORT：可靠性可匹配

传输可靠性 + 数据时效 + 指令超时处理，共同影响控制行为
```

### 命令行实验：用官方 CLI 观察不匹配与恢复

**实验目标：**只改变可靠性配置，观察有话题却收不到消息的情况；再比较两端的深度设置。本实验使用独立的 `/lesson9/qos_demo` 话题，不向实体底盘发送速度。

实验依据为上面的官方匹配规则，命令参数对照 Humble 官方 [ros2 topic pub](https://github.com/ros2/ros2cli/blob/humble/ros2topic/ros2topic/verb/pub.py)与 [ros2 topic echo](https://github.com/ros2/ros2cli/blob/humble/ros2topic/ros2topic/verb/echo.py)实现。操作使用官方工具，无需编写节点。

**准备条件：**Ubuntu 22.04 或 WSL Ubuntu 22.04，ROS 2 Humble，无需图形环境。以下命令在 Ubuntu Bash 执行，目录不限。缺少工具或消息包时安装：

```bash
sudo apt update
sudo apt install ros-humble-ros2topic ros-humble-std-msgs
```

准备 A、B、C 三个终端，每个终端执行：

```bash
source /opt/ros/humble/setup.bash
echo "${ROS_DOMAIN_ID:-0}"
```

各终端 Domain ID 一致，并停止其他在 `/lesson9/qos_demo` 上运行的发布者。本实验用 5 Hz 便于观察输出，不代表项目实际频率。A、B 中的持续命令均需先按 `Ctrl+C`，才能切换配置。

**① A：以 best effort 持续发布。**

```bash
ros2 topic pub /lesson9/qos_demo std_msgs/msg/String "{data: 'qos_demo'}" \
  --rate 5 --qos-reliability best_effort --qos-durability volatile \
  --qos-history keep_last --qos-depth 5
```

**B：以 reliable 请求接收。**

```bash
ros2 topic echo /lesson9/qos_demo std_msgs/msg/String \
  --qos-reliability reliable --qos-durability volatile \
  --qos-history keep_last --qos-depth 5
```

**C：检查发现的端点。**

```bash
ros2 topic info /lesson9/qos_demo --verbose
```

预期 A 持续输出发布记录，C 可以发现发布与订阅端点，但 B 收不到业务消息；具体实现可能同时提示可靠性不兼容。不要把发现了订阅端点等同于已经成功匹配，也不要仅凭有没有警告判断实验结果。

**② 保持 A 不变，B 按 `Ctrl+C` 后改为 best effort。**

```bash
ros2 topic echo /lesson9/qos_demo std_msgs/msg/String \
  --qos-reliability best_effort --qos-durability volatile \
  --qos-history keep_last --qos-depth 5
```

预期 B 开始持续收到 `data: qos_demo`。此处改变的是订阅端的可靠性，话题名与消息类型不变。

**③ B 保持 best effort，A 按 `Ctrl+C` 后改为 reliable。**

```bash
ros2 topic pub /lesson9/qos_demo std_msgs/msg/String "{data: 'qos_demo'}" \
  --rate 5 --qos-reliability reliable --qos-durability volatile \
  --qos-history keep_last --qos-depth 5
```

预期发现并匹配新端点后，B 继续收到消息，说明可靠发布端可以匹配尽力订阅端。

**④ 保持 A 的 reliable 发布，B 按 `Ctrl+C` 后请求 reliable，并将深度改为 1。**

```bash
ros2 topic echo /lesson9/qos_demo std_msgs/msg/String \
  --qos-reliability reliable --qos-durability volatile \
  --qos-history keep_last --qos-depth 1
```

C 再次查询：

```bash
ros2 topic info /lesson9/qos_demo --verbose
```

预期 B 能收到消息，C 显示发布端深度为 5、订阅端深度为 1。两端深度不必相同；本步骤验证配置和匹配，不证明 depth 为 1 就一定降低了延迟，也不通过肉眼输出推断网络丢包率。

**验收：**记录四次发布端与订阅端的可靠性及收数情况，完成匹配表；在最后一步指出两端各自的深度。解释第一步“端点存在但没有数据”的原因，以及 reliable 为什么不能替代设备执行反馈。

**排查与结束：**所有组合都无数据时，先检查环境、Domain ID、话题名与类型，再查发布进程及端点信息；第一步意外收到数据时，检查是否还有其他 reliable 发布者。某个参数不被识别时运行 `ros2 topic pub --help` 或 `ros2 topic echo --help`，核对实际工具版本。实验完成后 B、A 分别按 `Ctrl+C` 结束。

### 课堂提问

> IMU 发布端使用 sensor_data，订阅端指定 reliable，为什么可能完全收不到消息？

参考答案：sensor_data 的可靠性为 best effort，不能满足 reliable 订阅请求。应按上下游需求调整兼容配置，而非只增加队列深度。

> 队列深度从 5 改成 100，是否就解决了处理速度不足？

参考答案：没有提高处理能力，可能只是容纳更多积压数据。应检查回调耗时、调度、发布速率与允许的数据年龄。

> 使用 system_default，是否可以直接在报告中写“已确保 reliable”？

参考答案：不能。需要核对底层默认值或运行端点；明确要求可靠性时应显式配置并验证。

这页最后可以总结：

> **QoS 要同时检查两端的匹配、缓存策略和数据时效。选型依据是实际用途与运行结果，不能只凭话题名称或预设名称判断。**

---

## 第 9 页｜核心消息类型

这页你可以把它讲成：

> **驱动把串口里的字节转换出来以后，还要把它们放进含义明确的消息。速度要求用 Twist，运动估计用 Odometry，惯性信息用 Imu，电池状态用 BatteryState。读消息时要一起检查字段、单位、参考系和有效性，不能只看数值像不像。**

### 讲解与例子

**1. `geometry_msgs/msg/Twist`：线速度与角速度。**

`linear` 与 `angular` 各有 x、y、z 三个分量，分别使用 m/s 和 rad/s。本课的 `cmd_vel` 按车体系约定：x 前、y 左、z 上。全向底盘可以使用 `linear.y` 表达侧向速度，理想差速模型通常将其设为零；平面控制主要使用 `linear.x`、`linear.y` 与 `angular.z`。

Twist 是通用速度结构，本身没有 Header，也不强制限定车体系。本课的车体系来自控制接口约定；“地面机器人某些分量为零”也是当前模型与接口的限制，不是消息类型禁止这些字段取其他值。

**2. `nav_msgs/msg/Odometry`：位姿、速度与不确定度。**

按图片中的例子，`header.frame_id` 为 `odom`，`child_frame_id` 为 `base_footprint`。`pose.pose` 给出子坐标系在父参考系中的位置和姿态，`twist.twist` 给出的速度分量在子坐标系中表达。不能把两个部分都直接按 odom 轴向解释。

例如，机器人相对 odom 已左转 90°，车体系反馈 `linear.x = 0.2`、`linear.y = 0`，那么它沿车头前进，对应 odom 的正 y 方向，而不是正 x 方向。

`pose.covariance` 与 `twist.covariance` 各有 36 个元素，按行优先保存 6×6 矩阵。位姿部分描述位置与绕三轴旋转的不确定度，速度部分描述对应线速度和角速度的不确定度；不能因姿态用四元数，就把位姿协方差理解为四元数四个分量的协方差。

**3. `sensor_msgs/msg/Imu`：惯性测量与可用的姿态估计。**

`orientation` 为姿态四元数，`angular_velocity` 为三轴角速度，`linear_acceleration` 为三轴加速度数据。角速度必须转换为 rad/s，加速度必须转换为 m/s²，不能直接把 °/s 或以 g 为单位的原始读数填进去。加速度数据是否包含重力影响，需要核对设备输出和下游约定。

三部分各带一个 9 元素的 3×3 协方差矩阵。官方 Imu 定义约定：某部分协方差全零表示不确定度未知；若根本没有该部分的估计，应将相应协方差的第一个元素设为 -1。姿态不确定度描述绕三轴的旋转，不是四个四元数分量。

图片中的“下位机已融合”是本项目的处理分工，不代表所有 IMU 驱动都输出有效姿态。消费者必须检查数据与有效性标记。此处的全零约定来自 Imu 消息，不能直接套用到所有消息的协方差上。

**4. `sensor_msgs/msg/BatteryState`：电池状态。**

`voltage` 单位为 V，`current` 单位为 A；官方约定电流在放电时为负。下位机若使用相反符号，驱动需要先换算。

图片写“只使用这两个字段，其余默认”，实现时仍需遵守各字段的未知值约定。例如未测量的温度、剩余电量比例等浮点字段应按定义填写 NaN，不能把默认零当作“已测得为零”。电量比例 `percentage` 的有效范围是 0～1；没有电量估计时，不应填 0 来表达未知。

10 Hz 是课件中电池话题的发布频率，不是 BatteryState 消息自带的限制。

### 板书

```text
Twist：linear（m/s）+ angular（rad/s），参考系由接口约定
Odometry：pose 在 header.frame_id；twist 在 child_frame_id
Imu：姿态、角速度、加速度，各带 3×3 协方差
BatteryState：电压 V；电流 A，放电为负；未知值按字段定义表达

检查顺序：类型 → 字段 → 单位 → 坐标系 → 时间 → 有效性
```

### 命令行实验：查看官方定义并核对字段

**实验目标：**用官方命令读取四种消息的定义，再用官方生成的 Python 消息类完成一次字段检查。

来源：Humble 官方 [Twist](https://github.com/ros2/common_interfaces/blob/humble/geometry_msgs/msg/Twist.msg)、[Odometry](https://github.com/ros2/common_interfaces/blob/humble/nav_msgs/msg/Odometry.msg)、[Imu](https://github.com/ros2/common_interfaces/blob/humble/sensor_msgs/msg/Imu.msg)、[BatteryState](https://github.com/ros2/common_interfaces/blob/humble/sensor_msgs/msg/BatteryState.msg)，以及 [PoseWithCovariance](https://github.com/ros2/common_interfaces/blob/humble/geometry_msgs/msg/PoseWithCovariance.msg) 和 [TwistWithCovariance](https://github.com/ros2/common_interfaces/blob/humble/geometry_msgs/msg/TwistWithCovariance.msg) 定义。

**准备条件：**Ubuntu 22.04 或 WSL Ubuntu 22.04，已安装 ROS 2 Humble。使用 Ubuntu Bash，一个终端即可，执行目录不限，无需图形环境或实体底盘。

```bash
sudo apt update
sudo apt install ros-humble-geometry-msgs ros-humble-nav-msgs \
  ros-humble-sensor-msgs ros-humble-ros2interface
source /opt/ros/humble/setup.bash
```

**① 读取消息结构。**

```bash
ros2 interface show geometry_msgs/msg/Twist
ros2 interface show nav_msgs/msg/Odometry
ros2 interface show sensor_msgs/msg/Imu
ros2 interface show sensor_msgs/msg/BatteryState
```

预期看到嵌套字段与定长数组。请学生找到：Twist 有没有 Header；Odometry 的两个坐标系字段在哪里；Imu 的缺失估计如何标记；BatteryState 对放电电流和未测量字段怎样约定。必要时结合上面的官方源文件阅读注释。

**② 构造字段检查样例。**

下面是依据官方定义编写的最小课堂练习，不是硬件采样程序。只在内存构造消息，不发布话题，也没有填写有效采样时间。示例中的里程计协方差保持默认，仅检查数组长度，不作为可信估计使用。

```bash
python3 - <<'PY'
import math
from geometry_msgs.msg import Twist
from nav_msgs.msg import Odometry
from sensor_msgs.msg import Imu, BatteryState

command = Twist()
command.linear.x = 0.2
print('Twist:', command.linear.x, command.linear.y, command.angular.z)

odom = Odometry()
odom.header.frame_id = 'odom'
odom.child_frame_id = 'base_footprint'
odom.pose.pose.position.x = 1.0
odom.pose.pose.orientation.z = math.sin(math.pi / 4)
odom.pose.pose.orientation.w = math.cos(math.pi / 4)
odom.twist.twist.linear.x = 0.2
print('Odometry frames:', odom.header.frame_id, odom.child_frame_id)
print('Odometry covariance lengths:',
      len(odom.pose.covariance), len(odom.twist.covariance))

imu = Imu()
imu.orientation_covariance[0] = -1.0
print('Imu orientation marker:', imu.orientation_covariance[0])
print('Imu covariance lengths:', len(imu.orientation_covariance),
      len(imu.angular_velocity_covariance), len(imu.linear_acceleration_covariance))

battery = BatteryState()
battery.present = True
battery.voltage = 12.4
battery.current = -0.5
for field in ('temperature', 'charge', 'capacity', 'design_capacity', 'percentage'):
    setattr(battery, field, float('nan'))
print('Battery discharging:', battery.current < 0)
print('Battery percentage unknown:', math.isnan(battery.percentage))
PY
```

**预期结果：**Twist 的三个打印值为 `0.2 0.0 0.0`；两个里程计坐标系分别为 `odom`、`base_footprint`；Odometry 的两个协方差长度均为 36；Imu 的三个协方差长度均为 9，姿态缺失标记为 -1；最后两项电池检查均为 `True`。程序运行后自行结束。

**验收：**学生能解释本例中车体系前进速度对应 odom 的哪个方向，指出 IMU 姿态为何不能当作有效数据使用，并区分电量为零与电量未知。不能将样例中的期望速度与里程计速度恰好相等，理解成真实设备已经执行。

**排查：**找不到接口时检查依赖安装和 Humble 环境；Python 无法导入消息类时，确认使用的是能加载系统 ROS 2 包的 Python 环境，避免与未配置 ROS 路径的虚拟环境混用。本实验没有启动发布者，因此不应期待 `ros2 topic echo /odom` 能收到本例数据。

### 课堂提问

> `pose.pose.position.x` 与 `twist.twist.linear.x` 能否直接按同一个坐标轴解释？

参考答案：要先看两个坐标系。前者在 `header.frame_id` 中，后者在 `child_frame_id` 中；车体发生旋转后，两个 x 轴可能不同。

> Imu 的协方差全零，是否意味着测量完全没有噪声？

参考答案：按该消息的约定，全零表示不确定度未知。若某部分根本没有估计，应使用对应协方差首元素 -1 的标记。

> 电池电量没有估计时，为什么不能把 percentage 留为零？

参考答案：零是有效的电量比例，会被误认为电量耗尽；未知应按定义使用 NaN。

这页最后可以总结：

> **标准消息提供了共同结构，驱动还必须正确填写单位、坐标、时间与有效性，才能让下游正确使用数据。**

---

## 第 10 页｜定时器回调机制与命令行工具

这页你可以把它讲成：

> **驱动节点需要反复做几件事：收串口数据、更新里程计、发布 IMU、发布电池状态。定时器决定这些工作多久有一次执行机会，Executor 负责执行已经就绪的回调。写好周期只是第一步，还要确认回调不会互相拖慢，再用命令行观察节点、消息和坐标变换是否正常。**

### 讲解与例子

**1. 先把频率换算成周期。**

| 定时器 | PPT 设定频率 | 对应周期 | 本项目中的工作 |
| --- | --- | --- | --- |
| communication | 200 Hz | 5 ms | 读取串口字节，解析协议帧 |
| odom | 50 Hz | 20 ms | 请求速度数据、推算位姿、发布 TF |
| imu | 50 Hz | 20 ms | 请求并发布 IMU 数据 |
| battery | 10 Hz | 100 ms | 请求并发布电池数据 |

公式是 `周期 T = 1 / 频率 f`。上表是课件描述的设计配置，实际频率应通过运行结果核对，不能把表格当作已经测量过的数据。

请求下位机与收到回复之间存在通信延迟。因此，“每 20 ms 发起一次请求”不等于“每次回调开始时都已有一帧新数据”。实现还应区分最新有效反馈、尚未返回的请求与超时数据；不能把旧值重新发布就算作完成了一次新采样。

**2. 定时器到期表示回调就绪，不保证立即执行。**

本页按项目使用单线程 Executor 的情况讲解。同一执行器一次只执行一个回调；若串口回调仍未返回，里程计、IMU、订阅等就绪回调都需要等待。这里的“互不打断”指该执行器不会在一个普通回调中途切换去执行另一个回调，不表示操作系统不能抢占线程，也不表示所有回调严格按先来后到执行。

例如，communication 的周期只有 5 ms，若其中一次串口读取等待了 30 ms，这段时间内其他回调也不能由这个单线程执行器执行。后果可能是发布间隔变长、数据变旧、接收缓存积压；不能假定执行器随后会逐次补跑所有错过的周期。

对应关系可查阅官方 [Executor 概念说明](https://github.com/ros2/ros2_documentation/blob/humble/source/Concepts/Intermediate/About-Executors.rst)。

**3. 串口回调要及时返回，200 Hz 也不是越高越好。**

串口字节可能分批到达，一次读取不一定正好是一帧。回调应把已读字节交给接收缓存，解析完整帧，并保留尚未收全的部分。避免在回调中长时间 `sleep`、无限等待整帧或执行耗时计算。

200 Hz 轮询可以缩短读取间隔，降低数据长期积压的风险，但不会提高 UART 的波特率，也不能单靠这个数字保证缓存不溢出。还需要看输入数据量、缓存容量与每次处理耗时。例如，5 ms 周期却配置可能等待 10 ms 的阻塞读取，就应检查它对其他回调的影响。

需要推算里程计时，应使用实际有效时间间隔，而不是无论回调是否延迟都固定乘以 0.02 s。使用多线程也要同时考虑回调组与共享串口、缓存的访问保护，不能只把执行器换掉就认为问题解决了。

**4. 用工具按“节点 → 话题 → 数据 → 频率 → TF”检查。**

| 命令 | 可以检查什么 | 解释时的边界 |
| --- | --- | --- |
| `ros2 node list` | 当前可发现的节点 | 节点存在不代表回调运行正常 |
| `ros2 topic list` | 当前可发现的话题 | 有话题名称不代表正在收到数据 |
| `ros2 topic echo /odom` | 里程计消息内容 | 还要检查时间戳、坐标系与数值变化 |
| `ros2 topic hz /imu` | 工具接收到消息的频率 | 受 QoS、负载与传输影响，不等于传感器内部采样率 |
| `ros2 topic pub ... --once` | 发布一条指定消息 | 不能据此确认底盘执行，也不规定运动时长或距离 |
| `rqt_graph` | 节点与话题的连接 | 展示计算图，不展示完整 TF 坐标关系 |
| `ros2 run tf2_tools view_frames` | 收集 TF 并生成坐标树报告 | 没有 TF 发布者时不会凭空产生坐标关系 |
| `ros2 run tf2_ros tf2_echo odom base_footprint` | 查看车体系相对 odom 的变换 | 两个 frame 名称必须与实际发布的一致 |

PPT 的 `/cmd_vel` 示例表达车体 x 方向速度 0.2 m/s。`--once` 只限制发布次数，后续是否保持、超时停止，取决于接收端设计；真实底盘需要明确的指令超时与停止机制。本页用官方仿真话题练习发布命令。

### 板书

```text
200 Hz → 5 ms；50 Hz → 20 ms；10 Hz → 100 ms

定时器到期 → 回调就绪 → Executor 选择执行 → 回调返回
单线程：一个回调阻塞，会推迟其他回调
设定周期 ≠ 实际执行间隔 ≠ 工具接收间隔

计算图：节点与话题的连接
TF 树：坐标系之间的变换关系
```

### 实验一：观察官方 talker 的定时发布

**实验目标：**把定时器周期、回调发布和命令行测频对应起来。

来源：Humble 官方 [demo_nodes_py talker 源码](https://github.com/ros2/demos/blob/humble/demo_nodes_py/demo_nodes_py/topics/talker.py)。该例设置 1.0 s 周期，在定时器回调中发布 `/chatter`；它用于观察机制，不模拟项目的 200 Hz 串口通信。

**准备条件：**Ubuntu 22.04 或 WSL Ubuntu 22.04，已安装 ROS 2 Humble。以下使用 Ubuntu Bash；第二个实验还需要可用的图形桌面。安装依赖：

```bash
sudo apt update
sudo apt install ros-humble-demo-nodes-py ros-humble-turtlesim \
  ros-humble-turtle-tf2-py ros-humble-tf2-ros ros-humble-tf2-tools \
  ros-humble-rqt-graph
```

本页两个实验使用三个终端 A、B、C。**每个终端先执行下面三行**，让实验进程使用相同的本机通信环境；若 99 已用于其他实验，统一换成另一个空闲 Domain ID。

```bash
source /opt/ros/humble/setup.bash
export ROS_DOMAIN_ID=99
export ROS_LOCALHOST_ONLY=1
```

**① 终端 A 启动官方节点。**

```bash
ros2 run demo_nodes_py talker
```

**② 终端 B 查看节点与消息。**

```bash
ros2 node list
ros2 topic list
ros2 topic echo /chatter --once
```

预期发现 `/talker` 和 `/chatter`，收到一条包含 `Hello World` 及计数值的字符串消息。

**③ 终端 C 连续观察约 10 秒。**

```bash
ros2 topic hz /chatter
```

预期接收频率接近 1 Hz，通常会有小幅波动。结合官方源码中的定时器设置，说明“每秒执行一次发布回调”的含义；不要要求测量结果精确等于 1.000 Hz，也不要由此推断项目四个定时器都能达到设定频率。

**验收与结束：**记录设定周期、实测平均频率，说明二者为何不必完全一致。C 按 `Ctrl+C` 结束测频，A 按 `Ctrl+C` 停止 talker，再开始下一个实验。

### 实验二：用官方海龟示例观察计算图与 TF 树

**实验目标：**完成一次节点发现、话题检查、消息发布与 TF 观察，区分计算图和坐标树。

来源：Humble 官方 [tf2 入门示例](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Intermediate/Tf2/Introduction-To-Tf2.rst)及 [话题命令行教程](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Beginner-CLI-Tools/Understanding-ROS2-Topics/Understanding-ROS2-Topics.rst)。沿用上述终端环境；该示例的 frame 是 `world`、`turtle1`、`turtle2`，不是实际底盘的 `odom` 与 `base_footprint`。

**① 终端 A 启动官方 TF 示例。**

```bash
ros2 launch turtle_tf2_py turtle_tf2_demo.launch.py
```

预期出现海龟窗口与两只海龟，示例中的跟随节点会控制第二只海龟追踪第一只。

**② 终端 B 检查计算图，再观察消息。**

```bash
ros2 node list
ros2 topic list
ros2 run rqt_graph rqt_graph
```

在图中找到海龟相关节点与话题，注意界面过滤选项可能隐藏部分连接。观察后关闭图窗口或按 `Ctrl+C`，继续执行：

```bash
ros2 topic echo /turtle1/pose --once
ros2 topic hz /turtle1/pose
```

测频约 10 秒后按 `Ctrl+C`，记录实际接收频率。这里测的是官方仿真位姿话题，不是项目的 50 Hz `/odom`。

**③ 终端 C 向第一只海龟发布一条速度命令。**

```bash
ros2 topic pub /turtle1/cmd_vel geometry_msgs/msg/Twist \
  '{linear: {x: 0.2}, angular: {z: 0.0}}' --once
```

观察第一只海龟的运动和第二只海龟的跟随。海龟仿真采用自己的速度超时处理，不能把它的运动持续时间当作真实底盘的约定。随后显式发送零速度：

```bash
ros2 topic pub /turtle1/cmd_vel geometry_msgs/msg/Twist \
  '{linear: {x: 0.0}, angular: {z: 0.0}}' --once
```

**④ 终端 B 查看变换，终端 C 生成坐标树。**

终端 B：

```bash
ros2 run tf2_ros tf2_echo world turtle1
```

预期连续显示 `turtle1` 相对于 `world` 的平移与旋转。终端 C 在可写的实验目录中执行：

```bash
mkdir -p ~/ros2_timer_tf_lab
cd ~/ros2_timer_tf_lab
ros2 run tf2_tools view_frames
```

等待工具收集约 5 秒 TF，按终端提示打开生成的 PDF。预期能看到 `world` 与两只海龟坐标系的关系。对照刚才的 rqt_graph，指出“话题连接”与“坐标变换连接”的区别。

**验收：**保留一份测频记录和 TF 树报告；能指出一条节点与话题连接、一条父子坐标关系，并说明要在真实项目中使用 `tf2_echo odom base_footprint`，必须先有节点发布对应 TF。

**排查与结束：**节点相互不可见时，先核对每个终端的 Humble 环境、Domain ID 和本机通信设置；有话题但 echo 或 hz 没有数据时，检查发布者是否仍在运行，再用 `ros2 topic info <话题名> --verbose` 查看类型与 QoS。TF 提示找不到 frame 时，先检查名称、示例进程与 `/tf` 数据；生成报告失败时检查当前目录写权限和工具依赖。结束时 B 停止 tf2_echo，A 停止 launch，关闭图形窗口。

### 课堂提问

> 定时器设置为 200 Hz，为什么实际可能达不到每秒执行 200 次？

参考答案：5 ms 只是设定周期。回调耗时、其他回调占用线程、系统调度与通信等待都会影响实际执行；需要测量，不能把到期视为立即执行。

> 串口回调里等 100 ms，为什么 50 Hz 的里程计也会受影响？

参考答案：同一单线程执行器在这段等待期间不能执行其他回调，里程计即使已经就绪也要等待。串口等待应有合适的边界，并与缓存和超时处理一起设计。

> rqt_graph 已经看到节点连接，为什么还要运行 tf2_echo？

参考答案：节点连接只能说明通信关系；tf2_echo 用于检查指定坐标系之间能否获得变换及其数值。两种工具回答的问题不同。

这页最后可以总结：

> **定时器提供执行节奏，回调耗时决定节奏能否维持；命令行工具帮助我们从“配置看起来正确”走到“运行结果可以检查”。**

---

## 第 11 页｜坐标系与 TF 变换

这页你可以把它讲成：

> **上一章讲清了消息怎样传递，这一章要讲清消息里的数字相对于哪里。机器人说“向前走”，是沿车头方向，还是沿地图的某条轴？IMU 固定在车上，它的轴是否与车体完全一致？坐标系给这些数字一个参照，TF 把不同参照联系起来，姿态表示则说明这些坐标轴怎样相对旋转。**

### 讲解与例子

**1. 用一个问题引入：只说“机器人在 x = 1 m”，信息够不够？**

不够，还需要说明相对于哪个原点、沿哪条轴。描述机器人的位姿，要同时给出位置与朝向，并明确参考坐标系；涉及运动，还应明确对应时间。

例如，车体相对 odom 已左转 90°，这时车体“向前”对应 odom 的正 y 方向。车体系的 `linear.x = 0.2` 与 odom 中沿正 x 方向运动，不是同一件事。这里先让学生建立“同一个运动可以在不同坐标系下表达”的直觉，后续页面再按 PPT 展开变换计算。

**2. 先认识 odom、base_link、imu 三个角色。**

| 坐标系 | 本章先建立的认识 | 容易混淆的地方 |
| --- | --- | --- |
| `odom` | 用于连续的局部运动估计，机器人相对它的位姿随运动变化 | 里程计会漂移，不能直接视为长期无误差的全局坐标 |
| `base_link` | 固连在机器人底盘上的车体坐标系 | 原点位置由模型定义，不一定就是地面中心 |
| `imu` | 随传感器固定安装的坐标系，实际名称也可能是 `imu_link` 等 | 轴向可能与车体不同，需要安装变换 |

前文项目里出现的 `base_footprint` 用于地面平面上的车体参考。可先在板书中画出下面这条**关系示意**，实际层级、名称与数值要核对项目模型和发布配置：

```text
odom → base_footprint → base_link → imu
       车体平面参考      车体参考     传感器参考
```

其中，里程计提供机器人相对 odom 的运动关系；刚性安装的传感器相对车体的关系通常固定。`imu` 是坐标系名称，`/imu` 可以是话题名称，两者即使同名也不是同一个概念。只在消息中填写 `frame_id`，不会自动建立 TF 连接。

坐标轴与单位约定参考官方 [REP-103](https://github.com/ros-infrastructure/rep/blob/master/rep-0103.rst)：常见车体系采用右手系，x 向前、y 向左、z 向上，长度用米、角度用弧度。移动平台的 odom 与 base_link 语义参考 [REP-105](https://github.com/ros-infrastructure/rep/blob/master/rep-0105.rst)。

**3. 静态与动态，判断的是相对关系是否随时间变化。**

IMU 刚性固定在底盘上，即使整台机器人正在运动，它相对 base_link 的安装位置与安装朝向仍然不变，适合使用静态 TF。机器人相对 odom 的位姿随运动变化，需要带时间的动态 TF；机器人暂时停车，不会让这条运动关系变成安装关系。

本章先记住：静态安装关系通常通过 `/tf_static` 发布，随时间变化的关系通过 `/tf` 发布。传感器输出的姿态与传感器的安装变换含义不同，不能把 IMU 的实时姿态直接当作固定安装角。

**4. 四元数与欧拉角是描述同一旋转的不同方式。**

roll、pitch、yaw 便于人理解，分别关联绕 x、y、z 轴的旋转，但必须说明旋转顺序和约定，某些姿态还会出现奇异性。单位四元数适合旋转的组合与插值，也避免欧拉角参数化的万向节锁问题，因此 ROS 常用它来存储姿态。

ROS 消息的四元数顺序是 `(x, y, z, w)`，无旋转为 `(0, 0, 0, 1)`，不是四个零；有效旋转四元数的模应为 1。它用四个分量表示旋转，并不表示机器人有四个旋转自由度。欧拉角仍可用于输入和显示，不能把 PPT 的问题理解为 ROS 完全不使用欧拉角。

来源：Humble 官方 [四元数基础教程](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Intermediate/Tf2/Quaternion-Fundamentals.rst)。这一页先回答“为什么需要”，具体转换方法留到对应 PPT 页面。

### 板书

```text
位姿 = 位置 + 朝向；必须说明参考系与时间
车体轴向：x 前、y 左、z 上；右手系

TF：把不同坐标系的表达联系起来
静态：相对安装关系固定；动态：相对关系随时间变化

RPY：便于理解，注意约定与奇异性
四元数：(x, y, z, w)，单位模；无旋转为 (0, 0, 0, 1)
```

### 导入实验：用官方工具观察一条传感器安装变换

**实验目标：**看到一条同时包含平移与旋转的 TF，并把它读成实际安装关系。

来源：Humble 官方 [静态 TF 发布教程中的命令行工具](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Intermediate/Tf2/Writing-A-Tf2-Static-Broadcaster-Py.rst)。使用官方 `static_transform_publisher`；下方坐标名称和安装参数是课堂示意，**不是 NanoOmni 的实测标定值**。实验只建立 `base_link → imu`，不创建 odom，不读取真实 IMU，也不控制底盘。

**准备条件：**Ubuntu 22.04 或 WSL Ubuntu 22.04，已安装 ROS 2 Humble，无需图形桌面。在 Ubuntu Bash 中安装依赖：

```bash
sudo apt update
sudo apt install ros-humble-tf2-ros
```

先结束上一页的实验进程。打开终端 A、B，**两个终端都执行**以下环境设置；若 99 已有其他实验，统一换用空闲 Domain ID，避免与其他 TF 发布者混用。

```bash
source /opt/ros/humble/setup.bash
export ROS_DOMAIN_ID=99
export ROS_LOCALHOST_ONLY=1
```

**① 终端 A 发布示意安装关系，保持运行。**

```bash
ros2 run tf2_ros static_transform_publisher \
  --x 0.1 --y 0.0 --z 0.05 \
  --roll 0.0 --pitch 0.0 --yaw 1.5707963267948966 \
  --frame-id base_link --child-frame-id imu
```

这表示 imu 原点在车体系中位于前方 0.1 m、上方 0.05 m，安装朝向绕车体 z 轴为正 90°，即从上方看逆时针旋转。这里 `--yaw` 输入的是弧度。

**② 终端 B 查询变换。**

```bash
ros2 run tf2_ros tf2_echo base_link imu
```

预期看到平移约为 `[0.100, 0.000, 0.050]`，四元数按 `(x, y, z, w)` 约为 `[0.000, 0.000, 0.707, 0.707]`，RPY 中 yaw 约为 1.571 rad。程序也可能显示角度制的辅助输出，应根据标签区分单位。查询启动时可能短暂等待 TF，收到后数值应保持不变。

**验收：**学生指出父坐标系、子坐标系和安装平移，说明此时 imu 的正 x 轴朝向 base_link 的正 y 方向，并解释“静态 TF 为什么也可以有 90° 旋转”。答案是：静态表示该旋转随时间不变，不表示旋转必须为零。

**排查与结束：**一直找不到 frame 时，检查 A 是否运行、两个终端的环境是否一致，以及坐标名称是否拼写正确；出现多组矛盾变换时，检查是否还有其他进程发布同一子坐标系。不要把这组课堂参数直接加入真实项目。实验完成后 B、A 依次按 `Ctrl+C` 退出。

### 课堂提问

> 机器人向前运动时，为什么 odom 中的 x 不一定增加？

参考答案：向前指车体系的正 x 方向，odom 中的变化取决于车体相对 odom 的朝向；车体左转 90° 后向前，主要增加的是 odom 的 y。

> IMU 随车一起运动，为什么它到 base_link 的 TF 仍可以是静态的？

参考答案：TF 描述两者的相对关系。刚性安装时，相对平移与旋转不变，与整车是否运动无关。

> 为什么不直接把三个欧拉角填入四元数的 x、y、z，再把 w 设为 1？

参考答案：二者不是同一种参数表示，必须按正确约定转换；直接填值既不表达预期旋转，也通常不满足单位模约束。

这页最后可以总结：

> **先说清数据属于哪个坐标系，再通过 TF 联系不同坐标系；位置、朝向和时间含义明确，后面的里程计与传感器数据才有共同参照。**

---

## 第 12 页｜REP-103 标准与常用坐标系

这页你可以把它讲成：

> **这一页先统一方向，再统一名称。车体向前是正 x，向左是正 y，向上是正 z；从上方看，左转是正偏航。然后把地图、里程计、车体和 IMU 连成一棵树。统一这些约定，是为了让驱动、定位和导航对同一组数字有相同理解；具体原点放在哪里、由谁发布变换，还要看项目模型和配置。**

### 讲解与例子

**1. REP-103 规定轴向与单位，先用右手确认方向。**

对于本课的车体坐标系，x 向前、y 向左、z 向上。板书里的 `x × y = z` 指坐标轴单位向量的叉积关系，不是把三个坐标数值相乘。

让学生用右手演示：拇指指向正 z，四指弯曲的方向就是绕 z 轴的正转方向。观察者位于正 z 一侧向原点看，也就是从车顶向下看，逆时针旋转为正偏航；这对应机器人左转。观察方向换到下面，视觉上的顺逆时针会反过来，所以不能脱离观察方向只背“逆时针为正”。

例如，车头原来沿参考系正 x，左转 90° 后朝向正 y，yaw 增加约 `π/2 rad`。速度消息里，`linear.x > 0` 表达向车头方向运动，`linear.y > 0` 表达向车体左侧运动，`angular.z > 0` 表达正偏航角速度。长度使用 m，角度使用 rad，角速度使用 rad/s。

依据：[REP-103：单位与坐标约定](https://github.com/ros-infrastructure/rep/blob/master/rep-0103.rst)。PPT 中“所有 ROS 包和算法都遵循此标准”应作为接口一致性的目标来讲，接入设备时仍要核对。标准本身也规定了相机光学坐标等约定，例如 `_optical` 坐标系是 z 向前、x 向右、y 向下；不能把所有传感器轴都直接当成车体轴。

**2. 五个常用坐标系，各自解决不同问题。**

| 坐标系 | 本页讲解重点 | 需要核对的边界 |
| --- | --- | --- |
| `odom` | 提供连续的局部运动参考，适合短时间运动估计 | 它是世界固定参考；“漂移”指机器人相对它的估计逐渐偏离真实运动，不是坐标系随车移动 |
| `base_footprint` | 车体在地面上的平面参考，便于描述平面位置与偏航 | PPT 的 z=0 建立在地面被选作零高度平面的简化上，不表示它在任何全局坐标系中的 z 都为零 |
| `base_link` | 固连于机器人本体的参考，用来连接传感器与几何模型 | PPT 选取轮轴中心高度；这不是 REP 规定的统一高度，应以 URDF 和模型定义为准 |
| `imu` | IMU 的传感器参考，说明测量轴与车体轴的关系 | 核对实际 frame 名称、轴向与安装位置；话题名 `/imu` 不会自动创建这个坐标系 |
| `map` | 建图、定位使用的全局参考，支持长期位置估计 | 定位修正可能使机器人在 map 中的估计位姿跳变，不能理解为永远无误差 |

其中，odom、map、base_link 的语义来自 [REP-105：移动平台坐标系](https://github.com/ros-infrastructure/rep/blob/master/rep-0105.rst)，并非都由 REP-103 定义。先区分“轴向和单位标准”与“坐标系的用途”，学生后面才不容易混淆。

**3. 沿 PPT 的树，从全局参考走到传感器。**

```text
map
 └─ odom
     └─ base_footprint
         └─ base_link
             └─ imu
```

按本页的项目结构，`odom → base_footprint` 由里程计相关节点提供；`base_footprint → base_link` 描述本体相对平面参考的关系，`base_link → imu` 描述传感器安装关系。对于本课的平面刚性底盘，后两条通常可以用固定变换表达，实际数值需要查看模型和安装标定。

`map → odom` 把定位的全局修正接到局部里程计上。本课可以将它理解为由配置好的建图或定位组件负责，例如 Cartographer 或 AMCL；是否发布、frame 名称和发布权限，都要核对运行配置。不要让两个节点同时争相发布同一条关系。

“完整 TF 树包含 5 个坐标系”应理解为本页画出的五个主要坐标系。添加激光雷达、相机、轮子或机械臂后，树还会扩展；本页的中间层也不是每台机器人都必须照搬。树中每个非根坐标系应有一个父坐标系，不能为了方便给同一个 base_link 同时接两个父节点。

**4. 为什么既需要 map，又需要 odom？**

可以用“走廊中向前走，同时收到一次定位修正”举例。里程计希望运动估计保持连续，便于控制器平稳使用；定位系统希望把累计误差修正回来，修正结果可能不连续。通过调整 `map → odom`，可以把全局修正与局部连续运动参考分开处理。

这不意味着 odom 永远准确，也不意味着 map 的修正一定正确；两者提供的是不同用途的参考。后续排查时，要先明确看到的是哪一个参考系中的位置变化。

### 板书

```text
REP-103：右手系；车体 x 前、y 左、z 上；m、rad、rad/s
从 +z 看向原点：逆时针为正 yaw，即左转为正

REP-105：odom / map / base_link 的移动平台语义
odom：局部连续，估计会漂移
map：全局定位参考，修正可能跳变

本页结构：map → odom → base_footprint → base_link → imu
原点、高度、安装变换、发布者：查模型与配置
```

### 实验：用官方海龟 TF 示例验证正偏航方向

**实验目标：**把“从上方看左转为正”与实际 TF 输出联系起来，并确认坐标树的结构由发布者建立。

来源：Humble 官方 [tf2 入门示例](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Intermediate/Tf2/Introduction-To-Tf2.rst)。使用其中的双海龟场景、键盘控制和 TF 工具，课堂观察重点改为旋转方向与坐标层级。示例使用 `world`、`turtle1`、`turtle2`，不模拟项目的完整五坐标系结构，也不评估真实里程计漂移。

**准备条件：**Ubuntu 22.04 或 WSL Ubuntu 22.04，ROS 2 Humble，具有可用图形桌面。以下命令在 Ubuntu Bash 中执行：

```bash
sudo apt update
sudo apt install ros-humble-turtlesim ros-humble-turtle-tf2-py \
  ros-humble-tf2-ros ros-humble-tf2-tools
```

结束前一页的静态 TF 实验。打开终端 A、B、C，**每个终端都先执行**以下设置；若 99 已被其他实验使用，统一换用空闲 Domain ID。

```bash
source /opt/ros/humble/setup.bash
export ROS_DOMAIN_ID=99
export ROS_LOCALHOST_ONLY=1
```

**① 终端 A 启动官方场景，终端 B 启动键盘控制。**

终端 A：

```bash
ros2 launch turtle_tf2_py turtle_tf2_demo.launch.py
```

终端 B：

```bash
ros2 run turtlesim turtle_teleop_key
```

预期窗口中有两只海龟，第一只由键盘控制，第二只由示例的跟随节点控制。操作时让键盘焦点停留在终端 B。

**② 终端 C 观察第一只海龟相对 world 的变换。**

```bash
ros2 run tf2_ros tf2_echo world turtle1
```

记录初始 RPY 输出中的 yaw。场景刚启动且未操作时，第一只海龟通常朝向 world 的正 x，yaw 接近零。回到 B，短按左方向键，等待 C 下一次输出；预期海龟从上方看逆时针转动，yaw 相比初值增加。再短按右方向键，观察相反变化。

只做小角度转动，避免跨过 `±π` 的表示边界。跨界时 yaw 数字可能由接近 `+π` 变成接近 `-π`，这不能直接判为旋转方向错误。记录键盘操作前后的数值与可见转向，不要求每次按键产生完全相同的角度。

**③ 检查车体“向前”与参考系轴向的区别。**

在 B 中将第一只海龟转到大致朝 world 正 y 的方向，再短按上方向键。观察它沿自身车头方向前进，C 中主要增加的是平移的 y 分量。此处只做方向性验证，不把键盘控制当作精确的 90° 转角或距离控制。

**④ 终端 C 停止查询，再生成坐标树。**

按 `Ctrl+C` 结束 tf2_echo，然后执行：

```bash
mkdir -p ~/ros2_rep103_lab
cd ~/ros2_rep103_lab
ros2 run tf2_tools view_frames
```

等待工具收集约 5 秒 TF，按终端提示查看生成的 PDF。预期 `world` 是 `turtle1` 和 `turtle2` 的共同父坐标系；它们不是“第一只海龟下面接第二只海龟”的父子关系。第二只跟随第一只运动，也不要求 TF 树把二者直接连成父子。

**验收：**记录一组左转前后的 yaw，并说明正方向的观察视角；指出海龟朝 world 正 y 时向前运动的平移变化；画出示例的三坐标系树，再对照 PPT 说明为什么项目的五坐标系树需要对应发布者，而不会由名字自动生成。

**排查与结束：**键盘无响应时先检查焦点是否在 B；没有 TF 时检查 A 是否运行、frame 拼写及各终端环境；左转数值看似减小时先检查是否跨越角度边界或查反了坐标顺序。生成报告失败时检查目录权限与工具依赖。结束时 B 停止键盘控制，A 停止 launch，关闭海龟窗口。

### 课堂提问

> PPT 写 base_link 在轮轴中心高度，能否给所有机器人都这样定义？

参考答案：不能把它当作统一标准。base_link 固连于机器人本体，具体原点由模型选择，应核对该机器人的 URDF 与坐标说明。

> base_footprint 的 z=0，是否说明它在 map 中永远没有高度？

参考答案：不是。零高度依赖参考平面的选取和建模假设；它相对 map 的高度还取决于整条变换链。

> 第二只海龟追着第一只运动，为什么两者在 TF 树中仍可以是兄弟节点？

参考答案：TF 的父子关系表达坐标参照，不是任务跟随关系。它们都相对 world 发布位姿时，TF 可以通过共同父系计算两者间的变换。

这页最后可以总结：

> **先用 REP 统一轴向和含义，再用模型与配置确认坐标原点和 TF 层级；名字相同不保证定义正确，数据能够相互转换才说明坐标关系真正接通。**

---

## 第 13 页｜本项目的 TF 树

这页你可以把它讲成：

> **这一页把上一页的坐标约定落实到本项目。先从 odom 找到机器人在地面上的参考，再向上找到本体，最后找到 IMU。第一条关系随机器人运动变化，后两条描述固定的几何关系。读图时，每根箭头都要说清父坐标系、子坐标系，以及子坐标系相对父坐标系的位置和朝向。**

### 讲解与例子

**1. 沿着三条边读图，不要只记四个名字。**

| 父坐标系 → 子坐标系 | PPT 的设置 | 物理含义 |
| --- | --- | --- |
| `odom → base_footprint` | 动态，目标 50 Hz | 机器人平面参考相对里程计参考的运动位姿 |
| `base_footprint → base_link` | 静态，平移 `(0, 0, 0.032)` m，无旋转 | 车体原点比地面参考高 32 mm，坐标轴方向一致 |
| `base_link → imu` | 静态，零平移、无旋转 | 本页模型把 IMU 参考与车体参考设为重合、同向 |

这里的箭头表示 TF 树的父子关系；平移是**子坐标系原点在父坐标系中的坐标**，旋转描述子坐标轴相对父坐标轴的朝向。例如，第二条不是说地面参考在本体上方，而是本体原点在地面参考上方。

本页聚焦底盘驱动负责连接的四个坐标系。上一页出现的 `map → odom` 可以由建图、定位组件在上游提供，图中未画出它，不代表启动底盘驱动时就已经具备全局定位。

**2. 0.032 m 怎么来，为什么不能直接套到所有底盘？**

课件给出麦轮直径 64 mm，半径为 32 mm，即 0.032 m。如果把 base_footprint 原点放在地面，并把 base_link 原点放在轮轴中心高度，那么向上的平移就是 `(0, 0, 0.032)` m。

这里同时用了轮子尺寸和坐标原点的建模假设。若模型把 base_link 原点放在机身中心、控制板或其他位置，高度就可能不同。因此应核对 URDF、静态变换配置与实际结构，不能仅凭轮径决定 base_link 高度；更不能把 32 mm 直接填成 32 m。

**3. “零偏移”是模型声明，不等于没有 IMU 误差。**

`base_link → imu` 为单位变换，表示两个参考原点重合，而且 x、y、z 三轴逐一对齐。PPT 用“IMU 焊在板中央、x 轴朝前”解释这种简化，讲解时要补充：板中央未必就是 base_link 原点，只确认 x 轴也不足以确定完整的安装朝向。

实际接入时，应核对 IMU 测量参考点的安装位置、三轴方向，以及驱动是否已经把输出转换到车体系。若仍按传感器自身轴向发布，就需要与之匹配的安装 TF；若驱动已转换到车体系，应核对消息的 frame 声明，避免重复旋转。

这里的“零偏移”说的是几何安装关系，不是陀螺仪零偏、加速度计偏置或姿态误差。将 TF 设为单位变换不会完成传感器标定，也不会消除测量漂移。

**4. 动态 50 Hz 与静态 TF 的发布方式要分开理解。**

`odom → base_footprint` 的目标周期是 20 ms，发布时应带有与位姿估计一致的时间戳。即使机器人暂时不动，它仍是运动估计链路，不能用一个固定 TF 替代后续更新。50 Hz 是课件中的目标配置，实际频率和时间有效性还要检查。

两条安装关系通常通过 `/tf_static` 发布，不需要照着动态 TF 每 20 ms 重发。官方静态广播机制使后加入的 TF 监听者也能获得固定关系；课堂上应保持发布进程运行，不能把它理解成一条普通话题消息发完就可随意退出。

发布 Odometry 消息也不会自动生成这条动态 TF，需要相应广播者。反过来，能查到 TF 也不代表已经发布了 Odometry 消息。每条父子关系应由明确的发布者维护，避免驱动、URDF 的 robot_state_publisher 和手工命令重复发布同一关系。

来源：Humble 官方 [静态 TF 教程](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Intermediate/Tf2/Writing-A-Tf2-Static-Broadcaster-Py.rst)。

### 板书

```text
odom
 └─ base_footprint    动态，目标 50 Hz / 20 ms
     └─ base_link     静态，z = +0.032 m，无旋转
         └─ imu       静态，零平移、无旋转（课件模型）

64 mm ÷ 2 = 32 mm = 0.032 m
高度来自：轮子尺寸 + 原点选取
IMU 单位 TF ≠ 传感器零偏为零
```

### 实验：用官方工具连接并查询两条静态 TF

**实验目标：**按 PPT 参数建立静态子链，验证逐段查询、跨段组合和反向查询的结果。

来源：Humble 官方 [static_transform_publisher 用法](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Intermediate/Tf2/Writing-A-Tf2-Static-Broadcaster-Py.rst)与 [tf2 查询、坐标树工具示例](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Intermediate/Tf2/Introduction-To-Tf2.rst)。使用官方可执行工具，参数按本页课件设置，**用于验证课件模型，不作为实机安装标定结果**。本实验不发布假的 odom，也不连接或控制硬件。

**准备条件：**Ubuntu 22.04 或 WSL Ubuntu 22.04，已安装 ROS 2 Humble，无需图形桌面即可执行查询。以下使用 Ubuntu Bash：

```bash
sudo apt update
sudo apt install ros-humble-tf2-ros ros-humble-tf2-tools
```

先结束前一页海龟实验及其他手工 TF 发布进程。打开终端 A、B、C，**每个终端都执行**以下设置；若 99 已用于其他实验，统一换用空闲 Domain ID。

```bash
source /opt/ros/humble/setup.bash
export ROS_DOMAIN_ID=99
export ROS_LOCALHOST_ONLY=1
```

**① 终端 A 发布地面参考到车体参考的安装关系。**

```bash
ros2 run tf2_ros static_transform_publisher \
  --x 0.0 --y 0.0 --z 0.032 \
  --roll 0.0 --pitch 0.0 --yaw 0.0 \
  --frame-id base_footprint --child-frame-id base_link
```

**② 终端 B 发布车体参考到 IMU 参考的单位变换。**

```bash
ros2 run tf2_ros static_transform_publisher \
  --x 0.0 --y 0.0 --z 0.0 \
  --roll 0.0 --pitch 0.0 --yaw 0.0 \
  --frame-id base_link --child-frame-id imu
```

A、B 均保持运行。零旋转对应四元数 `(0, 0, 0, 1)`；不要把“零旋转”写成全零四元数。

**③ 终端 C 逐段和跨段查询。**

依次执行下列命令，每次观察输出后按 `Ctrl+C`，再执行下一条：

```bash
ros2 run tf2_ros tf2_echo base_footprint base_link
ros2 run tf2_ros tf2_echo base_link imu
ros2 run tf2_ros tf2_echo base_footprint imu
ros2 run tf2_ros tf2_echo imu base_footprint
```

| 查询命令中的坐标顺序 | 预期平移，单位 m | 预期旋转 |
| --- | --- | --- |
| `base_footprint base_link` | `(0, 0, +0.032)` | 单位四元数 |
| `base_link imu` | `(0, 0, 0)` | 单位四元数 |
| `base_footprint imu` | `(0, 0, +0.032)` | 单位四元数 |
| `imu base_footprint` | `(0, 0, -0.032)` | 单位四元数 |

前一个名称是本次输出的参考系，后一个是被查询的坐标系。跨段查询不需要额外发布 `base_footprint → imu`，TF 会沿已有链组合。反向查询得到负的高度，是因为地面参考在 IMU 参考下方；本例没有旋转，所以反向平移恰好取负，含旋转的一般情况不能只机械地改符号。

查询工具的屏幕刷新速度不等于 TF 发布速度；看到输出每秒更新，不能据此说动态 TF 只有 1 Hz，更不能说这两条静态 TF 必须按 50 Hz 发送。查询方向的含义也可核对官方 [tf2_echo 工具说明](https://github.com/ros2/geometry2/blob/humble/tf2_ros/src/tf2_echo.cpp)。

**④ 终端 C 生成坐标树。**

结束当前查询，在可写目录中执行：

```bash
mkdir -p ~/ros2_project_tf_lab
cd ~/ros2_project_tf_lab
ros2 run tf2_tools view_frames
```

预期生成包含 `base_footprint → base_link → imu` 的三坐标系树。此时查不到 odom 是正常的：实验只启动了两条静态安装关系，动态里程计广播者尚未运行。不能为了让图看起来完整，再用静态命令把 odom 固定到车体上。

**验收：**记录四组查询结果并解释高度正负号；指出为什么跨段查询不需要新建一条边；说明完整项目还缺少哪条动态关系，以及它与本实验的静态安装关系有何区别。

**排查与结束：**结果不符时先核对父子名称、m/mm 单位和旧发布进程；出现缺少 frame 时检查 A、B 的运行状态及终端环境。如果关闭某个静态发布者后，已经运行的监听者仍能查询，可能是它保留了静态缓存，不能据此确认发布者仍在工作；重新启动查询进程再检查。生成报告失败时检查目录写权限和工具依赖。实验结束后 C 停止查询，B、A 依次按 `Ctrl+C` 退出。

### 课堂提问

> 为什么 base_footprint 到 base_link 是正 0.032 m，反过来查询却是负值？

参考答案：前者表达车体原点在地面参考上方，后者表达地面参考在车体下方；参考系与被描述对象交换了。本例两系无相对旋转，因而平移直接变号。

> IMU 焊在板中央、x 轴朝前，是否足以证明安装 TF 为单位变换？

参考答案：还不够。需要确认测量参考点与 base_link 原点重合，并核对 y、z 轴方向及驱动是否已做坐标转换。

> 实验里能查询 base_footprint 到 imu，是否说明还要发布这条直接连接？

参考答案：不需要。TF 可以组合已有两条边；再给 imu 增加另一个父坐标系会破坏预期的树结构。

这页最后可以总结：

> **动态 TF 连接运动估计，静态 TF 连接安装几何；读懂每条边的方向、单位和来源，再用逐段与跨段查询验证整条链。**

---

## 第 14 页｜静态 TF 与动态 TF

这页你可以把它讲成：

> **静态 TF 表示固定的相对关系，动态 TF 表示某个时刻的相对关系。区别不只在发布快慢，更在时间含义。处理一帧传感器数据时，我们需要知道数据产生那个时刻的坐标关系；即使 TF 树已经连通，如果查询时间不在可用范围内，变换仍然可能失败。**

### 讲解与例子

**1. 用“是否随时间变化”分类，再看发布方式。**

| 对比项 | 静态 TF | 动态 TF |
| --- | --- | --- |
| 含义 | 固定的安装或几何关系 | 随时间变化的位姿关系 |
| 常用发布方式 | `static_transform_publisher`、静态广播接口；固定关节也可由 robot_state_publisher 提供 | 动态广播接口，由驱动、状态估计、建图或定位等节点提供 |
| 主要话题 | `/tf_static` | `/tf` |
| 时间语义 | 已接收的静态关系按固定关系使用，无需逐时刻采样 | 查询要匹配缓存中的时间范围，必要时插值 |
| 本项目示例 | `base_footprint → base_link`、`base_link → imu` | `odom → base_footprint`，以及定位组件提供的 `map → odom` |

PPT 的“一次发布／低频”应解释为固定关系不必持续按运动频率重发，而不是把动态 TF 降低频率就变成静态 TF。ROS 2 的静态 TF 使用 transient local 持久性，使后加入且采用匹配 QoS 的监听者能获得保留的数据；发布者仍应保持运行。已有监听者还可能保留自己的静态缓存。

本项目里程计 TF 的目标是 50 Hz；`map → odom` 的更新频率由建图、定位组件及其配置决定，不应一并说成 50 Hz。“各驱动节点”也不能概括所有动态 TF 发布者，AMCL 等定位组件同样可以提供动态关系。

来源：官方 [静态 TF 教程](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Intermediate/Tf2/Writing-A-Tf2-Static-Broadcaster-Py.rst)及 [静态 TF QoS 定义](https://github.com/ros2/geometry2/blob/humble/tf2_ros/include/tf2_ros/qos.hpp)。

**2. “时间戳一致”首先指同一状态的时间含义一致。**

如果某次计算得到机器人在时刻 t 的位姿，并同时发布 Odometry 和对应的 `odom → base_footprint` TF，两者应使用同一份状态对应的时间戳。不要把位姿标成采样时间 t，却把同一位姿的 TF 随手标成稍后的发布时刻。

这不要求里程计、IMU、相机等所有消息都写同一个时间戳。不同传感器可以在不同时间采样，关键是时间戳准确表达各自数据的有效时刻，并处在一致的时间基准上。硬件没有采样时间时，也应明确采用的接收时间近似及其延迟限制。

例如，MCU 的“开机后毫秒数”不能未经换算就当作 ROS 的绝对时间；仿真、回放中还要核对 `use_sim_time` 与 `/clock`，多机系统要核对时钟同步。增加发布频率不能解决两个时间基准不一致的问题。

PPT 提到项目曾修复时间戳问题，可以作为工程经验提醒；仅凭这一页无法确认当时的代码、错误原因和修复结果，课堂上不补写未经核对的项目修复细节。

**3. 插值需要前后样本，外推错误表示查询越过了可用范围。**

假设同一条动态关系在 10.00 s、10.02 s 各有一个样本。查询 10.01 s 时，可以在两者之间插值；查询 10.03 s 时，当前缓存没有更晚的样本，不能自动推算未来。如果请求比最早保留的样本还早，也会发生向过去外推错误。

PPT 的“在相邻变换之间线性插值”需要说得更准确：tf2 对平移做线性插值，对旋转做四元数球面插值（SLERP），不能直接把四元数四个分量各自随意线性平均。查询跨越多条动态边时，各边也都需要在该时刻有可用关系。

可用时间范围取决于**当前监听者已经收到并保留的数据**。因此刚启动时样本不足、传输延迟、过旧数据和时钟偏差，都可能导致查询失败；错误时间戳也可能产生看似平滑但时间含义错误的结果，不一定马上报错。

依据：Humble 官方 [TF 时间查询教程](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Intermediate/Tf2/Learning-About-Tf2-And-Time-Cpp.rst)及 [tf2 时间缓存与插值实现](https://github.com/ros2/geometry2/blob/humble/tf2/src/cache.cpp)。

**4. 排查时区分“没有坐标系”和“没有这个时刻的变换”。**

| 现象 | 优先检查 | 处理方向 |
| --- | --- | --- |
| frame 不存在或两系不连通 | 名称、发布进程、TF 树 | 修正名称或补上应有的发布者 |
| 向未来外推 | 请求时间是否晚于最新 TF；是否存在传输延迟或时钟偏差 | 核对时间基准；对正常延迟采用有界等待，并保证监听回调能继续接收数据 |
| 向过去外推 | 数据是否过旧；监听者是否刚启动；缓存是否覆盖该时刻 | 检查处理延迟、启动顺序与合理的缓存长度 |
| 能查询但叠图错位、跳变 | 时间戳与位姿是否对应；是否重复发布；定位是否在修正 | 对照原始时间与发布者逐项排查，不能只归因于插值 |

查询最新可用变换有时适合实时观察，但不能无条件代替按传感器消息时间查询。等待超时只提供接收迟到数据的机会，不会修正错误时间戳，也不会生成缺失历史。不要把旧数据全部重新盖成“现在”来掩盖问题。

来源：官方 [TF 调试教程](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Intermediate/Tf2/Debugging-Tf2-Problems.rst)。

### 板书

```text
静态：固定关系，/tf_static；动态：带时间的状态，/tf
同一状态：Odometry 与对应 TF 使用一致的有效时间

已有样本：10.00 s ── 10.01 s ── 10.02 s
                       ↑ 可插值          10.03 s → 当前缺少未来样本
平移：线性插值；旋转：四元数 SLERP

先查：frame 与连接 → 请求时间 → 缓存范围 → 时钟基准
最新可用 ≠ 数据采样时刻；等待 ≠ 外推或修正时间戳
```

### 实验一：观察官方动态 TF 示例中的时间信息

**实验目标：**区分动态变换的样本时间、查询工具的刷新频率和传输延迟。

来源：Humble 官方 [海龟 TF 示例](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Intermediate/Tf2/Introduction-To-Tf2.rst)及上述 TF 调试教程。无需修改官方示例源码。

**准备条件：**Ubuntu 22.04 或 WSL Ubuntu 22.04，ROS 2 Humble；本实验需要图形桌面，实验二不需要。使用 Ubuntu Bash 安装依赖：

```bash
sudo apt update
sudo apt install ros-humble-turtlesim ros-humble-turtle-tf2-py \
  ros-humble-tf2-ros ros-humble-tf2-ros-py ros-humble-geometry-msgs
```

结束前一页的手工发布进程。打开终端 A、B，**两个终端都执行**以下设置；若 99 已用于其他实验，统一换用空闲 Domain ID。

```bash
source /opt/ros/humble/setup.bash
export ROS_DOMAIN_ID=99
export ROS_LOCALHOST_ONLY=1
```

终端 A 启动官方场景：

```bash
ros2 launch turtle_tf2_py turtle_tf2_demo.launch.py
```

终端 B 先运行查询，观察数次输出后按 `Ctrl+C`，再运行监视工具：

```bash
ros2 run tf2_ros tf2_echo world turtle1
ros2 run tf2_ros tf2_monitor world turtle1
```

预期第一只海龟未运动时，位置可以保持不变，但动态 TF 的样本时间仍在更新。tf2_monitor 会给出所监视关系的频率与延迟统计；这些数值受示例实现、调度和运行环境影响，不要求等于项目的 50 Hz，也不能把 tf2_echo 的屏幕刷新间隔当作发布周期。

**验收与结束：**记录一次时间输出和一组监视统计，说明“位姿暂时不变”为什么仍可以通过动态 TF 发布。B 停止监视，A 停止 launch，再进行实验二。

### 实验二：可重复地观察插值与外推边界

**实验目标：**在已知样本时间的条件下，验证查询何时成功、何时报外推错误。

下面是依据官方时间查询教程及 [Python Buffer API](https://github.com/ros2/geometry2/blob/humble/tf2_ros_py/tf2_ros/buffer.py)编写的最小课堂练习，**不是官方完整示例程序**。它只向本进程的官方 TF 缓冲区放入构造样本，不发布 `/tf`、不模拟硬件测量，也不依赖网络或系统当前时间；因此能稳定观察指定的时间边界。

沿用已加载 Humble 环境的终端，直接执行：

```bash
python3 - <<'PY'
import rclpy
from geometry_msgs.msg import TransformStamped
from rclpy.duration import Duration
from rclpy.time import Time
from tf2_py import ExtrapolationException
from tf2_ros import Buffer


def sample(parent, child, seconds, x=0.0, z=0.0):
    transform = TransformStamped()
    transform.header.frame_id = parent
    transform.child_frame_id = child
    transform.header.stamp = Time(seconds=seconds).to_msg()
    transform.transform.translation.x = x
    transform.transform.translation.z = z
    transform.transform.rotation.w = 1.0
    return transform


rclpy.init()
try:
    buffer = Buffer(cache_time=Duration(seconds=10.0))
    buffer.set_transform(sample('lab_odom', 'lab_base', 10.0, x=0.0), 'lesson')
    buffer.set_transform(sample('lab_odom', 'lab_base', 12.0, x=2.0), 'lesson')
    buffer.set_transform_static(
        sample('lab_base', 'lab_sensor', 10.0, z=0.032), 'lesson')

    for label, seconds in [('middle', 11.0), ('future', 13.0), ('past', 9.0)]:
        try:
            result = buffer.lookup_transform('lab_odom', 'lab_sensor', Time(seconds=seconds))
            value = result.transform.translation
            print(f'{label}: x={value.x:.3f}, z={value.z:.3f}')
        except ExtrapolationException:
            print(f'{label}: ExtrapolationException')

    latest = buffer.lookup_transform('lab_odom', 'lab_sensor', Time())
    print(f'latest: x={latest.transform.translation.x:.3f}')
    buffer.set_transform(sample('lab_odom', 'lab_base', 13.0, x=3.0), 'lesson')
    result = buffer.lookup_transform('lab_odom', 'lab_sensor', Time(seconds=13.0))
    print(f'after sample: x={result.transform.translation.x:.3f}')
finally:
    rclpy.shutdown()
PY
```

**预期结果：**

```text
middle: x=1.000, z=0.032
future: ExtrapolationException
past: ExtrapolationException
latest: x=2.000
after sample: x=3.000
```

10 s 和 12 s 的位置分别为 x=0、x=2，查询 11 s 得到中间值 x=1；传感器到车体的静态高度为 0.032 m，所以组合结果始终带有这段高度。13 s 起初超出动态样本范围，加入 13 s 样本后才可查询；9 s 仍早于最早样本。`Time()` 在此接口中表示查询最新可用变换，初次查询得到的是 12 s 对应的 x=2，并没有获得 13 s 的状态。

**验收：**学生解释五行输出，并指出为什么一条静态边能够使用，不代表整条含动态边的链在任意时刻都可查询。本练习只观察平移插值和时间范围，不用于验证旋转插值精度、真实发布频率或网络 QoS。

**排查：**Python 导入失败时核对依赖安装、Humble 环境和当前 Python 解释器，避免混入未配置 ROS 的虚拟环境；外推错误没有按预期被捕获时核对所用发行版与 API。实验一启动阶段可能暂时缺少样本，应先等 TF 建立；持续缺少 frame 时先查发布进程与名称。本练习执行后自动结束，没有后台发布者需要关闭。

### 课堂提问

> 把同一份里程计位姿分别标上两个略有差别的时间，会发生什么？

参考答案：Odometry 与 TF 表达了不同的状态有效时间，下游可能在错误时刻组合数据，产生错位或查询失败；应统一该状态的时间来源。

> 请求 13 s，而最新动态样本只有 12 s，tf2 能否按前两帧的速度自动预测？

参考答案：普通时间查询不会这样预测，超出可用范围会报外推错误。可以在接收仍正常运行的前提下有界等待对应样本，但不能把等待当作预测。

> 为什么不能简单把所有查询都改成最新时间来解决报错？

参考答案：最新变换不一定对应传感器采样时刻。这样可能使查询成功，却把不同时间的数据混在一起，掩盖真正的时间同步问题。

这页最后可以总结：

> **静态 TF 说明固定关系，动态 TF 说明带时间的状态；树连得通只是第一步，查询时刻也落在有效范围内，数据才能按正确时间组合。**

---

## 第 15 页｜位姿表示：位置 + 四元数

这页你可以把它讲成：

> **位姿要回答两个问题：机器人原点在哪里，机器人朝向哪里。位置用三个数表达，朝向可以用单位四元数表达。这里的“位置加旋转”是说一份位姿由两部分组成，不是把向量与四元数直接相加。读消息时，要把位置、朝向与参考坐标系一起看，才能知道机器人真正处在什么状态。**

### 讲解与例子

**1. 三维位置描述的是原点在参考系中的坐标。**

位置 `(x, y, z)` 使用 m，描述被表示的刚体原点相对参考坐标系的位置。例如，机器人在 odom 系中的位置为 `(1, 2, 0)`，意思是它的参考原点位于 odom 的 x=1 m、y=2 m、z=0 m 处；这三个数本身还没有说明车头朝向。

PPT 的“地面机器人 z 恒为 0”应按本课的平面运动模型讲解：选择地面平面上的 base_footprint，忽略起伏和侧倾、俯仰后，可以令其在 odom 中的 z 为零。上一页 base_link 高出地面 0.032 m，说明地面机器人上的所有坐标原点并不都在 z=0；坡道、台阶和完整三维模型也不能直接套用这个简化。

**2. 四元数的 x、y、z 不表示位置，也不是三个欧拉角。**

本课采用 ROS 消息中的顺序 `q = [qx, qy, qz, qw]`，消息字段名称为 `x、y、z、w`。前三个是虚部，w 是实部，四个分量均无量纲。可以用下面的表格提醒学生区分同名字段：

| 字段 | 含义 | 单位 |
| --- | --- | --- |
| `position.x / y / z` | 原点的位置坐标 | m |
| `orientation.x / y / z / w` | 旋转四元数的四个分量 | 无量纲 |
| roll / pitch / yaw | 按明确约定表达的旋转角 | 通常用 rad |

用于表示旋转的单位四元数满足：

```text
qx² + qy² + qz² + qw² = 1
```

它用四个分量描述三维旋转，分量之间有约束，不代表增加了一个旋转自由度。无旋转是 `[0, 0, 0, 1]`，不是 `[0, 0, 0, 0]`。全零四元数没有有效旋转含义，也不能通过除以模长来归一化。

依据：Humble 官方 [四元数基础教程](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Intermediate/Tf2/Quaternion-Fundamentals.rst)。本课使用的 Pose、Odometry 与 TF 旋转字段采用四元数；PPT 的“ROS 中旋转一律用四元数存储”不能扩展为所有 ROS 接口都禁止直接使用角度。

**3. 平面运动只绕 z 轴旋转，先记住半角关系。**

在 `roll = pitch = 0`、仅有偏航角 θ 的情况下：

```text
qx = 0
qy = 0
qz = sin(θ / 2)
qw = cos(θ / 2)
```

公式中的 θ 使用弧度。90° 先换成 `π/2 rad`，代入后取的是半角 `π/4`；不要把角度 90 直接传给按弧度计算的三角函数，也不要漏掉除以 2。

| yaw | 弧度 | 四元数 `[qx, qy, qz, qw]` | 朝向解释 |
| --- | --- | --- | --- |
| 0° | 0 | `[0, 0, 0, 1]` | 与参考系方向一致 |
| +90° | π/2 | `[0, 0, 0.7071, 0.7071]`，近似值 | 从参考系正 x 转向正 y |
| +180° | π | `[0, 0, 1, 0]`，忽略浮点微小误差 | 转向参考系负 x |

这里沿用右手系，从正 z 一侧向下看，正 yaw 为逆时针。表中的 qz 不等于 yaw 本身；例如 yaw=90° 时 qz 约为 0.7071，而不是 90，也不是 π/2。这组简式只适用于纯偏航，不能用于同时存在 roll、pitch 的一般姿态。

**4. 同一位置可以有不同朝向；同一旋转也可能有不同符号表示。**

机器人位置都为 `(1, 2, 0)` 时，可以朝参考系正 x，也可以朝正 y。原地转向改变朝向，不要求位置跟着旋转到别处；若要旋转位置向量或转换参考系，需要另外执行相应变换，不能把这两种操作混为一谈。

单位四元数 q 与四个分量同时取反后的 -q 表示同一旋转。因此两个消息的四元数符号不同，不一定表示姿态发生了跳变。归一化只能修正合适的非零四元数的模长，不能纠正分量顺序、角度单位或参考系错误；不能只检查模长就断定姿态正确。

官方 [Pose 消息定义](https://github.com/ros2/common_interfaces/blob/humble/geometry_msgs/msg/Pose.msg)把位姿分为 `position` 与 `orientation`。Pose 本身不带 Header，参考系和时间需要由 PoseStamped、Odometry 等外层消息或接口约定说明。TF 中对应的字段是 `translation` 与 `rotation`。

### 板书

```text
位姿 = 位置 + 朝向（两部分组成，不是直接做加法）
位置：position(x, y, z)，单位 m
朝向：orientation(qx, qy, qz, qw)，单位模、无量纲

纯 yaw：q = [0, 0, sin(yaw/2), cos(yaw/2)]
0°   → [0, 0, 0, 1]
90°  → [0, 0, 0.7071, 0.7071]
180° → [0, 0, 1, 0]

先查：参考系 → 分量顺序 → 角度单位 → 单位模
```

### 实验：用官方 TF 工具核对位置与四元数

**实验目标：**固定平移，分别观察 0°、90°、180° 的偏航输出，再核对弧度输入与四元数输入的等价关系。

来源：Humble 官方 [静态 TF 工具教程](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Intermediate/Tf2/Writing-A-Tf2-Static-Broadcaster-Py.rst)及上述四元数基础教程。实验采用官方可执行工具，名称与数值是课堂设置。这里用静态 TF 展示三个固定姿态，不模拟机器人动态转动，也不向实机发指令。

**准备条件：**Ubuntu 22.04 或 WSL Ubuntu 22.04，已安装 ROS 2 Humble，无需图形桌面。使用 Ubuntu Bash：

```bash
sudo apt update
sudo apt install ros-humble-tf2-ros
```

结束此前的发布进程。打开终端 A、B，**两个终端都执行**以下环境设置；若 99 已用于其他实验，统一换用空闲 Domain ID。

```bash
source /opt/ros/humble/setup.bash
export ROS_DOMAIN_ID=99
export ROS_LOCALHOST_ONLY=1
```

**① 终端 A 先发布位于 `(1, 2, 0)`、偏航为零的固定姿态。**

```bash
ros2 run tf2_ros static_transform_publisher \
  --x 1.0 --y 2.0 --z 0.0 \
  --roll 0.0 --pitch 0.0 --yaw 0.0 \
  --frame-id lesson_odom --child-frame-id lesson_base
```

终端 B 查询：

```bash
ros2 run tf2_ros tf2_echo lesson_odom lesson_base
```

预期平移为 `[1, 2, 0]`，四元数为 `[0, 0, 0, 1]`。这表示 lesson_base 原点在 lesson_odom 中的位置与朝向；两个名称只是实验坐标系，不代表真实里程计已经运行。

**② 保持位置不变，依次观察 90° 与 180°。**

每轮先在 B、A 中依次按 `Ctrl+C`，停止查询和发布，再启动下一组姿态。**每轮都重新启动 B 的查询进程**，避免把修改静态关系与已有缓存混在一起。

A 的第二轮命令：

```bash
ros2 run tf2_ros static_transform_publisher \
  --x 1.0 --y 2.0 --z 0.0 \
  --roll 0.0 --pitch 0.0 --yaw 1.5707963267948966 \
  --frame-id lesson_odom --child-frame-id lesson_base
```

A 的第三轮命令：

```bash
ros2 run tf2_ros static_transform_publisher \
  --x 1.0 --y 2.0 --z 0.0 \
  --roll 0.0 --pitch 0.0 --yaw 3.141592653589793 \
  --frame-id lesson_odom --child-frame-id lesson_base
```

B 每轮重新执行第①步的查询。预期位置始终为 `[1, 2, 0]`，旋转分别对应表中的 90° 与 180°。输出精度可能只显示三位小数；180° 附近的 yaw 也可能显示为等价的 -180°，应结合四元数与朝向理解，不要求字符串完全相同。

**③ 用四元数输入复现 90°。**

再次停止 B、A，然后在 A 中执行下面的命令，B 重新查询：

```bash
ros2 run tf2_ros static_transform_publisher \
  --x 1.0 --y 2.0 --z 0.0 \
  --qx 0.0 --qy 0.0 --qz 0.7071067811865475 --qw 0.7071067811865476 \
  --frame-id lesson_odom --child-frame-id lesson_base
```

预期与第二轮的 90° 姿态等价，平移仍为 `[1, 2, 0]`。这两种输入方式分别由工具完成欧拉角转换，或直接提供四元数；不要在同一条命令中混用两套旋转参数。

**验收：**记录三个偏航特例的平移与四元数；用半角关系解释 90° 的两个 0.7071；指出改变朝向为什么没有改变本实验的位置，并说明四元数输入与角度输入如何对应。

**排查与结束：**结果不符时，先查命令中的弧度、父子坐标系顺序和四元数分量顺序；确认每轮旧进程已停止、查询已重启，没有重复发布者。微小浮点误差或四元数整体变号不能直接判为姿态错误。结束时 B、A 依次按 `Ctrl+C` 退出。

### 课堂提问

> 四元数里有四个分量，为什么不表示四个旋转自由度？

参考答案：表示旋转时它们满足单位模约束，仍描述三维旋转；而且 q 与 -q 表示同一旋转。

> yaw 为 90° 时，为什么 qz 不是 1？

参考答案：纯偏航使用半角关系，qz=sin(45°) 约为 0.7071，同时 qw=cos(45°) 约为 0.7071。qz=1、qw=0 对应的是 180°。

> 机器人位置为 `(1, 2, 0)`，朝向从 0° 改成 90°，位置一定也要改变吗？

参考答案：不需要。位姿中的位置和朝向分别描述原点所在处与坐标轴朝向；原地转向可以保持原点位置不变。

这页最后可以总结：

> **位置说明原点在哪里，四元数说明朝向怎样；核对参考系、分量顺序、弧度和单位模，才能把位姿消息读成正确的空间含义。**

---

## 第 16 页｜欧拉角 vs 四元数

这页你可以把它讲成：

> **欧拉角便于人理解，四元数便于程序表达和组合旋转。两者描述的是同一个朝向，但转换前必须统一旋转约定。不能把“绕哪三根轴”当成全部信息，还要说明轴是否随物体转动、旋转顺序怎样。遇到万向节锁或角度跳变时，也要先判断是表示方式的问题，还是真实姿态发生了变化。**

### 讲解与例子

**1. 先明确本页的旋转约定，再比较两种表示。**

本课使用与 tf2 `setRPY(roll, pitch, yaw)` 一致的固定轴 RPY：roll 绕 x，pitch 绕 y，yaw 绕 z，输入单位为 rad。采用列向量描述时，对应旋转矩阵写作：

```text
R = Rz(yaw) · Ry(pitch) · Rx(roll)
```

矩阵作用于向量时，右侧先作用，所以这对应依次绕固定的 x、y、z 轴旋转；它也等价于按 z、y、x 顺序绕随动轴进行内禀旋转。PPT 的“ZYX 顺序”需要连同这个约定一起讲，不能只看三个字母就把函数参数改成 `(yaw, pitch, roll)`。

旋转顺序通常不可交换。可以让学生拿一本书，分别尝试先绕固定 x 轴转、再绕固定 y 轴转，然后交换顺序，对比书的最终朝向。这里只比较朝向，不要求移动书的参考原点。

依据：Humble 官方 [四元数基础教程中的固定轴 RPY 说明](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Intermediate/Tf2/Quaternion-Fundamentals.rst)，以及 [tf2 Quaternion 的 setRPY 实现](https://github.com/ros2/geometry2/blob/humble/tf2/include/tf2/LinearMath/Quaternion.hpp)。

**2. 万向节锁影响的是欧拉角表示，不是机器人突然失去运动能力。**

| 对比项 | 本页采用的欧拉角 RPY | 单位四元数 |
| --- | --- | --- |
| 直观性 | 便于说明侧倾、俯仰和偏航 | 四个分量难以直接读出朝向 |
| 约束与约定 | 必须明确轴、顺序和角度单位 | 必须明确分量顺序、单位模与旋转含义 |
| 奇异性 | pitch=±90° 时，roll 与 yaw 不能独立、唯一确定 | 表示旋转时没有这类欧拉角奇异性 |
| 旋转组合 | 不能直接把三组角度逐项相加 | 可以按明确的顺序做四元数乘法 |
| 插值 | 逐项平均可能走错路径或跨越角度边界 | 可用 SLERP 等旋转插值方法 |

PPT 中“roll 与 yaw 轴重合、丢失自由度”可以用嵌套转轴帮助理解；在数学上，更准确地说是该姿态附近的三个角不再提供独立的参数方向。刚体仍可在三维空间旋转，只是这套角度表示退化了。

例如，按本页约定，`(roll, pitch, yaw) = (0°, 90°, 0°)` 与 `(30°, 90°, 30°)` 表示同一个旋转。把四元数再转换回欧拉角时，软件只能选择某一组等价角度；不应要求它原样返回输入的三个数。相关说明可参阅 [SciPy 官方欧拉角文档](https://docs.scipy.org/doc/scipy/reference/generated/scipy.spatial.transform.Rotation.as_euler.html)。

**3. 看懂转换公式中的半角、符号与输出顺序。**

令 `r=roll`、`p=pitch`、`ψ=yaw`，均为弧度，先计算：

```text
cr = cos(r/2)    sr = sin(r/2)
cp = cos(p/2)    sp = sin(p/2)
cy = cos(ψ/2)    sy = sin(ψ/2)
```

与 PPT 等价的四元数公式，按 ROS 消息的 `[qx, qy, qz, qw]` 顺序整理为：

```text
qx = cy·cp·sr - sy·sp·cr
qy = sy·cp·sr + cy·sp·cr
qz = sy·cp·cr - cy·sp·sr
qw = cy·cp·cr + sy·sp·sr
```

PPT 把 w 写在公式第一行，不表示 ROS 消息的分量顺序改成了 `[w, x, y, z]`。另外，式中的 `cy、sy` 来自 yaw，`qy` 却是四元数的 y 分量，两种“y”不要混淆。

令 r=p=0，就会退化为上一页的纯偏航公式 `[0, 0, sin(ψ/2), cos(ψ/2)]`，可以作为检查符号与半角的简单办法。包含三轴旋转时，例如 `(30°, 20°, 40°)`，结果约为：

```text
[qx, qy, qz, qw] = [0.182148, 0.244792, 0.283114, 0.909255]
```

开发中优先使用经过验证的库接口，例如 C++ 的 `setRPY(roll, pitch, yaw)`，再按消息字段转换；不要凭名字把不同的 Euler 接口当作同一种约定。检查单位模有助于发现错误，但不能代替核对顺序、参考系和单位。

**4. “插值平滑”也需要选对方法与等价表示。**

纯偏航从 +179° 到 -179°，两者实际只差约 2°；若直接平均两个角度会得到 0°，可能绕远路。这个角度边界问题与 pitch=±90° 的万向节锁不是同一个问题。

SLERP 可以在两端姿态之间沿最短旋转路径插值，但要正确处理 q 与 -q 的等价性，不能直接逐分量平均。两关键帧之间的旋转插值，也不自动保证多段轨迹的角速度、加速度连续或满足机器人运动约束。相关行为参考 [SciPy 官方 SLERP 说明](https://docs.scipy.org/doc/scipy/reference/generated/scipy.spatial.transform.Slerp.html)。

本页的开发习惯可以概括为：用 RPY 或坐标轴理解与检查朝向，程序中按接口约定使用单位四元数处理旋转。效率取决于具体运算，不能简单认为任何场景下四元数计算都一定更快。

### 板书

```text
固定轴 RPY：roll(X)、pitch(Y)、yaw(Z)，输入 rad
R = Rz(yaw) · Ry(pitch) · Rx(roll)
setRPY 的参数顺序仍是 roll、pitch、yaw

pitch=±90°：欧拉角表示退化，刚体旋转能力没有消失
q 与 -q：同一旋转；转换回 RPY 不保证恢复原输入

人读方向，程序按约定计算：查轴与顺序 → 查单位 → 查分量 → 查单位模
```

### 实验：用官方 TF 工具核对转换与万向节锁

**实验目标：**验证一个三轴旋转的四元数，并观察两组不同欧拉角如何表示同一奇异姿态。

来源：Humble 官方 [static_transform_publisher 的 RPY 输入示例](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Intermediate/Tf2/Writing-A-Tf2-Static-Broadcaster-Py.rst)及上述四元数教程。使用官方可执行工具，角度与 frame 名称是课堂设置；用静态 TF 展示固定朝向，不模拟实机运动。

**准备条件：**Ubuntu 22.04 或 WSL Ubuntu 22.04，已安装 ROS 2 Humble，无需图形桌面。使用 Ubuntu Bash 安装依赖：

```bash
sudo apt update
sudo apt install ros-humble-tf2-ros
```

结束此前的发布进程。打开终端 A、B，**两个终端都执行**以下设置；若 99 已用于其他实验，统一换用空闲 Domain ID。

```bash
source /opt/ros/humble/setup.bash
export ROS_DOMAIN_ID=99
export ROS_LOCALHOST_ONLY=1
```

**① 终端 A 输入 roll=30°、pitch=20°、yaw=40°。**

先将三个角分别换为弧度：

```bash
ros2 run tf2_ros static_transform_publisher \
  --x 0.0 --y 0.0 --z 0.0 \
  --roll 0.5235987755982988 --pitch 0.3490658503988659 --yaw 0.6981317007977318 \
  --frame-id lesson_world --child-frame-id lesson_body
```

终端 B 查询：

```bash
ros2 run tf2_ros tf2_echo lesson_world lesson_body
```

预期四元数约为 `[0.182, 0.245, 0.283, 0.909]`；非奇异情况下，RPY 输出应对应约 30°、20°、40°。输出通常经过小数位舍入，不要直接用屏幕上的三个小数位要求单位模精确等于 1。

**② 观察 pitch=90° 的第一组输入。**

先在 B、A 中依次按 `Ctrl+C`，停止旧查询和发布。A 改为：

```bash
ros2 run tf2_ros static_transform_publisher \
  --x 0.0 --y 0.0 --z 0.0 \
  --roll 0.0 --pitch 1.5707963267948966 --yaw 0.0 \
  --frame-id lesson_world --child-frame-id lesson_body
```

B 重新执行第①步的查询，记录四元数。预期约为 `[0, 0.707107, 0, 0.707107]`；在奇异姿态附近，RPY 的具体输出分支受实现和数值误差影响，重点记录四元数表示的朝向。

**③ 换成另一组表示同一姿态的输入。**

再次停止 B、A，A 改为 roll=30°、pitch=90°、yaw=30°：

```bash
ros2 run tf2_ros static_transform_publisher \
  --x 0.0 --y 0.0 --z 0.0 \
  --roll 0.5235987755982988 --pitch 1.5707963267948966 --yaw 0.5235987755982988 \
  --frame-id lesson_world --child-frame-id lesson_body
```

B 重新启动查询。预期与第②步的旋转相同，四元数数值相同或在浮点误差范围内等价，也允许整体符号相反。若转换回 RPY 时没有得到 `(30°, 90°, 30°)`，不能据此判断四元数转换失败。

**验收：**对照公式核对第①步的三轴旋转；记录第②、③步的四元数并解释其等价性；说明这与上一页纯 yaw 的半角公式有何联系，以及为什么奇异点不能只用“输入输出三个角相同”作为验收条件。

**排查与结束：**先核对固定轴约定、参数顺序、弧度单位和查询方向；每轮都重启查询，并确认只有一个发布者维护该关系，避免静态缓存与重复发布干扰。涉及旋转比较时应考虑 q 与 -q 的等价性和浮点误差。结束时 B、A 依次按 `Ctrl+C` 退出。

### 课堂提问

> PPT 写 ZYX，为什么函数仍然调用 setRPY(roll, pitch, yaw)？

参考答案：旋转矩阵乘积、固定轴或随动轴的描述顺序，与函数签名是不同层面的约定。本页固定轴 RPY 对应 `Rz·Ry·Rx`，函数参数按 roll、pitch、yaw 传入。

> 四元数没有万向节锁，为什么转回欧拉角仍可能出现不稳定或不唯一的角度？

参考答案：转换回去仍然使用欧拉角参数化，它在奇异姿态处的限制没有消失；应比较旋转本身，而非要求恢复唯一的三角组合。

> +179° 与 -179° 直接平均得到 0°，为什么可能不适合姿态插值？

参考答案：数值平均忽略了角度的周期性，两个端点实际相邻。需要按正确的旋转路径插值，不能把角度当作普通无界标量处理。

这页最后可以总结：

> **欧拉角便于表达意图，四元数便于处理旋转；统一轴、顺序和单位，再比较实际朝向，才能正确完成转换与插值。**
