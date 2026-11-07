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
