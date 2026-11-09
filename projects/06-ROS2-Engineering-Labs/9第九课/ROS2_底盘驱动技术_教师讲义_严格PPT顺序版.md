# ROS 2 底盘驱动技术：逐页教师讲

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

---

## 第 17 页｜为什么需要统一的 TF 系统

这页你可以把它讲成：

> **一台机器人有驱动、定位、导航、传感器和可视化等多个模块。如果每个模块都抄一份安装参数、写一套旋转与时间处理，修改一次传感器位置就可能要改很多地方。TF 把坐标关系按统一约定发布出来，由需要它的模块按参考系和时间查询，让关系的维护者与使用者各自负责清楚的一部分工作。**

### 讲解与例子

**1. 先让学生设想：三个模块各自保存一份传感器安装参数。**

例如，避障模块把传感器前移量写为 0.10 m，可视化模块写为 0.12 m，建图模块还沿用更换支架前的安装角。同一个观测点送进三个模块，得到的位置就可能不同。即使每份公式本身都正确，参数不同也会造成不一致。

| PPT 中的问题 | 具体表现 | 统一 TF 后的分工 |
| --- | --- | --- |
| 重复实现与维护 | 每个节点自行写旋转、求逆、链式组合和边界处理 | 消费端使用经过验证的 tf2 接口处理变换 |
| 参数或约定不一致 | 安装位置、轴向、单位、旋转顺序各写一份 | 为每条关系明确维护来源，并统一发布给消费者 |
| 时间处理复杂 | 直接拿最新姿态转换旧传感器数据，或各自保存历史 | 按数据时间查询 TF 缓存，在可用范围内完成插值 |

统一系统降低了重复实现和维护的成本，但不会自动发现安装角写错、单位填错或标定不准确。所有消费者都得到一致的错误参数，结果仍然会一致地出错。

**2. “统一变换服务”首先是统一接口，不必是一台中心服务器。**

常见用法是：发布者把动态与静态关系分别广播到 `/tf`、`/tf_static`；消费者用 TransformListener 接收，再写入自己的 Buffer。应用随后查询本地 Buffer，而不是每次都向某个中心节点发起 ROS Service 请求。

只要坐标系之间存在完整路径，并且相关数据已收到、查询时间有效，tf2 就能沿链组合或求逆。不需要为每个使用者再发布一条直接连接，也不需要应用自己硬编码完整的矩阵乘积。

所以 PPT 的“任意两坐标系”要带上条件：坐标系存在、路径连通、动态边在该时刻有可用数据。只创建一个空 Buffer，或者只填写消息的 `frame_id`，并不能完成查询。

依据：Humble 官方 [tf2 概念说明](https://github.com/ros2/ros2_documentation/blob/humble/source/Concepts/Intermediate/About-Tf2.rst)及 [Python 监听者教程](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Intermediate/Tf2/Writing-A-Tf2-Listener-Py.rst)。

**3. 统一时间查询，仍需要准确的时间戳与时钟基准。**

一帧传感器消息记录的是时刻 t 的观测，通常应使用 t 时刻的坐标关系转换。tf2 帮助消费者保存和查询带时间的变换，在有效范围内插值；它不会让相机、IMU 和编码器自动在同一时刻采样，也不会自行修正多机时钟偏差。

PPT 提到 Cartographer 等算法依赖这种能力，可以解释为算法处理带时间的数据时需要对应的坐标关系；不能理解成只要有 TF，所有传感器数据就已经完成同步。时间戳来源、时钟同步与消息配对，仍需要相应的驱动、系统或算法处理。

上一页讨论的外推错误也仍然存在：如果某条动态边没有 t 时刻的可用数据，树即使连通，查询也可能失败。不能通过把旧消息时间改成现在来代替正确的时间处理。

**4. “谁的数据谁广播”，落到工程上是每条关系有明确的负责者。**

本课可以把驱动提供的里程计关系 `odom → base_footprint` 作为例子：需要车体在 odom 中位置的模块消费这条关系，不再各自重新积分一份轮速来拼出不同的 TF。

若项目改为由融合估计节点发布这条关系，就应调整驱动的 TF 发布配置，避免二者同时维护同一子坐标系。传感器的固定安装关系同样应从明确的模型或标定配置提供，不要同时在多个 launch 文件中各写一份。

这里减少的是重复的数据来源和重复实现，不代表整个系统只计算一次。每个消费者仍可能维护缓存、做链式查询，并将自己的点或位姿转换到目标参考系；统一 TF 不会让这些必要运算消失。

### 板书

```text
状态 / 安装关系的负责者
        ↓ 广播 /tf、/tf_static
各消费者的 TransformListener → 各自的 Buffer → 应用查询与数据转换

减少重复实现：复用 tf2 的组合、求逆与时间查询
减少参数分叉：每条关系明确来源，消费者按约定使用
查询条件：有 frame + 有路径 + 有数据 + 时间有效

统一接口 ≠ 默认中心服务器
时间查询 ≠ 自动同步采样；一致参数 ≠ 参数必然正确
```

### 实验：发布者只给出两条关系，消费者完成跨链转换

**实验目标：**通过两条 TF 把传感器坐标中的一个点转换到世界参考，再反向还原，体会消费者无需硬编码安装参数与矩阵计算。

来源：Humble 官方 [静态 TF 工具教程](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Intermediate/Tf2/Writing-A-Tf2-Static-Broadcaster-Py.rst)、上述监听者教程，以及 [Buffer 数据转换接口](https://github.com/ros2/geometry2/blob/humble/tf2_ros_py/tf2_ros/buffer_interface.py)。发布端采用官方工具；消费端是按官方接口编写的最小课堂练习，**不是官方完整示例程序**。

本实验建立一个固定场景 `lab_world → lab_base → lab_sensor`。数值是课堂示意，不是实机标定；用静态关系方便观察几何结果，不把真实底盘的运动关系改成静态 TF。

**准备条件：**Ubuntu 22.04 或 WSL Ubuntu 22.04，已安装 ROS 2 Humble，无需图形桌面。使用 Ubuntu Bash：

```bash
sudo apt update
sudo apt install ros-humble-tf2-ros ros-humble-tf2-ros-py \
  ros-humble-tf2-geometry-msgs ros-humble-geometry-msgs
```

结束此前的发布进程。打开终端 A、B、C，**三个终端都执行**以下设置；若 99 已用于其他实验，统一换用空闲 Domain ID。

```bash
source /opt/ros/humble/setup.bash
export ROS_DOMAIN_ID=99
export ROS_LOCALHOST_ONLY=1
```

**① 终端 A 发布世界参考到车体参考的关系。**

```bash
ros2 run tf2_ros static_transform_publisher \
  --x 1.0 --y 2.0 --z 0.0 \
  --roll 0.0 --pitch 0.0 --yaw 1.5707963267948966 \
  --frame-id lab_world --child-frame-id lab_base
```

**② 终端 B 发布车体参考到传感器参考的关系。**

```bash
ros2 run tf2_ros static_transform_publisher \
  --x 0.5 --y 0.0 --z 0.1 \
  --roll 0.0 --pitch 0.0 --yaw 0.0 \
  --frame-id lab_base --child-frame-id lab_sensor
```

A、B 均保持运行。传感器原点在车体系中前移 0.5 m、上移 0.1 m；车体相对 lab_world 左转 90°，所以传感器原点在 lab_world 中位于约 `(1, 2.5, 0.1)`。

**③ 终端 C 先核对跨链关系。**

```bash
ros2 run tf2_ros tf2_echo lab_world lab_sensor
```

预期平移约为 `[1, 2.5, 0.1]`，旋转为正 90° 偏航。观察后按 `Ctrl+C`，再运行下面的消费端。它处理的是一个构造的几何点，不是真实传感器测量。

```bash
python3 - <<'PY'
import time
import rclpy
import tf2_geometry_msgs  # Register PointStamped transformations.
from geometry_msgs.msg import PointStamped
from rclpy.node import Node
from rclpy.time import Time
from tf2_ros import Buffer, TransformListener

rclpy.init()
node = Node('lesson_tf_consumer')
listener = None
try:
    buffer = Buffer()
    listener = TransformListener(buffer, node)
    point = PointStamped()
    point.header.frame_id = 'lab_sensor'
    point.header.stamp = node.get_clock().now().to_msg()
    point.point.x = 1.0
    query_time = Time.from_msg(point.header.stamp)

    deadline = time.monotonic() + 5.0
    while not buffer.can_transform('lab_world', 'lab_sensor', query_time):
        if time.monotonic() >= deadline:
            raise RuntimeError('TF unavailable: check both publishers and terminal environment')
        rclpy.spin_once(node, timeout_sec=0.1)

    world_point = buffer.transform(point, 'lab_world')
    restored = buffer.transform(world_point, 'lab_sensor')
    for label, value in [('world', world_point), ('restored', restored)]:
        p = value.point
        print(f'{label}: frame={value.header.frame_id}, '
              f'point=({p.x:.3f}, {p.y:.3f}, {p.z:.3f})')
finally:
    if listener is not None:
        listener.unregister()
    node.destroy_node()
    rclpy.shutdown()
PY
```

**预期结果：**世界参考中的点约为 `(1, 3.5, 0.1)`，还原到传感器参考后约为 `(1, 0, 0)`，末位可能有浮点误差或负零显示。消费端只声明源坐标系、数据时间与目标坐标系，没有复制前移量、安装高度或 yaw 参数。

可以先手算验证：传感器中的点 `(1, 0, 0)` 加上安装平移，在车体系中为 `(1.5, 0, 0.1)`；再随车体系左转 90°并加上世界系中的车体位置，就得到 `(1, 3.5, 0.1)`。TF 完成同样的链式变换，反向转换则还原输入点。

代码中的 `spin_once` 用于执行监听回调，把收到的 TF 写入 Buffer；只等待而不处理回调，缓存可能始终为空。`import tf2_geometry_msgs` 则注册 PointStamped 的官方转换支持，依据是 [tf2_geometry_msgs 的类型转换实现](https://github.com/ros2/geometry2/blob/humble/tf2_geometry_msgs/src/tf2_geometry_msgs/tf2_geometry_msgs.py)。修改 `frame_id` 字符串本身不会完成坐标转换。

**④ 保持发布者运行，再执行一次消费端。**

新进程会创建自己的 Buffer，通过监听获得相同的安装关系，预期得到相同几何结果。重跑只用于观察独立消费端获取统一关系，不证明不同节点在任意网络条件下都拥有完全相同的缓存内容。

**验收：**记录跨链 TF、世界系中的点与还原结果；指出几何参数由谁提供、消费端如何获得它们，以及为什么不需要再发布 `lab_world → lab_sensor` 这条直接连接。说明本实验的静态场景没有验证动态传感器时间同步。

**排查与结束：**超时先查 A、B 是否运行、frame 名称及各终端环境，再查路径是否完整；缺少消息类型转换支持时，检查依赖和 `tf2_geometry_msgs` 导入。结果不符时核对父子方向、单位及重复发布者，不能只改 frame 标签。消费端会自行结束，最后 B、A 依次按 `Ctrl+C` 退出。

### 课堂提问

> TF 统一了坐标变换，为什么每个消费者仍可能各有一个 Buffer？

参考答案：统一的是发布约定和查询接口，常见实现通过广播让各消费者建立本地缓存，不要求所有查询都经过中心服务器。

> 只要 TF 树连通，带任意时间戳的数据都能转换吗？

参考答案：不能。相关动态边还必须在请求时刻有可用数据，监听者也必须已经收到它们；TF 不会自动补齐缺失历史或同步采样。

> 所有模块使用同一组 TF 后，为什么仍需要检查安装标定？

参考答案：共享关系能减少参数分叉，但不能保证关系本身正确。错误参数也会被一致地使用，仍需核对实际安装与标定。

这页最后可以总结：

> **统一 TF，让关系有明确来源，让消费者按坐标系和时间查询；减少各写一套的维护问题，同时保留对参数、时钟和数据有效性的检查。**

---

## 第 18 页｜运动学与航位推算

这页你可以把它讲成：

> **上一章说明了坐标系之间怎样联系，这一章要回答运动怎样变成位置。底盘反馈“向前每秒走多少”，还不能直接说明它在 odom 中的 x、y 怎样变化；我们还需要车头朝向和经过的时间。对麦克纳姆轮底盘，还要理解四个轮子的转动怎样合成为前进、横移和旋转，最后把这些运动信息连起来，连续估计机器人的位姿。**

### 讲解与例子

**1. 用“同样向前走，为什么位置变化不同”引出旋转矩阵。**

先在黑板上画出固定的 odom 坐标轴，再画一辆车。车头沿 odom 的正 x 方向时，以车体系前向速度 0.2 m/s 匀速走 5 s，理想位移为 odom 中的 `(1, 0)` m；车头左转 90° 后以同样速度走同样时间，理想位移变成 `(0, 1)` m。

速度的数值没变，表达它的坐标轴转了，所以固定参考系中的分量会变。本章后续用平面旋转矩阵完成这一步，再把每个时间间隔的位移累加起来。这里先建立方向直觉，不在章节导入页展开全部矩阵推导。

本课所谓“世界坐标系下的位置”，在底盘里程计环节具体指 **odom 中的局部位置估计**。它连续，但会漂移；接入建图或定位后使用的 map 参考不能与 odom 混为一谈。

**2. 航位推算的起点是初始位姿，输入要有速度、航向和时间。**

航位推算从已知或约定的初始位姿出发，利用后续运动信息逐步估计新的位姿。里程计常把启动处约定为原点，并不因此知道自己在整栋楼里的绝对位置。

可以先写一条计算链：

```text
编码器 / 下位机反馈 → 车体系速度
IMU 姿态估计      → 对齐参考后的车体航向
数据时间戳        → 相邻更新的实际时间间隔
                           ↓
                速度旋转到 odom，再累计位移
                           ↓
                 Odometry + 对应的动态 TF
```

这里应使用反馈或估计的实际运动信息。`cmd_vel` 是期望运动，轮胎打滑、碰到障碍或电机饱和时，执行结果可能不同，不能把“发过指令”当成“确实走了相同距离”。开环积分指令可以作为特定模型的估计方式，但需要明确其假设。

前文提到项目里程计更新目标为 50 Hz，对应标称间隔 0.02 s；实际回调可能有抖动，计算时间间隔仍应遵循项目的数据时间戳设计，不能仅因配置为 50 Hz 就认定每次恰好经过 0.02 s。

ROS 的 [Odometry 官方消息定义](https://github.com/ros2/common_interfaces/blob/humble/nav_msgs/msg/Odometry.msg)也区分两种表达：pose 属于 `header.frame_id`，twist 属于 `child_frame_id`。本章的速度转换与位移累计正是在落实这种区别。

**3. 为什么使用 IMU 融合航向，而不是在驱动里再积一遍原始角速度？**

若陀螺仪在静止时仍有 0.01 rad/s 的残余零偏，直接积分 60 s 就会多出约 0.6 rad，即 34.4° 的朝向误差。这组数字是说明累计误差的课堂假设，不是本项目 IMU 的实测指标。

PPT 给出的项目思路是利用下位机已经输出的融合姿态，从中获得航向，避免在上位机用原始角速度另建一套未经同样处理的航向积分。使用前仍要核对姿态是否有效、轴向与安装关系是否正确、启动参考如何与 odom 对齐，以及姿态与速度的时间是否对应。

**融合姿态仍是估计值。**本课平台前文列的是六轴 IMU；加速度计提供的重力方向有助于约束横滚与俯仰，单靠重力不能确定绝对偏航。没有外部航向约束时，yaw 仍可能漂移，不能把“融合值”讲成“永不漂移的绝对航向”，也不能仅凭这一页认定具体融合算法。

依据：ROS 的 [REP-145 IMU 驱动约定草案](https://github.com/ros-infrastructure/rep/blob/master/rep-0145.rst)说明了姿态参考与缺少绝对 yaw 参考时的限制；[Humble Imu 消息定义](https://github.com/ros2/common_interfaces/blob/humble/sensor_msgs/msg/Imu.msg)规定了姿态不可用时的标记。消息里存在 orientation 字段，不等于传感器已经提供有效的融合姿态。

**4. 麦克纳姆轮把四个轮速与平面三种运动联系起来。**

斜置滚子及四轮的安装组合，使底盘在理想接触条件下可以合成前后运动、左右横移和绕 z 轴旋转；横移时不必先把车头转向目标方向。向学生强调：滚子方向、轮序、正转定义和底盘尺寸必须与公式匹配，不能先背一套正负号再直接套到所有底盘上。

| 本章的问题 | 需要建立的联系 | 课堂上先记住什么 |
| --- | --- | --- |
| 车速怎样变成位置 | 车体系速度 → odom 速度 → 累计位移 | 朝向和时间缺一不可 |
| 航向从哪里来 | 原始角速度与融合姿态的关系 | 融合不等于消除全部漂移 |
| 四轮怎样实现全向运动 | 四个轮速与 `(vx, vy, wz)` 的运动学映射 | 横移、前进、旋转可以组合 |

轮速求车体速度是正运动学，期望车体速度求各轮目标速度是逆运动学。官方 [ros2_control 麦克纳姆轮控制器说明](https://control.ros.org/rolling/doc/ros2_controllers/mecanum_drive_controller/doc/userdoc.html)可作为这种输入输出关系的补充阅读；该链接是 Rolling 开发版资料，本页只引用其运动学接口思路，不作为本项目 Humble 驱动的安装或配置步骤。

### 板书

```text
本章三问：速度怎样变位置？航向怎样获得？四轮怎样合成运动？

前向速度 0.2 m/s，持续 5 s：
朝向 0°   → odom 位移约 (1, 0) m
朝向 90°  → odom 位移约 (0, 1) m

航位推算：初始位姿 + 运动信息 + 时间 → 连续的位姿估计
cmd_vel 是期望；odom 是估计；二者都不能保证等于真实运动
麦轮平面运动：vx、vy、wz；六轴融合 yaw 仍可能漂移
```

### 导入实验：用官方 turtlesim 对比两个朝向下的前进

**实验目标：**保持前进指令相同，只改变初始朝向，观察固定参考中的位置沿哪条轴变化，建立“先转换方向，再累计位移”的直觉。

来源：ROS 2 Humble 官方 [话题教程](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Beginner-CLI-Tools/Understanding-ROS2-Topics/Understanding-ROS2-Topics.rst)与[服务教程](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Beginner-CLI-Tools/Understanding-ROS2-Services/Understanding-ROS2-Services.rst)。使用官方 turtlesim、话题发布与服务调用工具，朝向对比是本页安排的课堂步骤。

**准备条件：**Ubuntu 22.04 或带可用图形显示的 WSL Ubuntu 22.04，已安装 ROS 2 Humble。结束之前的实验进程，在 Ubuntu Bash 中安装：

```bash
sudo apt update
sudo apt install ros-humble-turtlesim
```

打开终端 A、B、C，**三个终端都执行**下列设置；若 99 已用于其他实验，统一改用空闲 Domain ID。

```bash
source /opt/ros/humble/setup.bash
export ROS_DOMAIN_ID=99
export ROS_LOCALHOST_ONLY=1
```

**① 终端 A 启动仿真，终端 B 持续查看位姿。**

终端 A：

```bash
ros2 run turtlesim turtlesim_node
```

终端 B：

```bash
ros2 topic echo /turtle1/pose
```

**② 终端 C 把初始位置设为 `(5.5, 5.5)`，朝向设为 0，再发一次前进指令。**

```bash
ros2 service call /turtle1/teleport_absolute turtlesim/srv/TeleportAbsolute \
  "{x: 5.5, y: 5.5, theta: 0.0}"
```

等终端 B 显示初始位置和朝向已更新，再执行：

```bash
ros2 topic pub --once /turtle1/cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: 0.5}, angular: {z: 0.0}}"
```

记录运动期间的位姿变化，等 `linear_velocity` 回到 0、位置稳定后记录终点。预期 x 增加，y 基本不变，theta 接近 0。

**③ 回到相同起点，改为朝向正 90°，再发完全相同的前进指令。**

```bash
ros2 service call /turtle1/teleport_absolute turtlesim/srv/TeleportAbsolute \
  "{x: 5.5, y: 5.5, theta: 1.5707963267948966}"
```

仍先等 B 确认初始状态，然后执行：

```bash
ros2 topic pub --once /turtle1/cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: 0.5}, angular: {z: 0.0}}"
```

预期这次 y 增加，x 基本不变，theta 接近 1.571 rad。以 `/turtle1/pose` 的数值判断轴向，不用显示屏像素向下的方向代替仿真的正 y 方向。

**观察说明：**Humble [turtlesim 源码](https://github.com/ros/ros_tutorials/blob/humble/turtlesim/src/turtle.cpp)按朝向转换线速度并更新位置，并在约 1 s 未收到新指令时将速度清零。因此单次消息只产生短时运动；`--once` 表示只发布一条消息，不表示移动固定距离。该实验验收运动方向，不要求终点精确相等，也不把仿真中的长度直接当作 NanoOmni 的实测米数。

本实验观察的是仿真器内部的平面运动更新；`/turtle1/pose` 不是 `nav_msgs/Odometry`，这里也没有编码器、IMU 或真实麦克纳姆轮，不能据此验证实机航位推算精度和轮速模型。

**验收：**提交两组起点与终点记录，分别计算 Δx、Δy，解释为何相同 `linear.x` 会对应不同的位移方向；再口头回答：若航向估计偏了，转换后的运动方向会受到什么影响？

**排查与结束：**窗口无法显示时先检查桌面或 WSL 图形环境；服务一直等待时检查 A 是否运行、三个终端环境是否一致；运动方向异常时确认初始朝向已更新，并关闭其他 teleop 或速度发布进程。实验后在 C 发送零速度，再对 B、A 依次按 `Ctrl+C`：

```bash
ros2 topic pub --once /turtle1/cmd_vel geometry_msgs/msg/Twist "{}"
```

### 课堂提问

> 已知车体系前向速度，为什么还不能直接更新 odom 的 x？

参考答案：还需要车体相对 odom 的朝向及时间间隔。前进方向随车头旋转，可能主要改变 odom 的 y，或同时改变 x、y。

> 已经使用融合后的 IMU 姿态，为什么还要讨论漂移和参考对齐？

参考答案：融合姿态受观测条件与误差影响，六轴 IMU 单靠重力不能确定绝对 yaw；其初始参考和安装轴向也不一定直接对应项目的 odom 与车体系。

> 麦克纳姆轮能横移，是否意味着轮速推算的位置一定准确？

参考答案：不能。运动学描述理想几何关系，打滑、轮径误差、滚子接触与时间误差等仍会影响实际运动和位置估计，需要后续校准与外部观测修正。

这页最后可以总结：

> **先把轮速联系到车体运动，再结合航向转换速度、按时间累计位移；理解这条链，才能判断里程计为什么能工作、又为什么会产生误差。**

---

## 第 19 页｜车体速度 → 世界速度：旋转矩阵

这页你可以把它讲成：

> **前面已经知道，同样一句“向前走”，车头朝向不同，在 odom 中的位置变化方向就不同。这一页把这个直觉写成公式：先看车体的前方和左方在 odom 中分别指向哪里，再把前向、侧向两个速度按这两个方向相加。旋转矩阵的每一列都有几何意义，特别是侧向速度前的负号，可以画图推出来，不需要死记。**

### 讲解与例子

**1. 先固定符号与适用条件，再写公式。**

| 符号 | 本页含义 | 单位与方向 |
| --- | --- | --- |
| `Vx`、`Vy` | 同一车体参考点的线速度在车体系中的分量 | m/s；正 x 向前，正 y 向左 |
| `x`、`y` | 该参考点在 odom 中的位置 | m；相对于 odom 原点 |
| `θ` | 车体系相对 odom 的偏航角 | rad；从 odom 正 x 轴转到车体正 x 轴，逆时针为正 |
| `ω` | 车体平面偏航角速度 | rad/s；绕正 z 轴为正 |

PPT 中的“世界系”在这里具体使用 odom。odom 是固定的局部参考，不能直接等同于没有误差的地图真值。车体系随机器人移动、旋转；本项目中 `base_link` 与 `base_footprint` 只有前文所述的固定高度差、轴向一致，因此在平面模型里可使用相同的 x、y 方向和 yaw。

本页采用平面运动近似：忽略高度变化、roll 和 pitch，以 `(x, y, θ)` 表示位姿。斜坡或明显侧倾时，应重新评估这种近似。轴向、右手系及弧度单位参考官方 [REP-103](https://github.com/ros-infrastructure/rep/blob/master/rep-0103.rst)。

PPT 说“传感器测量的是车体系速度”，在这里应理解为**进入这一步计算的反馈已按约定表达在车体系中**。编码器先输出轮转信息，需要运动学换算；IMU 也不是直接给出可靠的车体平移速度。不同设备的原始数据不能未经处理就当作 `Vx`、`Vy`。

**2. 旋转矩阵的第一列来自车体前方，第二列来自车体左方。**

在 odom 中画一条与正 x 轴夹角为 θ 的箭头，它就是车体正 x 方向的单位向量。把它投影到 odom 两条轴上，得到：

```text
车体正 x 方向：ex = (cosθ, sinθ)
```

车体正 y 方向在正 x 方向的左侧，即再逆时针转 90°，所以：

```text
车体正 y 方向：ey = (cos(θ + π/2), sin(θ + π/2))
                  = (-sinθ, cosθ)
```

`Vx` 乘以前方单位向量，`Vy` 乘以左方单位向量，两者相加就是同一个速度在 odom 中的表达：

```text
v_odom = Vx·ex + Vy·ey

[ dx/dt ]   [ cosθ  -sinθ ] [ Vx ]
[ dy/dt ] = [ sinθ   cosθ ] [ Vy ]

dx/dt = Vx·cosθ - Vy·sinθ
dy/dt = Vx·sinθ + Vy·cosθ
dθ/dt = ω                 （平面运动条件下）
```

矩阵的两列分别是车体两条单位轴在 odom 中的表达。`-Vy·sinθ` 的负号来自车体左方的投影；把它误写成加号，就不能正确表达左转后的侧向运动。

这里转换的是**同一个参考点的速度分量**，不需要加上机器人当前的位置 `(x, y)`。如果把速度从传感器安装点换算到另一个车体参考点，转动时还可能涉及参考点偏移产生的速度项，不能把本页的纯旋转直接用于所有异点速度换算。

**3. 用特殊角度核对符号，再看前向与侧向叠加。**

| 车体朝向 θ | odom 中的速度 `(dx/dt, dy/dt)` | 几何解释 |
| --- | --- | --- |
| 0° | `(Vx, Vy)` | 车体轴与 odom 轴对齐 |
| +90° | `(-Vy, Vx)` | 车头朝 odom 正 y，车体左方朝 odom 负 x |
| -90° | `(Vy, -Vx)` | 车头朝 odom 负 y，车体左方朝 odom 正 x |
| 180° | `(-Vx, -Vy)` | 前方与左方都与起始方向相反 |

强调 +90° 这一行：前进使 odom 的 y 增加，而向车体左侧横移使 odom 的 x 减小。只测试 `Vy = 0` 的前进，可能发现不了侧向项写错。

再给一个非直角例子：`Vx = 0.2 m/s`、`Vy = 0.1 m/s`、`θ = 30° = π/6 rad`。代入得到：

```text
dx/dt ≈ 0.2×0.8660 - 0.1×0.5    = 0.1232 m/s
dy/dt ≈ 0.2×0.5    + 0.1×0.8660 = 0.1866 m/s
```

旋转改变分量，不改变速度大小：转换前后均约为 `sqrt(0.2² + 0.1²) = 0.2236 m/s`。这提供了另一个自检方法，但保持模长本身不足以证明旋转方向正确，还要核对特殊角度。

**4. 程序中先算速度，位置更新留给时间积分。**

下面是本页公式的课堂实现，使用 Python 标准库，**不是官方示例源码**：

```python
import math

def body_to_odom(vx, vy, yaw_rad):
    c, s = math.cos(yaw_rad), math.sin(yaw_rad)
    return vx * c - vy * s, vx * s + vy * c

print(body_to_odom(0.2, 0.1, math.radians(30.0)))
```

预期约为 `(0.123205, 0.186603)`，输出仍是 m/s。`math.sin`、`math.cos` 接受弧度，不能直接传入数字 30 来表示 30°。反向把 odom 速度转回车体系时，应使用逆旋转 `R(-θ) = R(θ)ᵀ`，不能继续沿用同方向的旋转。

本页得到的是位置的变化率，乘以适当时间间隔并累计才会得到位置变化；θ 随时间改变时，各次更新应使用对应时刻的航向。公式中的 `dθ/dt = ω` 描述平面运动关系，不意味着项目必须舍弃融合姿态、重新积分原始陀螺仪。

### 板书

```text
约定：x 前、y 左、z 上；θ 从 odom x 轴转到车体 x 轴，逆时针为正

前方 ex = ( cosθ, sinθ)
左方 ey = (-sinθ, cosθ)
v_odom = Vx·ex + Vy·ey

dx/dt = Vx·cosθ - Vy·sinθ
dy/dt = Vx·sinθ + Vy·cosθ

检查：θ=0 → (Vx,Vy)；θ=90° → (-Vy,Vx)；旋转前后模长不变
速度转换不加位置；速度还需结合时间才能累计成位移
```

### 实验：用官方 turtlesim 验证侧向项的负号

**实验目标：**在前进与横移同时存在时，对比 0° 和 +90° 两种朝向，观察位置增量方向，检验公式中的两个速度分量及负号。

来源：Humble 官方 [话题教程](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Beginner-CLI-Tools/Understanding-ROS2-Topics/Understanding-ROS2-Topics.rst)中的命令行发布方法，以及 [turtlesim 官方实现](https://github.com/ros/ros_tutorials/blob/humble/turtlesim/src/turtle.cpp)对 `linear.x`、`linear.y` 和朝向的处理。使用官方程序与工具，下方参数是课堂验证场景。turtlesim 能接收侧向速度，但没有模拟真实麦轮的滚子接触、打滑或 IMU。

**准备条件：**Ubuntu 22.04 或有可用图形显示的 WSL Ubuntu 22.04，已安装 ROS 2 Humble。在 Ubuntu Bash 中执行：

```bash
sudo apt update
sudo apt install ros-humble-turtlesim
```

结束此前实验进程，打开 A、B、C 三个终端，**每个终端都执行**：

```bash
source /opt/ros/humble/setup.bash
export ROS_DOMAIN_ID=99
export ROS_LOCALHOST_ONLY=1
```

若 99 已被其他实验占用，统一使用空闲 Domain ID。终端 A 启动仿真，B 查看位姿：

```bash
# 终端 A
ros2 run turtlesim turtlesim_node
```

```bash
# 终端 B
ros2 topic echo /turtle1/pose
```

**① 在终端 C 设置起点与 0° 朝向，确认 B 的状态更新后再发速度。**

```bash
ros2 service call /turtle1/teleport_absolute turtlesim/srv/TeleportAbsolute \
  "{x: 5.5, y: 5.5, theta: 0.0}"
```

```bash
ros2 topic pub --once /turtle1/cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: 0.4, y: 0.2}, angular: {z: 0.0}}"
```

记录开始与结束的 x、y、theta；等 `linear_velocity` 回到 0 且位置稳定后再进行下一组。预期 x、y 都增加，位移方向满足 `Δy/Δx ≈ 0.5`。

**② 回到相同起点，朝向改为 +90°，确认后重复相同速度。**

```bash
ros2 service call /turtle1/teleport_absolute turtlesim/srv/TeleportAbsolute \
  "{x: 5.5, y: 5.5, theta: 1.5707963267948966}"
```

```bash
ros2 topic pub --once /turtle1/cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: 0.4, y: 0.2}, angular: {z: 0.0}}"
```

预期 x 减小、y 增加，位移方向满足 `Δy/Δx ≈ -2`。本组的负 x 增量来自正的车体侧向速度，直接检验 `-Vy·sinθ` 这一项。

| 组别 | θ | 车体系输入 `(Vx, Vy)` | 固定参考中的理论速度分量 | 预期位置增量符号 |
| --- | --- | --- | --- | --- |
| ① | 0 | `(0.4, 0.2)` | `(0.4, 0.2)` | Δx > 0，Δy > 0 |
| ② | π/2 | `(0.4, 0.2)` | `(-0.2, 0.4)` | Δx < 0，Δy > 0 |

上表使用仿真内部长度与时间单位，不能直接作为实机测量结果。两组运动期间的 `linear_velocity` 应都接近 `sqrt(0.4² + 0.2²) ≈ 0.4472`；朝向转换不改变速率。

**测量说明：**`/turtle1/pose` 给出仿真固定参考中的位姿，并非本项目 `/odom`。由于本实验 `angular.z = 0`、朝向恒定且等待了运动结束，可以用位移方向验证速度方向。不要拿 teleport 前后的跳变当成运动位移，也不要把终点坐标直接当成位移；每组都要减去该组运动开始时的坐标。

`--once` 只发送一条消息；Humble turtlesim 在约 1 s 没有新速度指令后停下，因此不要求精确移动指定距离。若持续旋转，位移方向会随时间变化，不能再用单一初始角度解释整段轨迹。

**验收：**提交两组起终点和 Δx、Δy，核对方向及比值；再手算 `θ = -90°` 时相同输入的理论速度，答案应为 `(0.2, -0.4)`。指出只测试前进、没有侧向速度时，为什么无法覆盖本页的全部符号。

**排查与结束：**服务等待时检查 A 和各终端环境；角度或轨迹不符时确认 teleport 已生效、没有其他速度发布者，且输入使用弧度。观察位姿数值判断 y 正向，不按屏幕像素方向判断。两组之间先等仿真停稳。结束时在 C 发零速度，然后 B、A 依次按 `Ctrl+C`：

```bash
ros2 topic pub --once /turtle1/cmd_vel geometry_msgs/msg/Twist "{}"
```

### 课堂提问

> 为什么旋转矩阵第二列是 `(-sinθ, cosθ)`？

参考答案：它是车体正 y 单位轴在 odom 中的表达，相对车体正 x 轴再逆时针转 90°，由三角函数关系可得。

> 把车体系速度转到 odom 时，为什么不加上车体当前的 `(x, y)`？

参考答案：这里表达的是同一参考点的速度向量，不是点的位置；改变表达坐标轴只需要旋转，位置不能与速度直接相加。

> 速度大小转换前后相同，是否足以证明公式正确？

参考答案：不足。反方向的旋转也保持模长；还要通过 0°、+90° 等已知朝向核对方向，尤其要单独覆盖侧向速度项。

这页最后可以总结：

> **把车体前方和左方投影到 odom，再按各自速度相加，就得到二维旋转矩阵；先核对坐标、方向和单位，再用转换后的速度计算位移。**

---

## 第 20 页｜航位推算（Dead Reckoning）原理

这页你可以把它讲成：

> **上一页把车体速度转成了 odom 中的速度，这一页开始用它更新位置。每次更新只前进一小步：上次的位置，加上这段时间内估计走过的位移。不断重复就能画出轨迹，但输入里的误差也会跟着累计。所以航位推算很适合提供连续的短时运动估计，长期位置是否正确，还需要独立的观测来检验和修正。**

### 讲解与例子

**1. 先把 PPT 的公式读成“一小段位移加回旧位置”。**

```text
位置(t + dt) ≈ 位置(t) + 速度(t) × dt

x[k+1] = x[k] + vx_odom[k] × dt[k]
y[k+1] = y[k] + vy_odom[k] × dt[k]
dt[k]  = t[k+1] - t[k]
```

速度必须与位置使用相同的表达坐标系，因此要先完成上一页的车体系到 odom 的转换。单位也要对应：`m/s × s = m`；如果 dt 是毫秒数，必须先换算为秒。

PPT 的等式适合按“本时间段内速度近似不变”的更新模型理解。真实连续运动中，位移来自这段时间的速度积分；用区间起点速度乘 dt 是一种离散近似，速度或航向在区间内变化明显时会有积分误差。具体积分实现留到相应 PPT 页面展开。

举例：初始位置 `(0, 0)`，odom 中的速度为 `(0.2, 0)` m/s，每次间隔 0.02 s，则每步 x 增加 0.004 m。累计 50 步对应 1 s，理想位置为 `(0.2, 0)` m；不能把“50 次更新”直接理解成“走了 50 个速度数值”。

初始位置是计算的起点。把上电处设为 `(0, 0)`，只是定义局部原点，不代表机器人知道自己在地图或地理坐标中的绝对位置。

**2. 自主、高频、短时可用，都有具体含义。**

| PPT 中的优点 | 课堂上怎样解释 | 需要带上的条件 |
| --- | --- | --- |
| 完全自主、无需外部参考 | 可用车载编码器、IMU 等运动信息持续推算，不依赖 GPS、信标或预建地图才能更新 | 仍然需要传感器、初始状态和有效数据；不等于无需输入 |
| 高频 | 计算量通常较小，可按反馈与实现能力较快更新；本项目目标为 50 Hz | 发布频率不等于独立测量频率，也不等于定位精度 |
| 短时误差较小 | 在标定合理、时间正确、接触条件较好的情况下，适合局部运动估计 | 打滑、错误初值或错误轴向也能使短时间估计明显失准 |
| 不依赖外部设施 | 外部定位暂时不可用时仍能给出连续预测 | 能继续输出，不代表长期位置仍然可靠 |

这里的“高频”要区分收到新数据和重复发布旧数据：驱动以 50 Hz 发布，不会自动创造 50 Hz 的新观测。实际积分需要明确每份反馈对应的时间段，避免漏算或重复累计。

**3. 为什么会“越走越偏”？把误差代入同一条公式。**

假设真实直线速度为 0.20 m/s，反馈却始终为 0.21 m/s，则每秒多算 0.01 m。10 s 后多算 0.10 m，60 s 后多算 0.60 m。轨迹可能非常平滑，方向也看起来正确，却一直偏离真实位置。

| 误差来源 | 怎样影响航位推算 |
| --- | --- |
| 轮径、比例系数或速度零偏 | 距离被持续放大、缩小，或静止时仍被积分出位移 |
| 轮胎、滚子打滑 | 轮子转动不再对应同样的地面位移，反馈运动与实际运动不同 |
| 航向误差 | 速度被投影到错误方向，直行也会产生横向位置误差 |
| 时间间隔错误 | 相同速度乘了错误的时间，造成距离误差或异常跳变 |
| 测量噪声 | 随积分传播到位置估计，使位置不确定性随运动过程变化 |

PPT 中“误差随时间累积”描述长期风险，不表示每次误差的大小都必然单调增加。误差可能部分抵消，也可能在特定运动中迅速放大；随机噪声和固定偏差的表现也不同。

提高更新频率有助于描述更快的运动变化，但不会自动消除固定偏差：在相同总时长内，把 0.01 m/s 的偏差积分成更多小步，累计偏差仍为 `0.01 × 总时长`。

**4. 没有独立参考，平滑轨迹本身无法证明没有漂移。**

仅看上述 0.21 m/s 的反馈，积分器不知道真实速度其实是 0.20 m/s。对同一份数据重复积分、把原点清零或增加输出小数位，都不能找回缺失的真实位置信息。

实际系统需要利用提供独立约束的观测限制长期漂移，例如激光或视觉的环境匹配、回环与地图定位，以及 GPS 等。具体方法依赖可用传感器和场景，不能理解成一定要安装 GPS，也不能认为任意 SLAM 算法在任何环境都能消除漂移。

官方 [REP-105](https://github.com/ros-infrastructure/rep/blob/master/rep-0105.rst)把 odom 定义为连续、适合短时局部使用但会漂移的参考；map 则承担长期参考的角色。常见 TF 分工是保持 `odom → base_footprint` 的连续运动估计，由定位模块更新 `map → odom` 来体现修正，而不是每次定位修正都直接把局部里程计位置跳到新值。

**5. 应用场景的共同点：利用短时连续性，同时管理长期误差。**

短距离运动控制和局部路径跟踪需要连续的运动反馈；SLAM 可以使用里程计作为运动预测或匹配初值，但是否使用、怎样使用要看具体算法与配置。GPS 暂时失效时，也可用航位推算衔接一段时间，并随着不确定性增加调整系统行为。

这些场景都不把航位推算当成永久准确的全局位置来源。其价值在于能持续提供局部运动预测，再与其他观测共同形成更可靠的估计。

### 板书

```text
位置更新：旧位置 + 本段速度 × 本段时长
先转到 odom，再累计；m/s × s = m

50 Hz → 标称 dt = 0.02 s
0.2 m/s × 0.02 s = 0.004 m/步；50 步合计 0.2 m

固定速度偏差 b：位置误差 = b × 总时长
高频 ≠ 高精度；轨迹平滑 ≠ 没有漂移

odom：连续的短时局部估计
独立观测：检验与限制长期漂移
```

### 实验：对照官方积分实现，观察固定偏差如何累计

**实验目标：**比较理想输入与带固定偏差的输入，验证相同时间内误差怎样累计，以及把更新频率从 50 Hz 改为 100 Hz 后，固定偏差是否消失。

官方依据是 Humble [turtlesim 的运动更新实现](https://github.com/ros/ros_tutorials/blob/humble/turtlesim/src/turtle.cpp)：它把转换后的线速度乘以 dt，再累计到位置。可先打开源码的 `Turtle::update`，找到位置更新部分，与上一页公式对应。源码中的屏幕纵坐标和发布的 pose.y 方向不同，阅读时要同时看 pose 的发布部分。

下方是为隔离误差来源编写的**课堂离线脚本，不是官方原样示例**。它只取朝向固定为 0、侧向速度为 0 的直线特例，使用合成输入；不需要启动 ROS 节点或真实底盘，也不把合成数据当作设备性能测试。

**准备条件：**安装 Python 3。以下命令在 Ubuntu Bash 中运行；脚本只用 Python 标准库，可在其他系统的 Python 3 环境中执行相同 Python 内容。先结束此前的仿真进程。

```bash
python3 - <<'PY'
def integrate(duration_s, hz, bias_mps):
    true_speed = 0.20
    dt = 1.0 / hz
    steps = int(round(duration_s * hz))
    estimated_x = 0.0
    for _ in range(steps):
        measured_speed = true_speed + bias_mps
        estimated_x += measured_speed * dt
    elapsed = steps * dt
    reference_x = true_speed * elapsed
    return steps, reference_x, estimated_x, estimated_x - reference_x


print('time_s  hz  bias_mps  steps  reference_m  estimate_m  error_m')
for duration in (10.0, 60.0):
    for hz in (50, 100):
        for bias in (0.0, 0.01):
            steps, reference, estimate, error = integrate(duration, hz, bias)
            print(f'{duration:6.0f} {hz:3d} {bias:9.2f} {steps:6d} '
                  f'{reference:12.3f} {estimate:11.3f} {error:8.3f}')
PY
```

脚本中的 hz 是**模拟采样频率**，循环会快速运行，不是让本机真实等待 10 s 或 60 s。`reference_x` 是由人为设定的真实速度计算出的评价参考，只用于比较，不送回积分过程修正结果。

**① 比较不同持续时间。**

| 模拟时长 | 频率 | 固定速度偏差 | 参考位置 | 估计位置 | 位置误差 |
| --- | --- | --- | --- | --- | --- |
| 10 s | 50 Hz | 0 m/s | 2.000 m | 2.000 m | 约 0 m |
| 10 s | 50 Hz | +0.01 m/s | 2.000 m | 2.100 m | +0.100 m |
| 60 s | 50 Hz | 0 m/s | 12.000 m | 12.000 m | 约 0 m |
| 60 s | 50 Hz | +0.01 m/s | 12.000 m | 12.600 m | +0.600 m |

此处理想组接近精确，是因为合成速度恒定、方向和时间都准确；不能据此推断一般实机运动没有积分误差。输出可能出现末位浮点误差或 `-0.000`。

**② 比较 50 Hz 与 100 Hz。**

相同总时长、相同偏差下，两种频率的位置误差应基本相同。60 s 时，50 Hz 累计 3000 步，100 Hz 累计 6000 步；每一步更小，但总偏差仍约为 0.600 m。

**③ 将偏差改为负值，先预测再运行。**

把脚本中的 `(0.0, 0.01)` 改为 `(0.0, -0.01)` 后再执行，预测 60 s 的有偏组应得到约 11.400 m，误差为 -0.600 m。说明“越走越偏”既可能是多算，也可能是少算。

**验收：**保存两次运行的结果，核对误差是否满足 `bias × 模拟时长`；解释提高频率为什么没有消除固定偏差，并指出积分器在不使用评价参考的情况下，是否能自行知道输入带偏差。

**排查与结束：**没有输出时检查 heredoc 结束标记 `PY` 是否独占一行，或将 Python 内容保存为 `.py` 文件再运行；改参数后结果异常时检查秒与 m/s 的单位、循环次数及偏差符号。脚本完成后自行退出，不产生 ROS 发布者或数据文件。本实验只演示固定速度偏差，不声称已经覆盖随机噪声、打滑、航向误差和真实时间抖动。

### 课堂提问

> 0.2 m/s 的速度以 50 Hz 更新，为什么每次只增加 0.004 m？

参考答案：每次标称经过 0.02 s，位移是速度乘以时间，而不是直接把速度数值加到位置上。

> 一条里程计轨迹连续且很平滑，为什么仍可能越来越不准确？

参考答案：固定偏差可以平滑地被累计进去。连续性与精度是不同性质，没有独立观测时不能仅凭平滑外观判断没有漂移。

> 航位推算不依赖 GPS 才能更新，为什么长期导航仍需要其他观测？

参考答案：它可以自主传播状态，但缺少独立位置或环境约束时无法可靠识别、限制长期漂移；其他观测提供新的约束，具体形式不一定是 GPS。

这页最后可以总结：

> **航位推算把每一小段运动累加成连续轨迹，也把输入误差传播到位置；短时用好它的连续性，长期用独立观测约束漂移。**

---

## 第 21 页｜离散化：增量式积分（核心算法）

这页你可以把它讲成：

> **现在把前两页连起来：每个时间段先用航向把反馈速度转到 odom，再乘以这个时间段的长度，最后累加到旧位置。位置要累加，航向在本项目中直接取对齐后的下位机融合值；速度用反馈，不用发出去的期望指令。程序只有几行，但坐标、单位和时间对应错了，结果就会持续偏离。**

### 讲解与例子

**1. 增量式积分中的“增量”，就是这一小段时间走过的位移。**

本页沿用车体系正 x 向前、正 y 向左、θ 逆时针为正的约定。把第 k 个时间段的车体反馈速度记为 `Vx[k]`、`Vy[k]`，与之对应的车体航向记为 `θ[k]`：

```text
dt[k] = t[k+1] - t[k]

Δx[k] = (Vx[k]·cosθ[k] - Vy[k]·sinθ[k]) × dt[k]
Δy[k] = (Vx[k]·sinθ[k] + Vy[k]·cosθ[k]) × dt[k]

x[k+1] = x[k] + Δx[k]
y[k+1] = y[k] + Δy[k]

θ 的来源：经过安装、轴向及初始参考对齐的下位机融合航向
本项目不在此处再执行 θ ← θ + 原始ω × dt
```

箭头表示更新状态，不是把 x 每次覆盖成 `Δx`。例如沿 odom 正 x 方向以 1 m/s 运动，每步 0.1 s，则每步增加 0.1 m；20 步后应为 2 m，而不是始终停留在 0.1 m。

`Yawz` 在 PPT 中表示融合后的偏航角，单位是 rad，不是 `angular_velocity.z` 的 rad/s。变量名称相近，不能直接互换。

**2. 先说明“这一段用哪个航向”，不要只照着三行赋值的书写顺序理解。**

为明确离散误差，本页实验采用**左端点欧拉积分**：区间 `[t[k], t[k+1])` 用起点时刻的速度和航向作近似。真实回调在收到下一份样本后才知道实际 dt，可以缓存上一份样本来实现这种约定。

如果实现选用区间末端的当前航向，则对应另一种端点近似；不能一边声称使用起点值，一边在更新位置前悄悄换成下一时刻的角度。PPT 中 θ 写在 x、y 更新之后，并不能单独证明实际驱动采用哪种采样时序，要结合反馈时间戳和代码确认。

| 实现步骤 | 应检查的内容 |
| --- | --- |
| 获取一组输入 | 反馈速度、融合航向、样本时间之间的对应关系；异步数据需明确配对或插值策略 |
| 确定 dt | 使用约定时钟下的实际时间差；首份样本只建立时间与输入缓存 |
| 计算两个位置增量 | 用同一组速度、航向与 dt，先旋转再乘时间；单位为 m |
| 更新位置与输入缓存 | x、y 保留累计值，为下一时间段保存需要的样本和时间 |
| 输出位姿 | 使用对应输出时刻的有效航向，发布一致的 Odometry 和动态 TF |

时间倒退、重复时间戳或反馈中断后过大的 dt 都需要显式处理。不要把很久以前的速度乘上整个失联时间，也不要在没有新观测时把同一段反馈重复积分。异常阈值与恢复方式应根据实际通信和时钟设计确定。

**3. 决策一：上位机使用融合航向，避免重复建立原始陀螺积分。**

PPT 的零偏例子可以现场换算：`0.01 rad/s × 600 s = 6 rad ≈ 343.8°`。这是累计角度误差的示意，不是设备实测值；若角度显示被限制在 `[-π, π)`，屏幕不会直接显示 343.8°，但积分偏差并没有因此消失。

本项目沿用下位机已输出的融合姿态，避免驱动再从原始 ω 积分出另一套航向。融合算法可以利用不同观测与标定处理改善估计，但这一页没有提供算法实现，不能仅凭“融合”二字认定其长期绝对精度。

特别注意 PPT 中“融合给出绝对航向”的说法：前文平台是六轴 IMU，重力能约束横滚、俯仰，不能单独确定绝对 yaw；缺少其他航向约束时，融合 yaw 仍可能漂移。使用时还需对齐安装轴向和启动参考，不能把原始融合角不加区分地当成 odom 航向。

依据：官方 [REP-145 IMU 驱动约定草案](https://github.com/ros-infrastructure/rep/blob/master/rep-0145.rst)明确区分姿态参考与绝对偏航参考。该决策是确定本项目的位置积分输入来源，不意味着角速度失去用途；它仍可用于速度消息、诊断或其他估计过程。

**4. 决策二：积分编码器反馈换算出的速度，不积分 cmd_vel。**

`cmd_vel` 表示希望机器人怎样运动；编码器测得轮子实际怎样转动，经运动学模型换算后才得到本页使用的车体速度。若命令前进 1 m/s，但电机未转、反馈为零，积分指令会让里程计“虚走”，积分有效反馈则不会凭空增加位移。

不过 PPT 中“反馈是实际”也要加限定：**编码器反映轮转，不直接测量地面位移。**车轮空转或打滑时，编码器仍可能有速度，轮速换算出的车体速度也会出错。采用反馈能反映执行差异，但不能自动解决打滑；仍需标定和其他观测约束。

作为官方工程实现的对照，可阅读 Humble [ros2_control 的里程计实现](https://github.com/ros-controls/ros2_controllers/blob/humble/diff_drive_controller/src/odometry.cpp)：其中区分反馈更新与开环更新，并提供不同的积分处理。该源码面向差速底盘，**不是本项目麦轮驱动，也不采用本页同一套融合航向输入**；这里只比较反馈、时间与积分的职责，不直接移植其轮速公式。

**5. PPT 的直线练习成立，还需要速度恒定这一条件。**

从 `(0, 0)` 出发，`Vx = 1 m/s`、`Vy = 0`、持续 2 s、`dt = 0.1 s`，共有 20 个时间段：

| 固定航向 | 单步增量 | 20 步后的理论位置 |
| --- | --- | --- |
| θ = 0 | `(0.1, 0)` m | `(2.0, 0)` m |
| θ = π/2 | `(0, 0.1)` m | `(0, 2.0)` m |

这两个算例中，速度和航向都恒定，欧拉累计除浮点舍入外可与解析结果一致。不能把它概括成“只要 θ 恒定就无误差”：即使方向不变，速度在每个区间内变化也可能产生积分误差。

在输入准确、时间一致、运动足够平滑的条件下，减小 dt 通常能减小欧拉法的离散误差；但上一页演示的固定速度偏差，以及打滑、航向漂移等，不会仅靠减小 dt 消失。

### 板书

```text
位置：旧值 + 本段增量；增量 = 旋转后的反馈速度 × 实际dt
航向：采用对齐后的融合值，本项目不在此处再积原始ω
速度：反馈轮速经运动学换算；cmd_vel只是期望

区间左端近似：用 t[k] 的输入覆盖 [t[k], t[k+1])
先核对样本时间，再讨论更新顺序

1 m/s × 0.1 s = 0.1 m/步；2 s 共20步
θ=0 → (2,0)；θ=90° → (0,2)

小dt减少离散误差，不能消除系统偏差
融合yaw≠绝对无漂移；轮转反馈≠无滑动的真实地面速度
```

### 实验：对照官方实现，验证直线累计与转弯离散误差

**实验目标：**复现 PPT 的两组直线练习，再用已知的连续转弯轨迹比较不同 dt 的欧拉积分结果。

官方参考：Humble [turtlesim 的位置累计实现](https://github.com/ros/ros_tutorials/blob/humble/turtlesim/src/turtle.cpp)，以及上述 ros2_control 源码的圆弧积分处理。turtlesim 在转弯时先更新朝向再更新位置，其端点选择与本实验明确采用的左端点近似不同；两者不能在相同步长下直接期待完全相同的端点。

下方是为本页编写的**离线课堂脚本，不是官方原样程序**。它使用合成的精确航向样本模拟融合航向输入，不运行真实 IMU 融合算法；转弯时角速度只用于生成已知参考轨迹，位置积分器仍只接收航向，不在内部积分 ω。

**准备条件：**Python 3，无第三方依赖。在 Ubuntu Bash 中运行；其他系统可将 Python 内容保存为 `.py` 文件后执行。

```bash
python3 - <<'PY'
import math


def step(x, y, vx, vy, yaw_rad, dt):
    if not math.isfinite(dt) or dt <= 0:
        raise ValueError('dt must be finite and positive')
    c, s = math.cos(yaw_rad), math.sin(yaw_rad)
    return x + (vx * c - vy * s) * dt, y + (vx * s + vy * c) * dt


def simulate(dt, heading_at):
    duration = 2.0
    count = int(round(duration / dt))
    if not math.isclose(count * dt, duration):
        raise ValueError('choose dt that divides the duration')
    x, y = 0.0, 0.0
    for k in range(count):
        sample_time = k * dt  # Left endpoint of this interval.
        yaw_sample = heading_at(sample_time)
        x, y = step(x, y, 1.0, 0.0, yaw_sample, dt)
    return x, y


for name, heading in [('straight_0', 0.0), ('straight_90', math.pi / 2)]:
    x, y = simulate(0.1, lambda t: heading)
    print(f'{name}: x={x:.6f}, y={y:.6f}')

speed = 1.0
turn_rate = math.pi / 4  # Used only to construct a known reference.
duration = 2.0
reference_x = speed / turn_rate * math.sin(turn_rate * duration)
reference_y = speed / turn_rate * (1 - math.cos(turn_rate * duration))
print(f'arc reference: x={reference_x:.6f}, y={reference_y:.6f}')
for dt in (0.1, 0.02, 0.01):
    x, y = simulate(dt, lambda t: turn_rate * t)
    error = math.hypot(x - reference_x, y - reference_y)
    print(f'dt={dt:.2f}: x={x:.6f}, y={y:.6f}, error={error:.6f}')
PY
```

**① 核对直线结果。**

前两行应分别约为 `(2.000000, 0.000000)` 与 `(0.000000, 2.000000)`。脚本以 20 个区间覆盖 2 s，不是把 0、0.1、……、2.0 这 21 个时刻都各积分一次；最后一个时刻是区间终点。

**② 核对转弯的解析参考。**

车体以 1 m/s 前进，朝向从 0 在 2 s 内匀速左转到 90°，圆弧半径为 `v/ω = 4/π m`。解析终点为 `(4/π, 4/π)`，约 `(1.273240, 1.273240)` m。这里参考公式的 ω 是人为设定的无偏真值，不是对实机原始陀螺积分精度的假设。

**③ 比较三种 dt。**

| dt | 欧拉终点 x | 欧拉终点 y | 与解析终点的距离误差 |
| --- | --- | --- | --- |
| 0.10 s | 约 1.3226 m | 约 1.2226 m | 约 0.0707 m |
| 0.02 s | 约 1.2832 m | 约 1.2632 m | 约 0.0141 m |
| 0.01 s | 约 1.2782 m | 约 1.2682 m | 约 0.0071 m |

左端点近似在每段内暂时保留较早的朝向，因此这组左转结果表现为 x 偏大、y 偏小；步长缩小后误差降低。把本实验与上一页并列看：**理想平滑转弯的离散误差会减小，固定输入偏差不会因此自动消失。**

脚本中的时间是合成采样时间，不会真实等待 2 s，也没有模拟串口延迟、异步配对、打滑或航向漂移。

**验收：**记录两组直线终点、解析转弯终点及三种步长的误差；解释为何融合航向不断更新时仍存在位置积分误差，并指出本代码哪一行选择了左端点样本。

**排查与结束：**结果相差很大时先查弧度与角度、总时间及循环次数，确认没有对航向再次执行 `ω × dt` 累加。更改步长时要覆盖相同总时长，不能保持步数不变。命令运行完成后自行退出，不产生 ROS 节点或输出文件。

### 课堂提问

> 位置更新为什么用 `+=`，而航向可以直接采用新的融合值？

参考答案：速度乘时间给出位移增量，需要累加；融合航向已经是相对于某个参考的姿态估计，完成参考对齐后用于表示朝向，不能把每次角度读数当成角度增量再相加。

> 车轮空转时，采用编码器反馈是否就能保证位置不变？

参考答案：不能。编码器测到轮子在转，运动学在无滑动假设下仍可能推算出平移，需要其他观测识别轮转与地面运动的差异。

> 航向保持不变，是否就能断言欧拉积分没有误差？

参考答案：不能，还要看速度是否在区间内变化、输入与时间是否准确。PPT 的直线算例同时满足恒定速度与恒定航向，才得到相应的精确理想结果。

这页最后可以总结：

> **增量积分的核心是把同一时间段的反馈速度、对齐航向和实际时长配在一起；明确样本时序、位置累加和航向来源，才能把简短公式写成可靠的更新过程。**

---

## 第 22 页｜麦克纳姆轮全向底盘运动学

这页你可以把它讲成：

> **前面把车体速度变成了位置，现在往底层再看一步：四个轮子怎样产生车体的前进、横移和旋转？麦克纳姆轮通过斜置的被动滚子与四轮组合，让不同轮速合成为平面运动。我们先固定轮序和符号，再看三种基本运动怎样叠加。本项目的逆运动学在 MCU 内完成，ROS 驱动传的是车体速度，理解公式是为了读懂接口和排查运动方向。**

### 讲解与例子

**1. 全向来自滚子与四轮布局的组合，不是单个轮子能主动朝任意方向驱动。**

麦轮通常采用约 45° 的斜置被动滚子。电机驱动轮体转动，滚子允许沿特定方向滚动；配合适当的四轮安装方向，接触约束使各轮转动合成为前后平移、左右横移和绕 z 轴旋转。

这一页是理想运动学模型：假设尺寸与安装符合约定、接触正常，并忽略非理想打滑等影响。模型描述速度关系，不直接给出牵引力、电机转矩或复杂地面的通过能力；不能把单个轮子的作用理解成沿滚子任意方向主动施力。

**2. 先标四个轮子，再定义尺寸和正方向。**

```text
俯视，车头朝上：

             车体 +x（前）
                  ↑
      左前 ω1  ○──────○  右前 ω2
               │ 中心 │
      左后 ω3  ○──────○  右后 ω4

车体 +y 向左；+ω 绕 +z 逆时针
本页轮序固定为：左前、右前、左后、右后
```

| 符号 | 本页明确定义 | 单位 |
| --- | --- | --- |
| `Vx`、`Vy` | 底盘几何中心处、沿车体系前方和左方的速度 | m/s |
| `ω` | 底盘绕中心的偏航角速度 | rad/s |
| `ω1…ω4` | 对应四轮的有符号角速度，正值约定为该轮参与正向前进的转向 | rad/s |
| `a` | 中心到前轮轴线沿 x 方向的距离，即前后轴距的一半 | m |
| `b` | 中心到左右轮中心沿 y 方向的距离，即轮距的一半 | m |
| `r` | 有效轮半径 | m |
| `K = a + b` | 公式使用的几何组合参数 | m |

上述 a、b 定义针对前后、左右对称的矩形布局。PPT 下方的 L 没有出现在四条公式中，本页不把 L 与 a、b 混用；不能把完整轴距、完整轮距直接代入半尺寸的位置。

轮速的正号是统一后的数学约定，不等于从车体左右两侧观察都“顺时针”。电机接线、编码器和关节轴向可能不同，实机应按协议与固件映射。滚子安装组合改变时，符号也可能改变；上方示意只标轮序，不能据此替代实际滚子方向核对。

**3. 逆运动学：期望车体速度怎样分配到四轮。**

按 PPT 给出的安装与符号组合：

```text
ω1 = (Vx - Vy - K·ω) / r     左前
ω2 = (Vx + Vy + K·ω) / r     右前
ω3 = (Vx + Vy - K·ω) / r     左后
ω4 = (Vx - Vy + K·ω) / r     右后
```

先读成三部分相加：前进项四轮同号，横移项对角轮同号，正旋转项左侧两轮为负、右侧两轮为正。各项在公式中线性叠加，所以可以边平移边旋转。

分子每一项都对应线速度：`K·ω` 是 m/s，除以轮半径后得到 rad/s。若控制板接口采用 rpm，还需按 `rpm = rad/s × 60 / (2π)` 换算，不能把两种单位的数值直接互填。

“三者解耦”在这里指理想运动学可以把三个基本分量独立表达再叠加。实际轮速上限、加速度、附着和动力限制会共同约束可实现的速度组合，并不意味着三项都能无限增大。

**4. 正运动学：从轮速反馈还原车体速度。**

为了验证方向，可以把上述关系反过来组合：

```text
Vx = r/4 × ( ω1 + ω2 + ω3 + ω4)
Vy = r/4 × (-ω1 + ω2 + ω3 - ω4)
ω  = r/(4K) × (-ω1 + ω2 - ω3 + ω4)
```

前进项相加，其余项抵消；另两种加减组合分别提取横移与旋转。这是同一理想模型下的反馈换算，仍受轮转与地面运动不一致的影响。对不符合模型的任意四轮测量，上式给出相应的线性估计，不保证实车一定按它运动。

官方对照可看 [ros2_control 麦轮控制器说明](https://control.ros.org/rolling/doc/ros2_controllers/mecanum_drive_controller/doc/userdoc.html)、[逆运动学实现](https://github.com/ros-controls/ros2_controllers/blob/master/mecanum_drive_controller/src/mecanum_drive_controller.cpp)及[正运动学实现](https://github.com/ros-controls/ros2_controllers/blob/master/mecanum_drive_controller/src/odometry.cpp)。在 base 参考点与几何中心重合、轴向一致时，可按物理轮名对应本页公式；官方数组顺序为左前、右前、右后、左后，与本页最后两轮的顺序不同。

这些链接是 Rolling / master 开发版的对照资料，不是本项目 Humble 固件源码或安装步骤。官方实现还处理参考点偏移等情况，本页只保留 PPT 的中心模型。

**5. 本项目把逆解放在 MCU，ROS 侧仍承担驱动职责。**

```text
导航 / 手柄发布车体速度 (Vx, Vy, ω)
            ↓
ROS 驱动订阅并编码成串口协议帧
            ↓
MCU 按底盘参数与轮序完成逆运动学、轮速控制
            ↓
电机与车轮执行，反馈经协议返回
```

“ROS 侧只透传”针对的是**下行速度分配**：不在这一层重复求四轮目标速度。ROS 侧仍需要做协议编码解析、设备管理、反馈消息发布和前文的里程计、TF 等工作。反馈的正运动学具体在哪端计算，应以实际固件和协议为准，不能仅凭本页猜测回传字段。

**6. 与差速底盘比较，要区分车体侧移与 odom 中的 y 变化。**

理想差速底盘可控制前向速度与偏航角速度，车体系 `Vy = 0`，不能在保持车头朝向不变时直接横移。它仍能通过转向和前进到达侧方目标，也能在 odom 中产生 y 位移；“两个控制分量”不等于平面位姿只有两个变量。

上一页的通用位置积分令 `Vy = 0`，就变成差速或单轨运动学常见的平面传播形式。PPT 的“linear.y 恒为 0”应针对差速模式理解，**不能把本项目全向底盘的侧向指令和反馈也统一清零**。

麦轮适合需要横移、掉头空间有限且地面较平整的作业；差速结构通常更简单。户外和非平地形的适用性还取决于轮径、悬挂、附着和底盘结构，不能只凭“差速”二字判断。

### 板书

```text
轮序：1左前、2右前、3左后、4右后；K=a+b（两个半尺寸之和）

逆解：车体速度 → 四轮目标速度
正解：四轮反馈 → 车体速度估计

前进：(+,+,+,+)
左移：(-,+,+,-)
左转：(-,+,-,+)
符号均以本页约定为准，先核对滚子、轮序、电机方向和单位

本项目：MCU逆解；ROS编码下发车体速度
差速：车体系Vy=0；全向：可独立控制Vy
```

### 实验：对照官方公式，完成四轮逆解与正解还原

**实验目标：**用三种基本运动和一种混合运动检查四轮符号，再将计算出的轮速送入正解，验证恢复的车体速度。

参考上述官方正、逆运动学实现，按本页轮序整理为最小离线练习。**脚本是课堂改写，不是官方原样示例**；不运行 ROS 控制器、不连接 MCU，也不模拟滚子接触。其作用是验证模型与编号，不是标定或实机性能验证。

为了方便手算，实验设 `r = 0.05 m`、`a = 0.15 m`、`b = 0.10 m`，这些均为课堂假设，**不是 NanoOmni 实测参数**。前文平台的标称轮径为 64 mm，对应半径 0.032 m，不能把本实验半径直接写入项目配置。

**准备条件：**Python 3，无第三方依赖。在 Ubuntu Bash 运行；其他系统可将 Python 内容保存为 `.py` 文件后执行。

```bash
python3 - <<'PY'
RADIUS = 0.05
K = 0.15 + 0.10


def inverse(vx, vy, wz):
    return (
        (vx - vy - K * wz) / RADIUS,  # Front left.
        (vx + vy + K * wz) / RADIUS,  # Front right.
        (vx + vy - K * wz) / RADIUS,  # Rear left.
        (vx - vy + K * wz) / RADIUS,  # Rear right.
    )


def forward(wheels):
    fl, fr, rl, rr = wheels  # PPT order, not the official array order.
    return (
        RADIUS / 4 * (fl + fr + rl + rr),
        RADIUS / 4 * (-fl + fr + rl - rr),
        RADIUS / (4 * K) * (-fl + fr - rl + rr),
    )


cases = [
    ('forward', (0.20, 0.0, 0.0)),
    ('left', (0.0, 0.10, 0.0)),
    ('turn_left', (0.0, 0.0, 0.40)),
    ('mixed', (0.20, 0.10, 0.40)),
]
for name, body in cases:
    wheels = inverse(*body)
    restored = forward(wheels)
    print(f'{name}: wheels={[round(v, 6) for v in wheels]}, '
          f'body={[round(v, 6) for v in restored]}')

mixed_wheels = inverse(0.20, 0.10, 0.40)
fl, fr, rl, rr = mixed_wheels
print('wrong rear-wheel order:', forward((fl, fr, rr, rl)))
PY
```

**① 对比手算结果。**

| 输入 `(Vx, Vy, ω)` | 四轮角速度 `(ω1, ω2, ω3, ω4)`，rad/s | 正解恢复值 |
| --- | --- | --- |
| `(0.20, 0, 0)` | `(4, 4, 4, 4)` | `(0.20, 0, 0)` |
| `(0, 0.10, 0)` | `(-2, 2, 2, -2)` | `(0, 0.10, 0)` |
| `(0, 0, 0.40)` | `(-2, 2, -2, 2)` | `(0, 0, 0.40)` |
| `(0.20, 0.10, 0.40)` | `(0, 8, 4, 4)` | `(0.20, 0.10, 0.40)` |

混合组也可通过前三组的轮速逐项相加获得，展示线性叠加。左前轮为零只说明该轮在这组理想组合中无需转动，不表示整车停止。

**② 检查最后一行，识别一个容易漏掉的验证问题。**

混合组的左后、右后恰好都为 4，交换它们后，数值仍一样。因此脚本最后一行虽然模拟了错误轮序，却仍会还原出原来的车体速度。不能仅凭这一组通过，就认定轮序正确。

把最后一段的 `inverse(0.20, 0.10, 0.40)` 改成 `inverse(0.0, 0.10, 0.0)` 后再运行。正确左移轮速为 `(-2, 2, 2, -2)`；交换后两轮后，按本页正解得到约 `(0, 0, 0.40)`，把横移误判成了旋转。这就是要用不同基本运动覆盖符号与轮序的原因。

**验收：**记录四组正确轮速及恢复值、两次错误轮序演示结果；解释为什么第一组错误轮序演示没有暴露问题，以及本项目为什么不需要 ROS 侧再下发这四个轮速。

**排查与结束：**数值不符时先查轮序、正负号、半径与直径、完整尺寸与半尺寸，以及 rad/s 与 rpm；前进正确但横移或旋转错误时，优先检查后两轮映射和各分量符号。脚本完成后自行退出，不发送实机命令或产生文件。

### 课堂提问

> 为什么四个轮子的速度不是四个独立的车体运动自由度？

参考答案：平面刚体运动由 Vx、Vy、ω 三个分量描述，符合无滑动理想模型的四轮目标速度受共同运动学关系约束；多一个轮子不增加一种平面位姿变量。

> 同一段前进代码正常，能否证明整个麦轮运动学正确？

参考答案：不能。前进时四轮同速，许多轮序互换不会暴露；还要验证横移、旋转和混合运动，核对物理轮名及符号。

> 差速底盘的车体系 Vy 为零，为什么 odom 中 y 仍可能增加？

参考答案：车头转向后，车体前向速度在 odom 的 y 轴上可以有分量；不能直接侧移与固定参考中的 y 不变是两回事。

这页最后可以总结：

> **麦轮用约定清楚的四轮组合实现三种平面运动；先核对轮序、安装方向、尺寸与单位，再用正逆解和基本运动检查结果，ROS 与 MCU 按接口各自完成职责。**

---

## 第 23 页｜误差累积与外部修正

这页你可以把它讲成：

> **这一章最后把两件事接起来：驱动持续提供平滑的局部运动估计，定位或建图模块利用环境和位置观测检查长期偏差。轮速和航向积分得再平滑，也不能只凭同一份输入证明自己没有漂移。外部修正的意义是带来新的约束；在 ROS 中，常通过 map 与 odom 之间的关系表达修正，让全局位置得到调整，同时保留局部里程计的连续性。**

### 讲解与例子

**1. “无法靠自身消除漂移”，是指单靠同一份运动输入无法判断未知误差。**

延续第 20 页的例子：真实速度为 0.20 m/s，反馈却为 0.21 m/s，积分器只看到后一组数字。它不知道机器人确实更快，还是速度估计带偏差；重复计算同一份输入，不会产生新的辨别依据。

PPT 用“系统性”强调长期漂移问题，但误差不全是固定系统偏差，还包括随机噪声、打滑、航向和时间误差。标定、改进融合与积分方法能改善估计，却不能据此保证长期不漂移。

“引入外部绝对参考”在本课中应理解为：**引入相对纯运动积分而言独立的环境约束或位置观测**。激光匹配、回环和地图定位不一定提供地球坐标中的绝对位置，但能约束相对于环境或地图的误差；不必把这句话理解成所有机器人都要装 GPS。

**2. 三种修正方式的输入和任务不同。**

| PPT 中的手段 | 主要输入与任务 | 怎样提供约束 | 不能忽略的条件 |
| --- | --- | --- | --- |
| Cartographer（本课关注 2D SLAM） | 激光扫描，配合配置所需的其他运动信息；估计轨迹并构建地图 | 扫描与局部子地图匹配；回环约束和全局优化进一步调整轨迹与子地图关系 | 需要有效观测、正确时间与 TF；局部匹配并不保证长期没有漂移 |
| AMCL | 已知地图、2D 激光和里程计运动信息；在地图中估计位姿 | 粒子表示候选位姿，通过运动预测与激光观测更新分布 | 主要做定位，不负责从零建图；初始化、地图适配及观测可辨识性影响结果 |
| GPS（室外，属于 GNSS 的一种） | 接收机观测卫星信号，输出地理位置等数据 | 为状态估计提供地理位置约束，帮助限制长期位置漂移 | 遮挡、多路径等会影响质量；需坐标转换与融合，不能直接覆盖 odom 数值 |

Cartographer 这一行不能只讲成“拿扫描对齐一张已经完成的地图”。建图过程中也会把新扫描匹配到正在维护的子地图；局部 SLAM 仍可漂移，回环和全局优化负责引入更大范围的一致性约束。依据是官方 [Cartographer 算法概览](https://google-cartographer-ros.readthedocs.io/en/latest/algo_walkthrough.html)。

AMCL 中的“全局定位”指在地图参考中确定机器人位姿；有很多候选位置时不保证一次扫描就能立刻收敛。其核心是概率估计，不能讲成直接把激光坐标当作真实位置。Humble [Nav2 AMCL 官方说明](https://github.com/ros-navigation/navigation2/blob/humble/nav2_amcl/README.md)明确其利用 2D 激光在已知地图中估计位置与朝向。

GPS 接收机是设备，“卫星”是信号来源。经纬度不是局部平面米制坐标；单次位置观测也不等于已经获得可靠车体航向。Humble [robot_localization 的 navsat_transform_node 文档](https://github.com/cra-ros-pkg/robot_localization/blob/humble-devel/doc/navsat_transform_node.rst)展示了将 GPS、航向和里程计信息转换到一致参考后供状态估计使用的流程，不是把经纬度直接填入 x、y。

**3. 常见分工：保持局部连续，在 map 与 odom 之间表达修正。**

官方 [REP-105](https://github.com/ros-infrastructure/rep/blob/master/rep-0105.rst)区分了 odom 的短时连续性与 map 的长期参考作用。结合本项目的地面投影坐标系，可画成：

```text
map ──定位 / 建图的修正关系──→ odom ──局部运动估计──→ base_footprint

map 中的车体位姿 = map 到 odom 的关系 × odom 中的车体位姿
```

这里的乘号表示刚体变换组合，不是把两个位置数值直接相乘。采用 `T_A_B` 表示 B 坐标系在 A 中的位姿，则：

```text
T_map_base = T_map_odom × T_odom_base
T_map_odom = T_map_base × inverse(T_odom_base)
```

定位结果与里程计必须在对应时刻比较；一般包含旋转时也不能只对 x、y 做减法。每条 TF 边应有明确发布者，不能让 Cartographer、AMCL 或其他估计器同时争用同一条 `map → odom` 关系。

给一个无旋转的一维例子：里程计估计车体在 x = 1.20 m，地图定位估计同一时刻为 x = 1.00 m。可以保持 odom 中的 1.20 m，通过 `map → odom` 的 x 平移 -0.20 m，得到 map 中的 1.00 m。局部里程计没有因此被清零，全局表达得到了修正。

这也解释了为什么 map 中的位姿可能在收到新定位或回环结果时调整，而 odom 中的局部轨迹应尽量保持连续。修正表达与真实算法的发布频率、平滑策略有关，不能要求所有系统都用同一种瞬时跳变方式。

**4. 外部观测也可能出错，不能把修正理解成必然正确。**

长走廊、重复结构或大面积动态物体会使激光匹配和定位变得困难；地图过时会造成不一致，GPS 也可能出现异常观测。系统需要按算法设计使用质量信息、不确定性与异常处理，而不是无条件相信每一帧外部数据。

修正前先核对坐标轴、单位、安装 TF 和时间戳；否则可能把接口错误当成漂移，再用定位去掩盖问题。是否已经得到有效修正，需要结合独立参考、观测残差或重复实验评估，不能只凭“轨迹看起来更平滑”。

**5. 按 PPT 收束本章：从反馈速度到局部位置，再接上外部约束。**

```text
编码器反馈换算出的车体系速度 (Vx, Vy)
        + 对齐后的融合航向 θ
                    ↓ 旋转矩阵
vx_odom = Vx·cosθ - Vy·sinθ
vy_odom = Vx·sinθ + Vy·cosθ
                    ↓ 按对应时间段积分
x ← x + vx_odom·dt；y ← y + vy_odom·dt
                    ↓
连续的局部里程计 / TF ──结合环境与位置观测──→ 全局一致性修正
```

三个项目决策保持一致：航向采用下位机融合值，上位机不在此处再积分原始 ω；速度使用轮速反馈换算值，不使用 cmd_vel 代替实际反馈；麦轮逆解放在 MCU，ROS 驱动按协议下发车体速度。

同时保留各自边界：融合 yaw 仍可能漂移，轮转反馈仍受打滑影响，逆解在哪一端执行也不决定定位精度。因此局部驱动、运动估计与外部定位是互相配合的职责。

### 板书

```text
纯积分：只能传播已知状态，不能凭同一输入识别所有未知误差
新约束：激光 / 地图 / 回环 / GPS 等，不一定是地球绝对坐标

Cartographer：局部扫描匹配 + 回环 / 全局优化，建图与估计轨迹
AMCL：已知地图中的概率定位
GPS：地理位置观测，转换坐标并融合

map → odom → base_footprint
全局修正关系     连续局部运动
例：-0.20 m + 1.20 m = 1.00 m（同向、无旋转的一维特例）
```

### 实验：用官方 TF 工具比较修正前后的两个快照

**实验目标：**观察同一份局部里程计位置在全局关系改变后，怎样得到不同的 map 表达，区分“改变修正关系”和“清零里程计”。

来源：Humble 官方 [静态 TF 工具教程](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Intermediate/Tf2/Writing-A-Tf2-Static-Broadcaster-Py.rst)，结合 REP-105 的分层关系。全部使用官方 `static_transform_publisher` 与 `tf2_echo`，数值是课堂假设，不是真实定位输出。

**本实验把两个独立的几何快照分别保持不变供查询，方便手算。**真实运动关系与在线定位修正应按实际系统动态发布；不要把本实验的静态发布方式直接用于实机的 `map → odom` 或运动中的里程计关系。实验不运行 Cartographer、AMCL 或 GPS 融合，不能验证其定位效果。

**准备条件：**Ubuntu 22.04 或 WSL Ubuntu 22.04，已安装 ROS 2 Humble，无需图形桌面。在 Ubuntu Bash 中执行：

```bash
sudo apt update
sudo apt install ros-humble-tf2-ros
```

结束此前实验的发布进程。打开 A、B、C 三个终端，**每个终端都执行**以下设置；若 99 已用于其他实验，统一换用空闲 Domain ID。

```bash
source /opt/ros/humble/setup.bash
export ROS_DOMAIN_ID=99
export ROS_LOCALHOST_ONLY=1
```

**① 终端 A 发布共同的局部位姿快照，保持运行。**

```bash
ros2 run tf2_ros static_transform_publisher \
  --x 1.20 --y 0 --z 0 --roll 0 --pitch 0 --yaw 0 \
  --frame-id lab_odom --child-frame-id lab_base
```

**② 终端 B 发布修正前的全局关系，保持运行。**

```bash
ros2 run tf2_ros static_transform_publisher \
  --x 0 --y 0 --z 0 --roll 0 --pitch 0 --yaw 0 \
  --frame-id lab_map --child-frame-id lab_odom
```

**③ 终端 C 依次查询两种参考中的位置，每次记录后按 Ctrl+C。**

先查询局部位置，预期 x 约为 1.20 m：

```bash
ros2 run tf2_ros tf2_echo lab_odom lab_base
```

退出上一条查询，再查询全局表达，预期 x 也约为 1.20 m：

```bash
ros2 run tf2_ros tf2_echo lab_map lab_base
```

**④ 先停止 C 的查询，再停止 B 的旧发布者，随后仅在 B 启动修正后的关系。A 保持运行。**

```bash
ros2 run tf2_ros static_transform_publisher \
  --x -0.20 --y 0 --z 0 --roll 0 --pitch 0 --yaw 0 \
  --frame-id lab_map --child-frame-id lab_odom
```

不要让新旧 B 同时发布同一子坐标系。重新启动 C 的两条查询，每条都记录后退出；使用新的查询进程，让新 Buffer 从当前发布者接收关系，避免沿用上一快照的静态缓存。

| 查询 | 修正前快照 | 修正后快照 | 说明 |
| --- | --- | --- | --- |
| `lab_odom → lab_base` | x ≈ 1.20 m | x ≈ 1.20 m | 局部位姿没有被清零或改写 |
| `lab_map → lab_base` | x ≈ 1.20 m | x ≈ 1.00 m | 全局表达随修正关系改变 |

两次快照中 y、z 和旋转均为零。`lab_map`、`lab_odom`、`lab_base` 是实验名称，不与真实系统的同名发布者混用。快照之间没有模拟连续运动、时间插值或定位收敛过程。

**验收：**提交四次查询的 x 值，手算 `-0.20 + 1.20 = 1.00`，指出究竟哪条关系改变；说明为什么这个结果只能验证 TF 组合，不能证明某个定位算法已经消除了漂移。

**排查与结束：**找不到 frame 时检查 A、B 是否运行以及各终端环境；值不符时检查平移符号、父子名称及旧 B 是否已退出，重新启动查询以建立新缓存。实验后 C、B、A 依次按 `Ctrl+C` 结束。实际系统的定位修正还需满足时间一致性与发布者职责约定。

### 课堂提问

> Cartographer 与 AMCL 都可以利用激光，为什么不能当成同一种任务？

参考答案：本课的 Cartographer SLAM 同时构建地图并估计轨迹，利用扫描匹配、回环和优化建立一致性；AMCL 主要利用已有地图估计当前位姿，不负责从零建图。

> 有了地图定位后，为什么还保留会漂移的 odom？

参考答案：它提供高频、连续的局部运动参考。全局修正可通过 map 与 odom 的关系体现，让局部反馈与长期参考各自承担适合的职责。

> 外部修正是否一定要来自 GPS，是否一定正确？

参考答案：不一定。地图、环境匹配和回环也能提供独立约束；各种观测都可能退化或出错，仍需正确的坐标、时间和质量评估。

这页最后可以总结：

> **里程计负责连续地描述局部运动，外部观测提供新的环境或位置约束；通过清楚的参考系与发布职责衔接二者，才能在持续运动中限制长期漂移。**

---

## 第 24 页｜里程计与协方差

这页你可以把它讲成：

> **上一章我们算出了位置和速度，这一章要把它们变成下游能够正确使用的里程计消息。只告诉算法“我在这里”还不够，还要说明这个位置属于哪个坐标系、对应什么时刻，以及估计有多大不确定性。协方差就是传递不确定性的一种方式。读懂它，才能区分“这个量是零”和“这个量我根本没有测到”，也才能判断定位融合为什么过度相信或忽略一份数据。**

### 讲解与例子

**1. 先提出本章的三个问题，不在导入页展开全部推导。**

| PPT 中的问题 | 本章要建立的认识 | 本页先记住什么 |
| --- | --- | --- |
| Odometry 消息里装了什么 | 时间与参考系、位姿、速度以及各自的不确定性 | 一条消息不仅是 x、y 和速度 |
| 协方差怎样告诉下游“我有多可信” | 各分量的误差尺度与误差之间的关联 | 方差是有单位的数，不是可信度百分比 |
| 为什么未观测的量不能随手填零方差 | 区分状态数值、对该数值的把握，以及是否参与融合 | 大方差是建模选择，`1e6` 不是通用规定 |

可以先问学生：“两个里程计都说 x = 1.00 m，一个通常偏几厘米，一个可能偏几十厘米，你会把它们当成一样的信息吗？”回答之后再引出协方差，不必先从 36 个数组元素开始背。

**2. Odometry 同时交代数值、时空参考和不确定性。**

根据 Humble [Odometry 官方定义](https://github.com/ros2/common_interfaces/blob/humble/nav_msgs/msg/Odometry.msg)，本项目可以先按四部分阅读：

```text
header：数据时间戳；frame_id 指定 pose 的参考系，例如 odom
child_frame_id：例如 base_footprint；twist 在该坐标系中表达
pose：位置 + 姿态，以及 6×6 协方差
twist：线速度 + 角速度，以及另一份 6×6 协方差
```

消息名中的 Odometry 表示运动估计，不是对真实位置的保证。位姿和速度属于不同参考时，协方差也必须与各自所描述的变量及参考一致，不能把同一组数字不加判断地复制过去。

四元数有四个分量，但 pose 的协方差不是 7×7。[PoseWithCovariance 官方定义](https://github.com/ros2/common_interfaces/blob/humble/geometry_msgs/msg/PoseWithCovariance.msg)规定它描述三个平移分量与绕固定轴的三个旋转分量。这里先认识“六个误差方向”；矩阵布局和索引放到后续对应页面展开。

**3. “可信”先用方差建立直觉，不能当作准确率。**

在同一坐标轴、同一米制单位下，假设两份 x 估计都为 1.00 m：

| 课堂假设 | x 方差 | 对应标准差 σ = √方差 |
| --- | --- | --- |
| A | 0.0025 m² | 0.05 m |
| B | 0.25 m² | 0.50 m |

在误差模型合理且估计可靠的前提下，A 声明的不确定性比 B 小。下游融合算法可以据此分配不同影响，但具体效果还取决于预测的不确定性、相关性及算法配置；不能只看一个数字就确定最终权重。

这两组是教学假设，不是底盘实测结果。方差小不证明实际误差小，发布者也可能低估自己的误差。把方差改小不会改善传感器；反而可能让下游过度相信错误数据。协方差的非对角项描述误差之间的关联，填零表示采用无相关性的模型假设，不自动证明误差真正独立。

**4. 数值为零、零方差、没有测量，是三种不同含义。**

例如消息里 `pose.pose.position.z = 0.0`，可能只是平面驱动没有估计高度而保留了默认值。它本身不能说明“机器人高度被精确测为零”。

在协方差的数学含义中，z 的方差为零意味着模型认为该方向没有不确定性，不能拿来表示“未知”。接收程序可能对零值另作处理，因此还要读消费者的约定；不要假定所有软件都会以同样方式接受、忽略或修补零方差。

PPT 中的 `1e6` 可理解为用很大的方差表达该分量不可靠的一种做法。若它位于位置方差上，单位是 m²，其标准差为 1000 m；它是有限的大数，不是“禁止融合”的专用标志，也不是 ROS 消息格式强制的数值。当前截图不足以确认本项目驱动实际填写了哪些分量、是否确实采用这一数值，后续还要对照代码与配置。

尤其要区分：**本传感器没有观测 z**，与**系统根据适用的平面运动假设约束 z**。后者是有条件的模型信息，不是传感器突然测到了高度；不能因为机器人主要在地面运行，就把所有未测量量都填成零方差。

Humble [robot_localization 数据准备说明](https://github.com/cra-ros-pkg/robot_localization/blob/humble-devel/doc/preparing_sensor_data.rst)明确建议通过配置选择不使用的变量，而不是靠夸大协方差来排除它们。其[配置文档](https://github.com/cra-ros-pkg/robot_localization/blob/humble-devel/doc/configuring_robot_localization.rst)还提供 `two_d_mode` 处理适用的平面运动假设。课堂上因此应把 PPT 的问题解释为“未知为何不等于零不确定性”，而不是要求所有系统统一填写 `1e6`。

**5. 本章后续要把设计值与实测依据接起来。**

本页先交代学习路线：读消息结构 → 理解方差和协方差 → 按可观测量及下游配置设计 → 用重复实验与误差记录检查合理性。静止数据可以观察部分噪声，却不能代替行驶中的打滑、累计漂移和航向误差评估。

提醒学生保留一个问题：“这些数字是谁给出的，依据是什么？”不能因为消息已经填满，就认定不确定性已经经过标定；也不能把本章的课堂示例当作项目推荐参数。

### 板书

```text
本章三问：装了什么？有多不确定？没测到的量怎样处理？

Odometry = 时间 / 坐标系 + pose / 协方差 + twist / 协方差
pose 在 header.frame_id；twist 在 child_frame_id

同样 x = 1.00 m：方差 0.0025 m² → σ = 0.05 m
                 方差 0.25 m²   → σ = 0.50 m

状态值为 0 ≠ 方差为 0 ≠ 没有观测
1e6 是大方差示例；忽略哪些量，应核对下游融合配置
```

### 导入实验：用官方消息接口区分“数值相同”与“不确定性相同”

**实验目标：**查看官方 Odometry 结构，构造两份位置相同而方差不同的消息，观察不确定性并不会从位置数值中自动产生。

来源：上述 Humble 官方消息定义，以及 ROS 2 官方[话题教程中的接口查看步骤](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Beginner-CLI-Tools/Understanding-ROS2-Topics/Understanding-ROS2-Topics.rst#6-ros2-interface-show)。使用官方 `ros2 interface show` 和生成的 Python 消息类；下面两份消息的构造与对比是课堂改编，不是官方完整融合示例。

**准备条件：**Ubuntu 22.04 或 WSL Ubuntu 22.04，已安装 ROS 2 Humble 与 `nav_msgs`、`geometry_msgs`、`ros2interface`。无需底盘、图形桌面或正在运行的节点；使用系统 `python3`，在 Ubuntu Bash 中执行：

```bash
source /opt/ros/humble/setup.bash
ros2 interface show nav_msgs/msg/Odometry
ros2 interface show geometry_msgs/msg/PoseWithCovariance
```

**① 在输出中找到四个字段。**

记录 `header`、`child_frame_id`、`pose`、`twist`；检查 pose 内部既有 quaternion，又有长度为 36 的 covariance。说明“姿态四元数四个数”为什么不意味着协方差有七行七列。

**② 构造两份只在 z 方差上不同的课堂消息。**

下面代码只在当前进程中检查消息对象，不发布话题、不运行滤波器。零方差那份是为了演示容易发生的误填，不是建议用于实际系统。时间戳未设置为采样时间，其余未赋值字段仍为默认值；这两份对象不能直接作为完整有效的实机数据发布。

```bash
python3 - <<'PY'
from copy import deepcopy
from math import sqrt
from nav_msgs.msg import Odometry

large = Odometry()
large.header.frame_id = 'odom'
large.child_frame_id = 'base_footprint'
large.pose.pose.position.x = 1.0
large.pose.pose.position.z = 0.0
large.pose.pose.orientation.w = 1.0

# Classroom values, not calibrated sensor parameters.
# Order: x, y, z, rotation about X, rotation about Y, rotation about Z.
diagonal = [0.0025, 0.0025, 1e6, 1e6, 1e6, 0.01]
covariance = [0.0] * 36
for i, variance in enumerate(diagonal):
    covariance[6 * i + i] = variance
large.pose.covariance = covariance

zero = deepcopy(large)
zero.pose.covariance[14] = 0.0  # row 2, column 2: z variance

for label, msg in [('large_variance', large), ('zero_variance', zero)]:
    variance = msg.pose.covariance[14]
    print(f'{label}: z={msg.pose.pose.position.z:.1f} m, '
          f'variance_z={variance:g} m^2, sigma_z={sqrt(variance):g} m')

assert len(large.pose.covariance) == 36
assert large.pose.pose.position.z == zero.pose.pose.position.z
assert large.pose.covariance[14] != zero.pose.covariance[14]
PY
```

**③ 记录并解释结果。**

```text
large_variance: z=0.0 m, variance_z=1e+06 m^2, sigma_z=1000 m
zero_variance: z=0.0 m, variance_z=0 m^2, sigma_z=0 m
```

两份消息的 z 都是 0，但对它的不确定性声明相差很大。代码只是把人为指定的方差写进数组，不会因为创建了 Odometry 对象就自动知道真实误差；打印出的标准差也是这些假设的结果。

**验收：**提交两行输出，指出位置值有没有变化、哪一项方差改变；解释为什么“没有高度观测”不能由零方差代替表达，并回答为什么本实验没有证明 `1e6` 适合项目、也没有证明滤波器一定忽略了 z。

**排查与结束：**找不到接口或无法导入 `nav_msgs` 时，先确认已 source Humble、依赖已安装且使用系统 Python；数组长度必须为 36，z 的对角项为从零计数的索引 14。脚本结束自动退出，不产生文件或后台进程。本页只验证消息结构和数字含义，融合效果与实测协方差需后续实验评估。

### 课堂提问

> 两条里程计的位置、速度都相同，是否就包含完全相同的信息？

参考答案：不一定。时间、参考系及协方差还可能不同；不确定性不同会影响下游对这些数值的使用。

> 把 x 的方差从 0.25 改成 0.0025，是否就把传感器精度提高了十倍？

参考答案：没有改变传感器或估计方法，只把声明的标准差从 0.50 m 改成 0.05 m。需要实测依据，否则可能造成过度自信。

> 不想让 robot_localization 使用某个变量，是否只要写 `1e6`？

参考答案：不应这样代替配置。应在对应传感器的融合配置中选择变量，并按需要检查平面模式；大方差没有统一的“禁用”含义。

这页最后可以总结：

> **里程计既报告估计值，也交代时空参考和不确定性；数值为零不代表误差为零，没有观测更不等于完全确定，协方差与下游配置必须一起理解。**

---

## 第 25 页｜什么是里程计 & Odometry 消息结构

这页你可以把它讲成：

> **里程计是机器人对自身运动的估计。在本项目里，我们把反馈速度与航向按时间累计，得到位姿，再将位姿、速度及它们的不确定性装进 Odometry。读这条消息时，先看时间和坐标系，再读数值：位置在哪个参考里，速度沿哪组轴，哪份协方差对应哪组量。两层 pose、两层 twist 看起来重复，实际是外层包装不确定性、内层存具体运动数值。**

### 讲解与例子

**1. 里程计是运动估计，不只是“走过多少米”。**

日常里程表常表示累计路程，本课的机器人里程计还包括位置、朝向与运动速度。机器人先前进再退回起点，累计路程会增加，但最终位置可以接近初始位置；不能把路程总和直接填进位置 x。

本项目使用轮速反馈与融合航向进行航位推算。一般意义上的里程计还可来自视觉等运动估计方式；`nav_msgs/msg/Odometry` 是承载结果的消息接口，其名称并不限定必须采用哪一种传感器或积分算法。

**2. 把 PPT 中的展开结构还原为明确的父子层级。**

依据 Humble [Odometry 官方定义](https://github.com/ros2/common_interfaces/blob/humble/nav_msgs/msg/Odometry.msg)，可先画下面这棵字段树。缩进表示包含关系，并不是把所有字段平铺到同一层：

```text
Odometry
├── header
│   ├── stamp                         数据对应的时间
│   └── frame_id                      pose 的参考系，例如 odom
├── child_frame_id                    车体坐标系，例如 base_footprint
├── pose                             PoseWithCovariance
│   ├── pose                         Pose
│   │   ├── position                 x、y、z
│   │   └── orientation              四元数 x、y、z、w
│   └── covariance                   36 个元素，6×6 位姿协方差
└── twist                            TwistWithCovariance
    ├── twist                        Twist
    │   ├── linear                   三轴线速度
    │   └── angular                  三轴角速度
    └── covariance                   36 个元素，6×6 速度协方差
```

因此 Python 中位置 x 的路径为 `msg.pose.pose.position.x`，前向速度为 `msg.twist.twist.linear.x`。相应的不确定性位于 `msg.pose.covariance` 与 `msg.twist.covariance`，不要误写成 `msg.pose.pose.covariance`。

| 要读取的信息 | 字段路径 | 单位或含义 |
| --- | --- | --- |
| 数据时间 | `header.stamp` | 秒与纳秒组成的时间戳 |
| 位姿参考系 | `header.frame_id` | 本项目示例为 `odom` |
| 被描述的车体坐标系 | `child_frame_id` | 本项目示例为 `base_footprint`，也是 twist 的表达坐标系 |
| 位置 | `pose.pose.position` | m |
| 姿态 | `pose.pose.orientation` | 单位四元数，不是四个角度 |
| 线速度 | `twist.twist.linear` | m/s |
| 角速度 | `twist.twist.angular` | rad/s |

时间戳应反映数据对应的时刻；同一条消息中的估计要在时间上相互对应。真实驱动不能因为“现在发布”，就无条件把一份较早采集的反馈标成当前采样。本页实验会使用当前时间表示人为构造的快照，不承担真实采集与同步任务。[Header 官方定义](https://github.com/ros2/common_interfaces/blob/humble/std_msgs/msg/Header.msg)给出了时间与参考系字段。

**3. pose 和 twist 的坐标系分工必须按消息定义落实。**

PPT 以 `odom` 与 `base_footprint` 为例：pose 表示车体在 odom 中的位置与姿态；twist 则把运动速度按车体轴向表达。这里的 odom 是连续但会漂移的局部参考，不等于已经完成地图定位或 GPS 定位。

给一个与前面旋转矩阵相接的例子：机器人在 odom 中位于 `(1, 2)` m，车头相对 odom 左转 90°，此刻沿自身前向以 0.2 m/s 行驶。消息中的 `twist.twist.linear.x` 可以是 0.2，而不是因为 odom 中的运动朝正 y 就改填 `linear.y = 0.2`。若要得到 odom 中的速度，需要结合姿态转换。

准确地说，**pose 使用 `header.frame_id`，twist 使用 `child_frame_id`**，这是 Odometry 消息注释直接规定的；`odom`、`base_footprint` 是本项目的具体名称，不是消息类型强制写死的字符串。前文 REP-105 说明的是坐标系角色，两者不要混作同一条规定。

**4. 两份 6×6 协方差的尺寸相同，描述的变量不同。**

按照 [PoseWithCovariance](https://github.com/ros2/common_interfaces/blob/humble/geometry_msgs/msg/PoseWithCovariance.msg) 与 [TwistWithCovariance](https://github.com/ros2/common_interfaces/blob/humble/geometry_msgs/msg/TwistWithCovariance.msg) 的官方定义，数组以行优先方式存储：

```text
pose 协方差的分量顺序：
[x, y, z, 绕固定 X 轴旋转, 绕固定 Y 轴旋转, 绕固定 Z 轴旋转]
本课常简记为 [x, y, z, roll, pitch, yaw]

twist 协方差的分量顺序：
[vx, vy, vz, wx, wy, wz]
对应 linear.x/y/z 与 angular.x/y/z

行优先索引：covariance[6*i + j] = 第 i 行、第 j 列，i、j 从 0 开始
六个对角项索引：0、7、14、21、28、35
```

PPT 最后给出的 `[x, y, z, roll, pitch, yaw]` 有助于记住六个方向，但讲速度协方差时要改读为线速度与角速度，不能把其中的 yaw 当成偏航角本身。[Twist 官方定义](https://github.com/ros2/common_interfaces/blob/humble/geometry_msgs/msg/Twist.msg)包含完整三轴向量；PPT 写的 Vx、Vy、ω 是本项目平面运动的常用分量，不代表消息里只有这三个字段。一般三维运动中，三轴角速度也不直接等于三个欧拉角的时间导数。

例如，`pose.covariance[0]` 是 x 位置方差，单位 m²；`twist.covariance[0]` 是 x 向线速度方差，单位 (m/s)²。它们不能因为索引相同就共用一个数。四元数虽然存四个分量，位姿协方差仍按三个旋转误差方向表达，不变成 7×7。

**5. Odometry 消息和 TF 相互配合，但不会自动互相生成。**

发布 Odometry 不会自动广播 `odom → base_footprint` 的 TF；驱动或指定的估计节点仍需按职责发布变换。同一时刻，消息的 pose 与对应 TF 应一致，不能出现两套不同的位置与朝向。

TF 表达坐标关系，Odometry 额外承载速度及两份协方差；二者不能完全互相替代。本页先学会读结构，不在这里展开协方差估计或实机定位精度验证。

### 板书

```text
机器人里程计 ≠ 只累计路程
本项目：运动反馈 + 航向 + 时间 → 位姿与速度估计

msg.pose.pose.position.x          位置
msg.pose.covariance               位姿不确定性
msg.twist.twist.linear.x          线速度
msg.twist.covariance              速度不确定性

pose 看 header.frame_id；twist 看 child_frame_id
两份 covariance 各 36 项；对角索引 0、7、14、21、28、35
发布 Odometry ≠ 自动发布 TF
```

### 实验：用官方命令发布并逐字段读取一条 Odometry 快照

**实验目标：**在接收端找到嵌套字段、参考系及两份协方差，结合姿态解释速度，而不是只看字段名字猜方向。

来源：ROS 2 Humble 官方[话题教程](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Beginner-CLI-Tools/Understanding-ROS2-Topics/Understanding-ROS2-Topics.rst)中的 `interface show`、`topic echo` 与 `topic pub`。本实验使用官方命令，Odometry 数据是课堂改编的单时刻样例，不是底盘实测或官方完整里程计驱动。

**准备条件：**Ubuntu 22.04 或 WSL Ubuntu 22.04，已安装 ROS 2 Humble，以及 `nav_msgs`、`ros2interface`、`ros2topic`。无需底盘或图形桌面。结束之前的实验进程，在 A、B 两个 Ubuntu Bash 终端中**分别执行**；若 99 已被占用，统一换用空闲 Domain ID：

```bash
source /opt/ros/humble/setup.bash
export ROS_DOMAIN_ID=99
export ROS_LOCALHOST_ONLY=1
```

**① 终端 A 查看官方接口。**

```bash
ros2 interface show nav_msgs/msg/Odometry
ros2 interface show geometry_msgs/msg/TwistWithCovariance
```

对照字段树找到两份 covariance，确认各有 36 个元素；注意 `twist` 外层包装与内层向量不是同一层。

**② 先在终端 B 等待接收。**显式写出类型，允许在发布者启动前建立订阅；收到一条后自动退出：

```bash
ros2 topic echo /lab/odom nav_msgs/msg/Odometry --once
```

**③ 再在终端 A 发送一条样例。**

```bash
ros2 topic pub --once -w 1 /lab/odom nav_msgs/msg/Odometry '
header:
  stamp: now
  frame_id: odom
child_frame_id: base_footprint
pose:
  pose:
    position: {x: 1.0, y: 2.0, z: 0.0}
    orientation: {x: 0.0, y: 0.0, z: 0.7071067811865475, w: 0.7071067811865476}
  covariance: [0.0025, 0, 0, 0, 0, 0,
               0, 0.01, 0, 0, 0, 0,
               0, 0, 1000000.0, 0, 0, 0,
               0, 0, 0, 1000000.0, 0, 0,
               0, 0, 0, 0, 1000000.0, 0,
               0, 0, 0, 0, 0, 0.04]
twist:
  twist:
    linear: {x: 0.2, y: 0.1, z: 0.0}
    angular: {x: 0.0, y: 0.0, z: 0.05}
  covariance: [0.0004, 0, 0, 0, 0, 0,
               0, 0.0009, 0, 0, 0, 0,
               0, 0, 1000000.0, 0, 0, 0,
               0, 0, 0, 1000000.0, 0, 0,
               0, 0, 0, 0, 1000000.0, 0,
               0, 0, 0, 0, 0, 0.0025]'
```

`--once` 表示只发一条，`-w 1` 等待至少一个匹配订阅者；若一直等待，先检查 B 是否已启动。`stamp: now` 使用 Humble 命令行支持的当前时间展开，表示这个教学快照的时间。真实驱动仍应按数据时间设计赋值。

本例沿用上一页的大方差示意，仅帮助认出矩阵位置；所有方差均为教学假设，不是项目标定值，也不替代下游变量选择配置。这里只发布一个运动瞬间，不构成连续里程计轨迹，且没有广播 TF。

**④ 从 B 的输出完成字段核对。**

| 检查项 | 预期结果 |
| --- | --- |
| 时间与参考系 | 时间戳已展开；`frame_id` 为 `odom`，`child_frame_id` 为 `base_footprint` |
| 位姿 | odom 中位置 `(1, 2, 0)` m；四元数对应约 +90° 偏航 |
| 速度 | 车体系线速度 `(0.2, 0.1, 0)` m/s；绕 z 角速度 0.05 rad/s |
| 位姿协方差 | 36 项；索引 0 为 0.0025，索引 35 为 0.04 |
| 速度协方差 | 另有 36 项；索引 0 为 0.0004，索引 35 为 0.0025 |

**再做一次解释而非改写消息：**将车体系速度按约 90° 偏航旋转到 odom，得到 `vx_odom ≈ -0.1 m/s`、`vy_odom ≈ 0.2 m/s`。B 中的原始 twist 仍应是 `(0.2, 0.1)`，不会因为 echo 显示它就自动转换坐标系。

**验收：**保存 B 的一次输出，标出 `pose.pose`、`twist.twist` 及两份 covariance；写出上述速度转换结果，并说明哪份索引 0 的单位是 m²、哪份是 (m/s)²，以及为什么本实验结束后不会自动得到一条 TF 边。

**排查与结束：**无输出时核对两个终端的 ROS 环境、Domain ID、话题名和消息类型；已错过单次消息时重新启动 B，再在 A 重发。YAML 报错时检查缩进、外层单引号及两组数组各 36 项。A、B 正常收到后均自行退出；仍在等待时按 `Ctrl+C`。实验不发送速度控制指令，不生成文件。

### 课堂提问

> 为什么读取位置要写两次 pose？

参考答案：外层是包含 pose 和 covariance 的 PoseWithCovariance，内层 Pose 才包含 position 与 orientation。

> 机器人朝 odom 的正 y 方向前进，twist.linear.x 是否必须为零？

参考答案：不是。要看 child_frame_id 的轴向；车头相对 odom 偏航 90° 时，自身前向速度仍可记为车体系的正 x 分量。

> 两份 covariance 的最后一个对角项，是否都表示 yaw 角方差？

参考答案：不是。pose 中对应绕 z 的姿态误差方差；twist 中对应 z 轴角速度方差，变量和单位都不同。

这页最后可以总结：

> **读 Odometry 要顺着层级找数值，按时间和参考系解释运动，并分别理解位姿与速度的不确定性；消息装对了，才有条件让下游正确使用估计结果。**

---

## 第 26 页｜协方差矩阵：不确定度的“官方语言”

这页你可以把它讲成：

> **上一页认识了消息里的两组 36 个数，这一页要读懂这些数在说什么。对角线说明各个方向有多大不确定性，其他位置说明误差之间怎样共同变化。下游算法可以据此决定一份观测应当产生多大影响，但前提是这些声明合理。不能为了让滤波结果好看就随意缩小方差，也不能把没有测到的量包装成完全确定的零。**

### 讲解与例子

**1. 协方差描述误差的分布，不是一次误差的大小。**

以平面位置为例，设估计误差为 `ex = 估计 x - 参考 x`、`ey = 估计 y - 参考 y`。在所采用的误差模型中：

```text
Var(ex)     = E[(ex - E[ex])²]
Cov(ex, ey) = E[(ex - E[ex])·(ey - E[ey])]
```

对角项是方差，标准差为其平方根；非对角项是协方差。这里的 E 表示期望，不要求学生在本页完成统计估计推导。一次测量偏了多少，与描述许多可能误差的分布，是两种不同的信息。

协方差本身也不会完整记录固定偏差。例如每次都偏 10 cm，但波动很小，围绕均值计算的方差仍可能很小。因此还要检查偏差、参考真值与模型假设，不能只凭“方差小”认定估计准确。

**2. 从 6×6 矩阵到 36 项数组，保留上一页的变量顺序。**

Humble [PoseWithCovariance 官方定义](https://github.com/ros2/common_interfaces/blob/humble/geometry_msgs/msg/PoseWithCovariance.msg)规定行优先存储，以及三个平移和三个固定轴旋转分量的顺序。以位姿协方差记为 Σ，可写成：

```text
              x       y       z      roll    pitch     yaw
x           Var(x)  Cov(x,y)   …        …       …        …
y          Cov(y,x)  Var(y)    …        …       …        …
z              …       …    Var(z)     …       …        …
roll           …       …      …    Var(roll)   …        …
pitch          …       …      …        …   Var(pitch)   …
yaw            …       …      …        …       …     Var(yaw)

covariance[6*i + j] = Σ[i,j]，i、j 从 0 开始
对角项：索引 0、7、14、21、28、35
对称项：例如 covariance[1] 与 covariance[6] 都是 x、y 的协方差
```

合法的协方差矩阵应对称且半正定：沿任意组合方向算出的方差都不能为负。仅把上下三角填得一样，还不足以保证合法。[SciPy 官方多元正态分布文档](https://docs.scipy.org/doc/scipy/reference/generated/scipy.stats.multivariate_normal.html)也以对称半正定矩阵描述协方差；本页不要求学生使用该软件或计算全部特征值。

速度协方差沿用 `[vx, vy, vz, wx, wy, wz]` 的顺序。位置方差单位为 m²，姿态角方差为 rad²，线速度方差为 (m/s)²；不同种类的数不能脱离单位直接比较“谁更可信”。

**3. 把 PPT 的三个数量级翻译回物理尺度。**

| PPT 中的方差示例 | 标准差 √方差 | 正确的讲解边界 |
| --- | --- | --- |
| x 或 y 方差 `1e-3 m²` | 约 0.0316 m，即 3.16 cm | 表示模型声明的位置不确定性尺度；不能据此认定所有编码器里程计都达到此精度 |
| yaw 方差 `1e-2 rad²` | 0.1 rad，约 5.73° | 是角度尺度，不是 0.01 rad 的标准差；是否足够要看任务 |
| 位置方差 `1e6 m²` | 1000 m | 相对于室内定位通常非常大，但仍是有限方差，不是禁用标记 |

若 `1e6` 填在角度方差上，平方根的单位就是 rad；PPT 最后一行的“1000”必须结合变量解释。“较可信”“中等可信”是课堂直觉，不是 ROS 官方给出的等级或项目实测结论。

标准差也不是保证误差绝不超出的界限。没有说明分布模型时，不能把“σ 为 3.16 cm”直接改讲成“所有误差都不超过 3.16 cm”。

**4. 非对角项是协方差，不是无单位的相关系数。**

PPT 将非对角项概括成“相关性”，实际存储的是协方差。两者在标准差非零时通过以下关系相连：

```text
相关系数 ρxy = Cov(ex, ey) / (σx·σy)
|ρxy| ≤ 1；Cov(ex, ey) 的单位是两种误差单位的乘积
```

例如 x、y 方差都为 `0.001 m²`，协方差为 `0.0004 m²`，则相关系数为 0.4。正协方差表示两种中心化误差倾向于同号变化；负协方差表示倾向于反号变化。它不表示机器人一定向某个方向运动。

非对角项置零，是采用误差不相关的模型简化，不能证明真正独立。共同的航向误差、速度来源或坐标变换都可能引入相关性。这里先说明含义，不要求本页给出项目全部交叉项的标定结果。

**5. 用一维更新说明零方差与大方差的影响。**

从官方 [robot_localization 的 Humble EKF 校正实现](https://github.com/cra-ros-pkg/robot_localization/blob/humble-devel/src/ekf.cpp)中的矩阵更新，取只有一个状态、直接观测该状态、预测误差与观测误差不相关的特例：

```text
预测状态 x⁻，预测方差 P；观测值 z，观测方差 R
K = P / (P + R)
x⁺ = x⁻ + K·(z - x⁻)
```

先固定 `P = 0.04`。R 越大，观测对本次更新的影响越小；理想公式中 `R = 0` 且 `P > 0` 时，K = 1，状态会直接跟随观测。这说明零方差不是“不知道”的自然写法。这里的 z 是观测符号，不是特指高度坐标。

但 **R 必须相对于 P 来判断大小**。`R = 1e6`、`P = 0.04` 时 K 约为 `4e-8`；若 P 也增大到 `1e8`，同一个 R 会得到约 0.9901 的 K。即使 R 数值很大，也不保证算法忽略它；多维更新还涉及状态相关性等因素。

PPT 的“填 0 表示 100% 确定”可用于解释理想方差含义，但具体实现可能对过小方差加下限，不能断言所有滤波器都会按原始零值计算或一定崩溃。该 EKF 实现就包含小方差保护；保护措施不等于误填已经合理。

对于真正没有提供的观测，应明确选择哪些量参与融合。Humble [robot_localization 数据准备说明](https://github.com/cra-ros-pkg/robot_localization/blob/humble-devel/doc/preparing_sensor_data.rst)建议用对应配置忽略变量，不靠夸大方差代替配置。因此 PPT 的“告诉下游别用”应解释为大方差在特定条件下使影响减小，而不是统一的自动屏蔽协议。

### 板书

```text
对角：方差 σ²；非对角：协方差；标准差 σ = √方差
Σ 对称、半正定；行优先 36 项

0.001 m² → 约 0.0316 m
0.01 rad² → 0.1 rad → 约 5.73°
1e6 m² → 1000 m（有限大数，不是“禁用”）

一维、直接观测：K = P/(P+R)
固定 P，R 增大 → K 减小；R=0、P>0 → 理想 K=1
不知道某个量 ≠ 精确知道它为 0
```

### 实验：按官方消息布局检查协方差，再比较一维融合权重

**实验目标：**验证标准差换算、行优先与对称项，观察相同观测在不同方差声明下产生的更新差异。

来源：上述官方 PoseWithCovariance 定义与 robot_localization EKF 实现。先用官方命令查看接口，再用本页编写的纯 Python 数值实验提取矩阵布局及一维更新原理。该脚本是教学简化，不是官方完整 EKF 示例，不运行 ROS 滤波节点，也不复现其异常拒绝、方差保护或时间预测流程。

**准备条件：**接口查看在已安装 ROS 2 Humble、`geometry_msgs`、`ros2interface` 的 Ubuntu 22.04 或 WSL Ubuntu 中进行；数值脚本只需系统 Python 3 标准库，无需底盘、网络或图形桌面。在 Ubuntu Bash 中执行：

```bash
source /opt/ros/humble/setup.bash
ros2 interface show geometry_msgs/msg/PoseWithCovariance
```

**① 对照官方注释确认长度、顺序与行优先。随后运行数值脚本。**

```bash
python3 - <<'PY'
from math import sqrt, degrees, isclose

# Classroom pose covariance in the official row-major layout.
diagonal = [1e-3, 1e-3, 1e6, 1e6, 1e6, 1e-2]
cov = [0.0] * 36
for i, variance in enumerate(diagonal):
    cov[6 * i + i] = variance
cov[1] = cov[6] = 4e-4

assert len(cov) == 36
assert all(cov[6*i+j] == cov[6*j+i] for i in range(6) for j in range(6))
assert cov[1]**2 <= cov[0] * cov[7]
print(f'sigma_x={sqrt(cov[0]):.6f} m')
print(f'sigma_yaw={sqrt(cov[35]):.6f} rad = {degrees(sqrt(cov[35])):.3f} deg')
print(f'rho_xy={cov[1] / sqrt(cov[0] * cov[7]):.3f}')

# Each row starts from the same prior: these are independent comparisons.
prior_x, observation = 0.0, 1.0
prior_variance = 0.04
for r in (1e-3, 1e-2, 1e6, 0.0):
    gain = prior_variance / (prior_variance + r)
    updated = prior_x + gain * (observation - prior_x)
    print(f'R={r:g}: K={gain:.9f}, updated_x={updated:.9f} m')

gain_large_prior = 1e8 / (1e8 + 1e6)
assert isclose(gain_large_prior, 100.0 / 101.0)
print(f'P=1e8, R=1e6: K={gain_large_prior:.9f}')

# Symmetry alone is insufficient: variance of error_x - error_y must be >= 0.
bad_cov_xy = 0.002
bad_difference_variance = cov[0] + cov[7] - 2 * bad_cov_xy
assert bad_difference_variance < 0
print(f'invalid xy block: Var(error_x-error_y)={bad_difference_variance:.6f} m^2')
PY
```

**② 对照预期结果，解释每一项。**

```text
sigma_x=0.031623 m
sigma_yaw=0.100000 rad = 5.730 deg
rho_xy=0.400
R=0.001: K=0.975609756, updated_x=0.975609756 m
R=0.01: K=0.800000000, updated_x=0.800000000 m
R=1e+06: K=0.000000040, updated_x=0.000000040 m
R=0: K=1.000000000, updated_x=1.000000000 m
P=1e8, R=1e6: K=0.990099010
invalid xy block: Var(error_x-error_y)=-0.002000 m^2
```

四个 R 的比较每次都从同一个预测开始，不是依次更新四遍。位置观测都为 1 m，数值相同而方差不同，产生的修正也不同。R = 0 一行只展示理想公式，不能当作真实软件会原样采用零方差的运行结果。

最后一项故意把 x、y 协方差改得过大：虽然上下对称、对角线仍为正，却让误差差值的方差变成负数，说明这组矩阵不合法。脚本中的对称与二元界限检查不是任意 6×6 矩阵的完整合法性检验；本例之外仍需检查半正定性等条件。

**验收：**提交输出，解释 `0.01 rad²` 为什么对应约 5.73°；比较两种 P 下同为 `1e6` 的 R，说明为什么不能把它当作自动忽略开关；再指出最后一行暴露的矩阵问题。

**排查与结束：**接口找不到时检查 ROS 环境与依赖；数值不符时检查是否把标准差直接填进方差、角度与弧度是否混用，以及对称项的索引。纯数值部分无需 ROS Python 模块，脚本结束自行退出，不产生文件或后台进程。这里验证的是数学与布局，不证明项目协方差已标定或实际 EKF 融合有效。

### 课堂提问

> 非对角项能不能随便写成 0.8，表示“有八成相关”？

参考答案：不能。数组存的是带单位的协方差，不是相关系数；要结合两边的标准差换算，并保证整体矩阵合法。

> 方差越小，是否就说明这份数据一定越准确？

参考答案：它只是声明的不确定性较小；模型错误、固定偏差或人为低报方差都可能导致过度自信，需要实测与参考检验。

> 给一个未测量方向填 `1e6`，是否就完成了下游融合设置？

参考答案：没有。有限方差的影响依赖预测不确定性与算法；应按消费者约定配置使用哪些变量，并明确模型约束与真实观测的区别。

这页最后可以总结：

> **协方差要同时读数值、单位和变量关系；诚实报告不确定性，并明确下游是否使用该量，比用零值或大数代替判断更可靠。**

---

## 第 27 页｜本项目的协方差设计

这页你可以把它讲成：

> **前两页讲清了结构和含义，现在把 PPT 给出的项目数值放回具体字段。位置与速度各有一份矩阵，它们的六个对角项要分别赋值。平面位置、平面速度和航向相关量是本页重点，其余方向用大方差表达当前设计没有提供可靠估计。读这些数字时，还要问清数据来源、单位以及依据；代码填对只能证明声明被写进了消息，不能证明声明已经符合实际误差。**

### 讲解与例子

**1. 先把左右两栏变成一张赋值对照表。**

以下数值按本页 PPT 整理。当前课程目录没有找到第九课底盘驱动源码，因此它们是**课件给出的设计值**，尚不能当作已经核对过的实机代码或标定结果。依据 Humble 官方 [PoseWithCovariance](https://github.com/ros2/common_interfaces/blob/humble/geometry_msgs/msg/PoseWithCovariance.msg) 与 [TwistWithCovariance](https://github.com/ros2/common_interfaces/blob/humble/geometry_msgs/msg/TwistWithCovariance.msg) 的布局，对照如下：

| 对角索引 | pose 中的量 | pose 方差 | twist 中的量 | twist 方差 |
| --- | --- | --- | --- | --- |
| 0 | x 位置 | `1e-3 m²` | vx 线速度 | `1e-3 (m/s)²` |
| 7 | y 位置 | `1e-3 m²` | vy 线速度 | `1e-3 (m/s)²` |
| 14 | z 位置 | `1e6 m²` | vz 线速度 | `1e6 (m/s)²` |
| 21 | 绕 x 的姿态误差（roll） | `1e6 rad²` | wx 角速度 | `1e6 (rad/s)²` |
| 28 | 绕 y 的姿态误差（pitch） | `1e6 rad²` | wy 角速度 | `1e6 (rad/s)²` |
| 35 | 绕 z 的姿态误差（yaw） | `1e-2 rad²` | wz 角速度 | `1e-3 (rad/s)²` |

因此两组对角线分别为：

```text
pose： [0.001, 0.001, 1000000, 1000000, 1000000, 0.01]
twist：[0.001, 0.001, 1000000, 1000000, 1000000, 0.001]
```

这两行各有 6 个数，只是对角线摘要，不是能够直接赋给 covariance 的完整数组；实际字段各要 36 项。本页实验将其展开，非对角项采用零的教学简化。

**2. 把“为什么这样设计”接回运动信息的来源。**

按 PPT 的设计说明，平面位置由反馈速度结合航向进行位移积分得到，平面速度来自编码器反馈换算，航向来自下位机 IMU 融合姿态；课件还说明 z 轴角速度来自陀螺仪瞬时测量。后续实操应沿串口解析变量和赋值链核实，不能仅因消息里字段名为 `angular.z` 就认定来源必然是陀螺仪。

位置积分还会受到时间间隔、轮径误差、打滑和航向误差影响。“编码器积分的位置可信”应讲成项目对平面估计作出了这一不确定性声明，不能解释成长期位置误差总是保持几厘米。固定方差也不会自动随行驶时间传播误差。

瞬时速度测量避免了在该字段中再次累计全部历史误差，但仍可能有零偏、噪声、比例误差或打滑影响；“直接测量”本身并不保证高精度。是否合理，需要结合实际输出、标定和运动场景评估。

**3. 高度、倾角的大方差，描述的是这份里程计的建模范围。**

这里宜表述为“本页里程计设计没有为这些方向提供可靠估计”，而不是“所有地面机器人都永远无法观测 z、roll、pitch”。例如 IMU 可为倾角提供信息，但本页 Odometry 是否使用这些信息，是另一项实现选择。

同样，平面假设可以提供模型约束；它与某个传感器实际测到了高度或倾角不同。`1e6` 沿用上一页的大方差做法，不是自动禁用协议。对 robot_localization，应按其[数据准备说明](https://github.com/cra-ros-pkg/robot_localization/blob/humble-devel/doc/preparing_sensor_data.rst)明确配置使用哪些变量，而不是单靠放大方差代替配置。

本项目是可横移的麦克纳姆轮底盘，不能照抄差速底盘“vy 恒为零”的运动约束。[robot_localization 官方配置说明](https://github.com/cra-ros-pkg/robot_localization/blob/humble-devel/doc/configuring_robot_localization.rst)中的非完整约束轮式机器人示例依赖其运动假设，不应原样用于本项目。

**4. PPT 的关键对比要保留物理单位。**

```text
pose 的 yaw 方差：0.01 rad² → 标准差 0.1 rad，约 5.73°
twist 的 wz 方差：0.001 (rad/s)² → 标准差约 0.0316 rad/s，约 1.81°/s
```

右栏标注的“yaw”应在讲解中读成 z 轴角速度 wz。它与左栏的偏航角不是同一种物理量，因此不能把两个方差相除就说“角速度比航向可信十倍”。两份矩阵虽然都用索引 35，该索引承载的变量不同。

PPT 用“瞬时测量”和“融合后的积分量”解释设计动机，可以帮助学生理解累计误差的来源；更严谨的结论是：**这些数值表达了不同变量各自的不确定性假设，是否合理必须分别验证。**融合航向也可能包含外部约束，不能把所有融合姿态都概括成未经修正的陀螺积分。

**5. 赋值检查与可信度验证分开做。**

本页先检查矩阵长度、索引、单位、对称性，以及代码是否按课件声明赋值。后续逐行对照真实代码时，还要确认速度与姿态来源、坐标变换、角度单位和时间处理，检查同一消息的 pose 与 twist 是否被误用为独立信息重复融合。

官方配置说明专门提醒：来自同一运动源的位姿与速度可能重复提供信息。本项目的轮速、融合航向及其他 IMU 输入是否共享来源，也要在配置时考虑；不能因写在不同字段或话题中就视为独立传感器。

### 板书

```text
                   0      7      14     21     28     35
pose 对角：       1e-3   1e-3   1e6    1e6    1e6    1e-2
twist 对角：      1e-3   1e-3   1e6    1e6    1e6    1e-3

pose[35]：航向角误差方差，rad²
twist[35]：z 轴角速度方差，(rad/s)²
数值大小不能跨单位直接比较可信度

课件设计值 → 正确写入 36 项数组 → 核对实际数据链 → 实测评估
```

### 实验：把本页两组设计值展开，并写入官方 Odometry 消息

**实验目标：**将 PPT 对角项正确展开成两份行优先数组，区分 pose 与 twist 的最后一项，并检查消息中的实际赋值。

来源：上述 Humble 官方消息定义。本实验使用官方生成的消息类，赋值与核对步骤按本页 PPT 改编，不是官方完整底盘驱动示例，也不运行 EKF 或修改项目驱动。

**准备条件：**第一段数值检查只需 Python 3 标准库；第二段消息检查需要 Ubuntu 22.04 或 WSL Ubuntu 的 ROS 2 Humble、`nav_msgs` 及其依赖，使用系统 Python。在 Ubuntu Bash 中执行以下数值检查，无需底盘或图形桌面。

**① 展开并打印两份矩阵。**

```bash
python3 - <<'PY'
from math import sqrt, degrees

def expand(diagonal):
    if len(diagonal) != 6 or any(value < 0 for value in diagonal):
        raise ValueError('Expected six non-negative diagonal variances')
    result = [0.0] * 36
    for i, value in enumerate(diagonal):
        result[6*i+i] = float(value)
    return result

pose = expand([1e-3, 1e-3, 1e6, 1e6, 1e6, 1e-2])
twist = expand([1e-3, 1e-3, 1e6, 1e6, 1e6, 1e-3])

for name, covariance in [('pose', pose), ('twist', twist)]:
    print(name)
    for row in range(6):
        print(' '.join(f'{value:g}' for value in covariance[6*row:6*row+6]))
    assert len(covariance) == 36
    assert all(covariance[6*i+j] == 0.0
               for i in range(6) for j in range(6) if i != j)

assert [i for i in range(36) if pose[i] != twist[i]] == [35]
print(f'pose yaw sigma: {sqrt(pose[35]):.6f} rad = {degrees(sqrt(pose[35])):.3f} deg')
print(f'twist wz sigma: {sqrt(twist[35]):.6f} rad/s = {degrees(sqrt(twist[35])):.3f} deg/s')
PY
```

预期各打印 6 行、每行 6 项；非对角项为零。两组矩阵只有索引 35 不同，最后两行约为：

```text
pose yaw sigma: 0.100000 rad = 5.730 deg
twist wz sigma: 0.031623 rad/s = 1.812 deg/s
```

这些零非对角项表达本实验采用的无相关性假设，不证明实际误差独立。两份对角矩阵形式合法，也不意味着数值已经经过实测标定。

**② 在 ROS 环境中核对正式消息字段。**

以下是独立脚本，不依赖上一段的进程变量。为完整展示字段赋值，重新生成数组：

```bash
source /opt/ros/humble/setup.bash
python3 - <<'PY'
from nav_msgs.msg import Odometry

def expand(diagonal):
    result = [0.0] * 36
    for i, value in enumerate(diagonal):
        result[6*i+i] = float(value)
    return result

msg = Odometry()
msg.pose.covariance = expand([1e-3, 1e-3, 1e6, 1e6, 1e6, 1e-2])
msg.twist.covariance = expand([1e-3, 1e-3, 1e6, 1e6, 1e6, 1e-3])

indices = [0, 7, 14, 21, 28, 35]
for index in indices:
    print(f'index={index}: pose={msg.pose.covariance[index]:g}, '
          f'twist={msg.twist.covariance[index]:g}')
assert msg.pose.covariance[35] == 0.01
assert msg.twist.covariance[35] == 0.001
PY
```

预期六行索引按 `0、7、14、21、28、35` 输出，最后一行为 `index=35: pose=0.01, twist=0.001`。这段只检查协方差字段；对象的其他字段保留默认值，不发布为完整里程计观测。

**③ 回答赋值与融合之间还缺什么。**

试想把最后一项误写成相同值：程序仍可能接受两组 36 项数组，但它已不符合本页 PPT。反过来，即使数值完全一致，也尚未验证它们是否符合实际噪声、长期漂移或下游配置。

**验收：**提交两份矩阵、两行标准差和六个消息索引记录；说明 pose[35] 与 twist[35] 的不同单位，指出哪些数据来源还需核对真实驱动代码，并解释为何麦轮不能直接采用差速底盘的 vy = 0 约束。

**排查与结束：**协方差赋值失败时检查长度是否误为 6、赋值路径是否混入内层 pose/twist；`nav_msgs` 无法导入时检查已 source Humble、依赖已安装且使用系统 Python。角度换算保留 `/s`，不要把角速度标准差写成角度。两段脚本均自动退出，不生成文件或后台进程，不发送实机命令。

### 课堂提问

> 本页左右两栏的 x 方差都为 `1e-3`，是否能互相替代？

参考答案：不能。左边描述位置误差，右边描述线速度误差；单位不同，数据来源与时间行为也不同。

> 航向和角速度都来自 IMU，是否就应设置相同方差？

参考答案：不是。它们是不同物理量，可能经历不同处理链；应分别按误差模型与验证结果设计，不能由共用设备直接推出数值。

> 矩阵赋值全部符合 PPT，能否直接宣布底盘里程计精度达标？

参考答案：不能。这只证明消息声明与课件一致，还需核对真实实现、下游融合选择，并通过适当的参考与实测评估准确性。

这页最后可以总结：

> **先按变量、单位和索引写对两份矩阵，再核对数据链与实测依据；课件中的设计值是待验证的不确定性声明，不是精度保证。**
