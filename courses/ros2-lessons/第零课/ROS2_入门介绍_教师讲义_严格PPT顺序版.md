# ROS 2 入门介绍：逐页教师讲义

对应课件：`2026.7.28 ROS2入门介绍.pptx.pptx`，共 23 页。

环境：WSL Ubuntu 22.04、ROS 2 Humble、Bash、Python 3。下面的 Linux 命令在 **WSL Ubuntu 终端**执行，不在 Windows PowerShell 中执行。

前置知识：能打开终端，了解文件夹与程序的区别。课堂按原 PPT 的第 1 至第 23 页推进，补充操作放在对应页，不增加课件页码。

本课贯穿示例：用官方 `demo_nodes_cpp` 的 talker 和 listener 认识通信，再创建独立的 `.`，构建自己的 Python 发布节点。这样既能观察现成系统，又能走完一次开发流程。

## 上课前准备

### 1. 环境与终端

先进入讲义根目录，再创建旁边的独立工作空间；之后本课的文件命令均从 `ros2_intro_ws` 根目录执行，新终端也先进入同一目录。

```bash
mkdir -p ../ros2_intro_ws/src ../ros2_intro_ws/tf_debug
cd ../ros2_intro_ws
```


准备三个终端：A 运行发布者，B 运行订阅者，C 查询和调试。长期运行的命令占用当前终端，按 `Ctrl+C` 结束后才能执行下一条命令；同一代码块中标注 A/B/C 的命令需要分别执行。

每个新终端先执行：

```bash
source /opt/ros/humble/setup.bash
echo "$ROS_DISTRO"
echo "${ROS_DOMAIN_ID:-0}"
command -v ros2
python3 --version
```

预期发行版为 `humble`，三个终端的 Domain ID 相同，`ros2` 路径存在。为空或提示文件不存在时，先确认 Humble 已安装，再继续。本讲义以已有 Humble 为前提，不把安装 ROS 2 的全部流程混入本课。

课前检查：

```bash
ros2 pkg prefix demo_nodes_cpp
ros2 pkg executables demo_nodes_cpp
ros2 pkg prefix rqt_graph
colcon --help
rosdep --version
```

应能找到演示包，并在可执行文件列表中看到 `talker`、`listener`。缺少依赖时，教师在 WSL 中按需安装：

```bash
sudo apt update
sudo apt install ros-humble-demo-nodes-cpp ros-humble-rqt-graph \
  python3-colcon-common-extensions python3-rosdep \
  ros-humble-example-interfaces ros-humble-geometry-msgs ros-humble-nav-msgs \
  ros-humble-sensor-msgs ros-humble-ros2doctor
```

`rqt_graph` 需要可用的图形显示环境；WSL 中先确认 WSLg 或自己的显示配置正常。图形工具不可用时，仍可完成本课全部 CLI 演示。

### 2. 本课统一名称

| 对象 | 名称或路径 | 用途 |
| --- | --- | --- |
| 官方包 | `demo_nodes_cpp` | 运行 talker、listener |
| 官方节点 | `/talker`、`/listener` | 观察节点关系 |
| 官方话题 | `/chatter` | 观察字符串消息 |
| 工作空间 | `.` | 第零课专用，避免改动前课空间 |
| 自建包 | `intro_demo` | Python 节点工程 |
| 自建节点、入口 | `/intro_talker`、`intro_talker` | 第 18 页构建，第 20 页启动 |
| 自建话题 | `/intro/chatter` | 每秒发布一条字符串 |
| 消息 | `std_msgs/msg/String` | 字段 `data` 保存字符串 |

本课第 18 页包含完整源码与配置。教师预演时可以提前准备，课堂仍到第 18 页才讲构建。所有写文件命令面向新建的第零课目录；如果已做过练习，先查看同名文件再重新写入。

---



使用 rosdep 前，先初始化并更新规则；已初始化时跳过 init。

```bash
if [ ! -f /etc/ros/rosdep/sources.list.d/20-default.list ]; then
  sudo rosdep init
fi
rosdep update --rosdistro humble
```

## 第 1 页｜ROS2 核心理论概论学习

这页你可以把它讲成：

> **一台机器人不是一个大程序。摄像头要采集，算法要识别，底盘要执行，调试工具还要观察。今天我们先学会把这些程序组织起来，并看清它们怎样交换数据。**

用校园配送机器人开场：摄像头发现行人，避障模块计算速度，底盘执行速度。学生不必现在理解导航算法，但要能说出“采集、处理、执行”三个职责。

### 本课的学习目标

学完要能回答：代码放在哪里？运行起来的模块叫什么？数据经什么通道传递？改完代码怎样构建、加载和验证？

板书：

```text
工作空间 → 功能包 → 节点
                       ↓
                 接口与通信
                       ↓
                构建、加载、验证
```

### 开场操作

在终端 C 执行 `ros2 --help`。引导学生找到 `node`、`topic`、`pkg`、`run` 等入口。这里先认识工具目录，不要求记住全部子命令。

课堂提问：机器人换一台摄像头，应该重写所有算法吗？

参考答案：如果驱动仍提供约定的数据接口，算法可以继续使用；但分辨率、坐标系和时间戳等约定仍要核对。

这页最后可以总结：

> **今天先建立工程结构和通信视角，再用命令验证每一个概念。**

---

## 第 2 页｜初始 ROS2 概览

> **ROS 名字里有“操作系统”，但它主要是一套机器人软件框架和工具生态。我们仍需要 Ubuntu 这样的操作系统，它负责进程、文件和硬件资源，ROS 2 负责组织机器人应用。**

### 起源与升级动机

沿 PPT 的顺序讲：早期研究需要把不同实验室的软件接起来，后来形成可复用的包和工具生态。ROS 2 延续这种复用方式，同时面向多机器人、跨平台和更复杂的部署需求重新设计底层通信。

不要让学生把发展史背成年份表。用“摄像头驱动不用每个团队从零写”解释生态价值，用“多台机器人能在统一接口上协作”解释架构价值。

### 操作：区分系统与框架

终端 C：

```bash
cat /etc/os-release
uname -m
echo "$ROS_DISTRO"
ros2 pkg prefix rclpy
```

预期第一条给出 Ubuntu 版本，第二条给出 CPU 架构，第三条为 Humble，最后一条找到 Python 客户端库所在安装前缀。这四个结果分别回答“系统、硬件、ROS 发行版、开发库在哪里”。

课堂提问：装上 ROS 2 以后，还需要操作系统调度进程吗？

参考答案：需要。ROS 2 没有替代操作系统内核。

教师说明：课件的实时、安全和平台能力是框架提供的支持条件，不能理解成任何节点都自动满足硬实时、自动加密或在任意平台无修改运行。

> **ROS 2 的价值在于统一机器人软件协作方式，而不是替代底层操作系统。**

---

## 第 3 页｜为什么使用 ROS & 开发 ROS2

> **如果驱动、算法、可视化都用各自的数据格式，接一次设备就要改一大片代码。ROS 把常见通信和工具统一起来，让团队把时间放在机器人业务上。**

### 模块化、生态与 DDS

先解释模块化：摄像头驱动只负责可靠采集，识别算法只负责消费图像并输出结果。再解释生态：成熟的消息、工具和驱动减少重复实现。最后引入 DDS——数据分发服务，是 Humble 常见 RMW 实现背后的通信技术。

板书：`统一接口 → 模块替换 → 工具复用 → 系统集成`。

### 操作：找库、找接口、找工具

终端 C：

```bash
ros2 pkg prefix sensor_msgs
ros2 interface show sensor_msgs/msg/Image
ros2 pkg prefix rmw_fastrtps_cpp
printenv RMW_IMPLEMENTATION
```

观察 `Image` 中的尺寸、编码和数据字段，说明消息不仅装数据，还约定解释方式。`RMW_IMPLEMENTATION` 为空不等于没有中间件，可能使用默认实现；第三条只证明该包已安装，不能单独证明当前进程正在使用它。

课堂提问：两个节点都说“我发的是图像”，就一定互通吗？

参考答案：还要核对话题名、接口类型、QoS 和网络发现条件；业务上还要核对编码等字段。

教师说明：多线程不等于实时性保证，DDS Security 也需要明确配置。不要把课件中的性能优点讲成不带条件的性能排名。

> **复用能够成立，前提是模块对接口与运行条件有共同约定。**

---

## 第 4 页｜Master 节点：集群的“大脑”

> **这一页讨论的是一般分布式系统里的协调中心。放回 ROS 的语境，ROS 1 Master 更像“通讯录”，而不是替机器人规划任务的大脑；ROS 2 则不要求这个 Master 存在。**

### 区分课件中的不同系统

PPT 列举 K8s 控制平面、数据库主从、HDFS 元数据管理和 YARN 资源管理，它们的职责并不相同。Raft、法定人数和冗余部署是部分分布式系统的高可用设计，不能直接套成 ROS 1 Master 或 ROS 2 的部署步骤。

ROS 1 Master 帮助节点注册与查找通信对象；已经建立的数据连接通常直接在节点间传输。Master 失效会影响新注册和发现，不应讲成所有已有数据流必然立即中断。

### 操作：ROS 2 不启动 roscore

终端 A：

```bash
ros2 run demo_nodes_cpp talker
```

终端 B：

```bash
ros2 run demo_nodes_cpp listener
```

终端 C：

```bash
ros2 node list
ros2 topic list -t
```

预期 A 打印 `Publishing`，B 打印 `I heard`，C 看到 `/talker`、`/listener` 和 `/chatter`。没有运行 `roscore`，通信仍能建立。保留 A/B，后续继续观察。

课堂提问：`ros2 daemon` 是 ROS 2 Master 吗？

参考答案：不是，它为 CLI 缓存和查询计算图信息，节点通信不以它作为中央转发器。

教师说明：本页的“调度任务、维护集群可信状态、3 或 5 节点选主”不是 ROS 2 自动发现的职责。参考 [ROS 2 对 ROS 1 的架构变化](https://design.ros2.org/articles/changes.html)。

> **先辨清协调中心的具体职责，才能解释 ROS 2 为什么无需 ROS 1 Master。**

---

## 第 5 页｜ROS 的版本规则与演进

> **选择发行版，就像选择教材和实验器材的配套版本。不是版本名字越新越好，而是操作系统、依赖包和课程示例要能一起工作。**

### 发行版与包版本

Humble 是 ROS 2 发行版名称；某个包又有自己的版本号。Rolling 是持续更新的发行版，不适合作为本课需要固定复现结果时的默认环境。课件中的命名、发布节奏和支持周期用于介绍规律，具体支持日期以官方发行版页面和 REP 为准。

本课程统一使用 Ubuntu 22.04 与 Humble。后续查资料也优先选 Humble，避免直接照抄新版本的子命令或插件配置。

### 操作：确认实际环境

```bash
echo "$ROS_DISTRO"
ls /opt/ros
dpkg-query -W ros-humble-ros-base
ros2 doctor --report
```

如果装的是其他安装组合，第三条可能提示没有这个元包；这不代表所有 ROS 包都没安装，继续检查 `ros2 pkg prefix rclpy`。`doctor --report` 的网络或其他提示应按项目环境分析，不是每条警告都阻止课堂演示。

课堂提问：别人分享的 Jazzy 命令能否保证在 Humble 中直接运行？

参考答案：不能，必须核对接口和工具版本。命令的 `--help` 是本机最直接的入口。

教师说明：不把“每年固定发布”“所有版本支持五年”讲成涵盖历史与所有发行版的统一规则。教师备课查阅 [ROS 2 发行版资料](https://docs.ros.org/en/humble/Releases.html) 与 [REP-2000](https://www.ros.org/reps/rep-2000.html)。

> **先确认发行版，再选择对应文档和命令，是复现实验的第一步。**

---

## 第 6 页｜ROS2 核心概念体系总览

> **现在把几个容易混在一起的词分开：工作空间是项目目录，功能包是组织和分发单元，节点是运行中的功能角色，接口是交换数据的约定。**

用实验室类比：工作空间像实验室，功能包像一套工具箱，节点像正在完成工作的岗位，消息像规定格式的记录表。工具箱存在不代表岗位已经开始工作，因此“包已安装”和“节点已运行”要用不同命令检查。

板书：

```text
磁盘上：工作空间 / src / 功能包 / 源码
运行时：进程 → 一个或多个节点 → 通信对象
数据约定：msg / srv / action
```

### 操作：分别检查三个层次

```bash
ros2 pkg prefix demo_nodes_cpp
ros2 pkg executables demo_nodes_cpp
ros2 node list
ros2 interface show std_msgs/msg/String
```

预期安装前缀、可执行程序、运行节点和消息字段分别出现。先按 `Ctrl+C` 停止 A 中的 talker，再运行 `ros2 node list`，观察节点消失而包仍可查询，随后重启 talker。

课堂提问：一个功能包只能包含一个节点吗？

参考答案：可以包含多个节点、接口和配置。节点也不与进程一一对应，后续组件课程会见到一个进程中的多个节点。

教师说明：PPT 中“节点为进程”是入门简化说法，应在此改为“节点可运行在独立进程中，也可共享进程”。

> **磁盘上的组织、运行中的对象和数据接口，是三个不同层次。**

---

## 第 7 页｜ROS2 工作空间

> **工作空间不是 ROS 自动生成的一种神秘文件，而是我们约定好结构的项目目录。自己主要维护 src，构建工具负责生成 build、install 和 log。**

### 四个目录怎样分工

| 目录 | 内容 | 教师讲法 |
| --- | --- | --- |
| `src` | 包源码 | 在这里修改程序与配置 |
| `build` | 构建中间产物 | 可查构建过程，不当成源码编辑 |
| `install` | 安装产物及环境脚本 | ROS 运行时查找本项目的入口 |
| `log` | colcon 构建日志 | 定位构建错误 |

运行节点的 ROS 日志通常在 `~/.ros/log`，不应把工作空间 `log` 讲成所有节点日志的统一位置。

### 操作：建立本课目录

终端 C：

```bash
mkdir -p src
pwd
ls -la
colcon list
```

预期只有 `src` 等已有文件，空空间的 `colcon list` 没有包，这是正常现象。`build/install/log` 在构建之后才出现。

课堂提问：看到 `src` 目录就能执行 `source install/setup.bash` 吗？

参考答案：不能，必须先成功构建并生成安装目录。

教师说明：在工作空间根目录构建、源码放 `src` 是本课采用的规范。colcon 可以递归发现包并使用指定搜索路径，不能把课件“放在根目录一定无法识别”讲成工具硬限制。环境叠加也不是完全隔离的虚拟环境。

> **明确源文件和生成文件的边界，能避免许多构建与环境问题。**

---

## 第 8 页｜功能包（Package）

> **功能包把实现、依赖和安装规则放在一起。别人拿到一个包，不只拿到代码，还应知道需要什么依赖，以及运行入口在哪里。**

### 认识包类型与配置

本课选 `ament_python`，便于看清 Python 节点入口；`ament_cmake` 常用于 C++ 和接口生成。C++ 与 Python 的选择取决于业务和库，不是“C++ 必定实时、Python 只能做简单工作”。

`package.xml` 声明元信息和依赖；Python 包的 `setup.py` 注册安装信息与命令入口，`setup.cfg` 指定脚本安装位置。它们与程序源文件承担不同职责。

### 操作：创建包并检查文件

在 C 执行；若 `src/intro_demo` 已存在，跳过创建，先检查已有内容：

```bash
ros2 pkg create --destination-directory src --build-type ament_python --license Apache-2.0 \
  intro_demo --dependencies rclpy std_msgs
colcon list
find src/intro_demo -maxdepth 2 -type f | sort
cat src/intro_demo/package.xml
cat src/intro_demo/setup.cfg
```

预期 `colcon list` 找到 `intro_demo`，包中有 `package.xml`、`setup.py`、`setup.cfg`、`resource/intro_demo` 和 Python 模块目录。此时尚未编写发布者，不要尝试运行 `intro_talker`；第 18 页会补齐源码和入口。

课堂提问：只把 Python 文件复制到 src，ROS 就一定能用 `ros2 run` 启动它吗？

参考答案：还要有正确的包配置、安装入口、成功构建以及环境加载。

> **功能包让“代码能写”进一步变成“依赖明确、入口可找、工程可分发”。**

---

## 第 9 页｜节点（Node）：ROS2 执行单元

> **节点按业务职责划分。我们今天的 talker 负责产生字符串，listener 负责处理字符串；两个节点的关系从运行结果里可以查出来。**

### 操作：查看节点职责

确认 A/B 仍运行官方 talker/listener，在 C 执行：

```bash
ros2 node list
ros2 node info /talker
ros2 node info /listener
```

引导学生分别找到 Publishers 与 Subscribers。除 `/chatter`，还会见到日志和参数相关对象；节点不只包含一个话题。

### 操作：改变节点名

在 A 按 `Ctrl+C` 停止 talker，再运行：

```bash
ros2 run demo_nodes_cpp talker --ros-args -r __node:=talker_zero
```

C 执行 `ros2 node info /talker_zero`。预期节点名变了，B 仍能接收 `/chatter`。节点名称和话题名称是两个独立对象。演示后停止 A，用不带重命名参数的原命令恢复 `/talker`。

课堂提问：改了发布节点名称，订阅端必须改代码吗？

参考答案：如果话题名、类型和匹配条件没改变，通常不必。

教师说明：节点名建议唯一，重复名称可能导致查询和参数操作歧义，但 ROS 2 并非绝对拒绝同名节点。节点崩溃影响范围还取决于是否共享进程和业务依赖。

> **节点是业务角色，进程是操作系统容器，名称是查询与管理的标识。**

---

## 第 10 页｜节点（Node）：视觉感知系统拆分示例

> **沿着这页的三个岗位看数据：相机驱动产生图像，处理节点把图像变成检测结果，显示节点把结果呈现出来。语言可以不同，职责与接口必须清楚。**

### 板书与类比

```text
摄像头 → 驱动节点 → 图像话题 → 处理节点
                                  ↓
                              检测结果话题
                                  ↓
                                显示节点
```

用流水线理解：更换工位不必更换整条流水线，但交接物的规格要一致。本课没有相机设备和图像算法，因此用字符串模拟节点间交接，不声称它等价于真实视觉系统。

### 操作：增加观察端

保留 A/B 的 talker/listener，在 C 执行：

```bash
ros2 topic echo /chatter
```

预期 B 和 C 同时收到字符串。C 是额外的观察订阅者，证明在不修改 talker 代码的情况下可以接入调试模块。按 `Ctrl+C` 停止 C 中的 echo。

课堂提问：显示节点停止后，驱动必须停止采集吗？

参考答案：话题机制不要求停止；业务上是否继续采集由系统策略决定。

> **先按职责拆模块，再用统一的数据接口把它们接起来。**

---

## 第 11 页｜计算图（Computation Graph）

> **计算图是系统运行时的关系图。磁盘里有一个节点文件，并不代表图里有它；只有程序启动并创建通信对象，运行关系才出现。**

### 操作：文字查询与图形观察互相印证

在 C 执行：

```bash
ros2 node list
ros2 topic list -t
ros2 topic info /chatter
ros2 run rqt_graph rqt_graph
```

预期图中出现 talker、`/chatter`、listener 的发布订阅关系。若图形里看不到预期对象，先刷新，并检查隐藏节点、叶子话题等过滤选项，再与 CLI 输出对照。不同视图不一定显示所有服务和动作，不应据此说系统中没有它们。

### 课堂观察

让学生停止 B，再刷新图和运行 `ros2 topic info /chatter`。没有其他观察订阅者时，订阅数量应下降；重新启动 B 后恢复。发现与缓存更新可能有短暂延迟。

课堂提问：图里有发布订阅连线，能否证明消息内容正确？

参考答案：不能，还要 echo 查看内容，必要时测频率和检查字段语义。

> **计算图负责告诉我们谁连接谁，数据检查负责告诉我们实际传了什么。**

---

## 第 12 页｜通信接口总述与 IDL

> **通信方式回答“怎样交互”，接口回答“传什么”。同样都是两个节点，持续广播温度、查询一次状态、执行一段导航任务，适合的交互方式并不一样。**

| 对象 | 交互方式 | 例子 |
| --- | --- | --- |
| Topic | 持续发布与订阅 | 相机图像、温度流 |
| Service | 一次请求与响应 | 查询、重置、短时计算 |
| Action | 目标、反馈、结果及取消机制 | 导航、长时间运动任务 |
| Parameter | 节点配置读写 | 阈值、频率、模式 |

IDL 是接口定义语言。ROS 的 `.msg/.srv/.action` 描述经过生成工具形成可供不同语言使用的代码；初学时先看字段与结构，不要求现在手写底层 IDL。

### 操作：比较接口结构

```bash
ros2 interface show std_msgs/msg/String
ros2 interface show example_interfaces/srv/AddTwoInts
ros2 interface show example_interfaces/action/Fibonacci
```

预期 String 只有 `data`；服务用一条 `---` 分隔请求与响应；动作用两条 `---` 分隔目标、结果和反馈。示例接口来自官方演示依赖，若包缺失可安装 `ros-humble-example-interfaces`。

课堂提问：服务一定只能阻塞调用吗？

参考答案：不是，客户端可以异步调用。服务的核心是请求响应，阻塞与否是调用方式。动作提供长任务交互协议，也不自动保证任意业务逻辑都安全抢占。

教师说明：本页参数作为第四种常用交互对象介绍，不把它再称为一种 `.param` 接口文件。

> **先按业务选择交互方式，再用接口定义约束数据结构。**

---

## 第 13 页｜话题通信（Topic）- Part 1

> **话题就像约定了内容格式的广播频道。发布者不必点名某个接收者，订阅者按频道名和类型接入，但能否通信还取决于配置是否兼容。**

### 操作：从名字查到实际数据

保留 A/B 的 talker/listener，在 C 依次执行：

```bash
ros2 topic list -t
ros2 topic type /chatter
ros2 topic info /chatter --verbose
ros2 topic echo /chatter --once
```

预期类型为 `std_msgs/msg/String`，echo 输出 `data` 字段中的问候文本。`--once` 收到一条后退出，适合课堂检查；若没有任何消息，它仍会等待，可按 `Ctrl+C`。

继续运行：

```bash
ros2 topic hz /chatter
```

等待多个采样周期，观察频率后按 `Ctrl+C`。官方 talker 通常约每秒一条，实际测量会受调度、网络和观察订阅者影响，不要求恰好等于整数。

板书：`创建端点 → 发现与匹配 → 发布数据 → 执行订阅回调`。

课堂提问：列出话题就等于收到消息了吗？

参考答案：不等于，端点存在但未发布、QoS 不兼容或处理阻塞都可能导致无数据。

教师说明：QoS 是兼容性匹配，不是双方自动协商成相同配置。`publish()` 也不应概括为任何实现、任何资源条件下都绝不阻塞。

> **检查话题时，要把名称、类型、端点、内容和频率连起来看。**

---

## 第 14 页｜话题通信（Topic）- Part 1：一对多模型

> **沿着这页的模型图看，一个发布端把数据交给同一话题，多个订阅端分别接收。这不是谁先抢到谁独占的工作队列。**

### 操作：验证一份数据被多个节点消费

A 保持 talker。B 停止原 listener，启动第一个接收者：

```bash
ros2 run demo_nodes_cpp listener --ros-args -r __node:=listener_one
```

C 启动第二个接收者：

```bash
ros2 run demo_nodes_cpp listener --ros-args -r __node:=listener_two
```

观察两个终端都打印收到的问候信息。为查询数量，C 停止 listener_two 后先执行 `ros2 topic info /chatter`，然后重新启动它；教师也可以课前准备第四个观察终端，在两个 listener 同时运行时查询，预期至少两个订阅端。

课堂提问：新增订阅者后，talker 必须知道它的节点名吗？

参考答案：不必按节点名寻址。通信依赖话题名、接口类型、域及 QoS 等条件。

教师说明：一个发布者面对多个订阅者，仍可能承担额外传输和缓存开销；“不用修改代码”不等于“增加订阅者完全没有成本”。演示后停止两个重命名 listener，在 B 恢复普通 listener，C 留作调试。

> **话题支持数据分发，多个模块可以消费同一数据流。**

---

## 第 15 页｜话题通信（Topic）- Part 2

> **同一种数据可以服务多个模块，但我们必须先把名字管理好。多机器人最容易出现的问题，是两台机器人的同名话题混在一起。**

### 名称、命名空间与重映射

相对话题名会受节点命名空间影响，绝对话题名以 `/` 开头。重映射可以在启动时改变名称，避免为换部署路径直接修改源代码。名称区分大小写，课堂统一小写下划线。

### 操作：同一套节点搬到另一个命名空间

先停止 A/B 的官方演示节点。A：

```bash
ros2 run demo_nodes_cpp talker --ros-args -r __ns:=/robot0
```

B：

```bash
ros2 run demo_nodes_cpp listener --ros-args -r __ns:=/robot0
```

C：

```bash
ros2 node list
ros2 topic list -t
ros2 topic echo /robot0/chatter --once
```

预期节点和话题带 `/robot0` 前缀，B 正常接收。随后停止 A/B，改用下面的命令演示话题重映射：

```bash
# A
ros2 run demo_nodes_cpp talker --ros-args -r chatter:=/intro/chatter
# B
ros2 run demo_nodes_cpp listener --ros-args -r chatter:=/intro/chatter
```

课堂提问：只把发布端改成 `/intro/chatter`，订阅端仍监听 `/chatter`，会怎样？

参考答案：它们成为不同话题，不再形成这条数据关系。演示后停止 A/B，避免与后面自建节点同时发布同名话题。

> **命名空间管理归属，重映射管理连接，二者都服务于部署复用。**

---

## 第 16 页｜QoS 服务质量

> **同一话题存在不代表双方一定匹配。QoS 是通信规则：发送端能提供什么，接收端要求什么，要求超过提供能力时就可能没有数据。**

### 先认识可靠性

BEST_EFFORT 是尽力传输，RELIABLE 提供确认与重传机制。不能把可靠传输解释成业务任务必成功或安全急停必执行，也不能把尽力传输解释成任意场景下延迟必然更低。

### 操作：制造并修复可靠性不兼容

当前 A/B 没有其他发布者。A 启动低频模拟传感器：

```bash
ros2 topic pub /intro/qos std_msgs/msg/String "{data: sensor}" \
  --rate 2 --qos-reliability best_effort
```

B 先执行：

```bash
ros2 topic echo /intro/qos --qos-reliability reliable
```

预期没有收到数据，可能出现不兼容警告。C 执行：

```bash
ros2 topic info /intro/qos --verbose
```

找到发布端 BEST_EFFORT 与订阅端 RELIABLE。B 按 `Ctrl+C` 后改为：

```bash
ros2 topic echo /intro/qos --qos-reliability best_effort --once
```

预期看到 `data: sensor`。结束 A 的发布。

课堂提问：可靠发布者与尽力订阅者能匹配吗？

参考答案：可靠性这一项可以，反过来则不兼容；完整匹配还要考虑其他策略。

> **QoS 排查从端点配置对照开始，不能只看话题名是否相同。**

---

## 第 17 页｜消息：通信载体

> **消息类型是一张双方共同遵守的表格。String 只有一格文字，而 Twist 把线速度和角速度分开；选类型时，我们要让字段能够表达真实含义。**

### 标准包与自定义消息

PPT 中几何和传感器包的准确名称是 `geometry_msgs` 与 `sensor_msgs`；导航数据包是 `nav_msgs`。`std_msgs` 适合展示基础结构，工程中优先使用语义明确的标准接口，或定义合适的专用消息。

### 操作：查看结构并按结构填写

```bash
ros2 interface show std_msgs/msg/String
ros2 interface show geometry_msgs/msg/Twist
ros2 interface show sensor_msgs/msg/Imu
ros2 interface show nav_msgs/msg/Odometry
```

观察 Twist 的 `linear` 与 `angular` 都包含三轴分量。本页仅发布到专用练习话题，避免混入底盘控制通道：

```bash
# A：持续发布，保证观察端能接入
ros2 topic pub /intro/twist geometry_msgs/msg/Twist \
  "{linear: {x: 0.2}, angular: {z: 0.3}}" --rate 1
# B
ros2 topic echo /intro/twist --once
```

预期 `linear.x` 为 0.2、`angular.z` 为 0.3，其余未填字段采用默认值。结束 A。

### 教师说明：EnvData.msg

PPT 的 `EnvData.msg` 是自定义结构示例：Header 提供时间戳和坐标系字段，温度字段要约定单位，有效性字段表达业务状态。本课先读懂结构，不提前把第一课的接口生成工程插入此处。

后续实现时需要 `ament_cmake` 接口包、`rosidl_default_generators`、运行导出和接口包组声明，并把 `std_msgs` 依赖同时加入消息生成配置与 `package.xml`；不是随便放一个 `.msg` 就自动可用。

课堂提问：温度用 String 发 `"25"` 可以吗？

参考答案：可以传字符串，但单位、类型与语义都不够清楚，应按应用选结构化接口。

> **接口不仅规定字段类型，也需要我们约定单位、坐标系和业务含义。**

---

## 第 18 页｜colcon 构建系统与编译规范

> **现在从“运行别人写好的节点”转到“运行我们自己的节点”。colcon 负责组织包的构建；具体如何编译或安装，仍由 CMake、setuptools 等工具完成。**

### 1. 写完整发布节点

执行前提：已完成第 7、8 页，存在 `src/intro_demo`。终端 C：

```bash
cat > src/intro_demo/intro_demo/intro_talker.py <<'PY'
import rclpy
from rclpy.node import Node
from std_msgs.msg import String


class IntroTalker(Node):
    def __init__(self):
        super().__init__('intro_talker')
        self.publisher = self.create_publisher(String, '/intro/chatter', 10)
        self.count = 0
        self.timer = self.create_timer(1.0, self.publish_message)

    def publish_message(self):
        message = String()
        message.data = f'第零课消息 {self.count}'
        self.publisher.publish(message)
        self.get_logger().info(message.data)
        self.count += 1


def main(args=None):
    rclpy.init(args=args)
    node = IntroTalker()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == '__main__':
    main()
PY
```

讲解职责：`Node` 管理通信对象；Publisher 绑定类型和话题；Timer 每秒产生一次事件；`spin` 驱动回调；退出时释放节点与运行上下文。`10` 指定 KEEP_LAST 队列深度，不代表频率为 10 Hz。

### 2. 注册可执行入口

以下是本课新建包的完整 `setup.py`，不是对已有复杂包的通用覆盖方式：

```bash
cat > src/intro_demo/setup.py <<'PY'
from setuptools import find_packages, setup

package_name = 'intro_demo'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
         ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='ROS2 Teacher',
    maintainer_email='teacher@example.com',
    description='ROS 2 lesson zero publisher demonstration',
    license='Apache-2.0',
    entry_points={
        'console_scripts': [
            'intro_talker = intro_demo.intro_talker:main',
        ],
    },
)
PY
```

入口的三个部分分别是“可执行名称 = Python 模块:函数”。`setup.cfg` 保留第 8 页创建工具生成的配置，脚本应安装到 `$base/lib/intro_demo`。

### 3. 检查依赖并构建

```bash
python3 -m py_compile src/intro_demo/intro_demo/intro_talker.py
python3 -m py_compile src/intro_demo/setup.py
colcon list
rosdep check --from-paths src --ignore-src --rosdistro humble
colcon build --packages-select intro_demo --symlink-install
ls
test -f install/setup.bash && echo "安装环境脚本已生成"
```

预期语法检查不输出错误，`colcon list` 找到包，构建摘要显示 `intro_demo` 成功，生成 `build/install/log`。`py_compile` 只检查 Python 语法，不证明 ROS 运行正确。

如果 rosdep 提示数据库未初始化，教师先运行一次 `sudo rosdep init`，再以普通用户执行 `rosdep update`；如果已经初始化，不重复 init。缺少依赖时在工作空间根目录执行：

```bash
rosdep install --from-paths src --ignore-src --rosdistro humble -y
```

教师应解释它会安装声明的系统依赖，不是替我们生成源码。

课堂提问：构建成功，为什么新终端仍可能找不到包？

参考答案：新终端尚未加载工作空间环境；第 20 页完成加载和运行闭环。

> **源码、入口、依赖和构建规则齐全，colcon 才能把工程变成可运行产物。**

---

## 第 19 页｜核心编译命令

> **命令参数解决的是具体问题：选哪些包、怎样安装、怎样显示日志和怎样限制资源。记住问题，再记参数，比背一张命令表更有效。**

### 操作：针对本课包构建

在 C 的工作空间根目录执行：

```bash
colcon build --packages-select intro_demo --symlink-install \
  --event-handlers console_direct+
```

预期只针对 `intro_demo` 构建，日志直接显示。首次使用依赖于空间内其他源码包的目标时，应考虑 `--packages-up-to`：

```bash
colcon build --packages-up-to intro_demo --symlink-install \
  --parallel-workers 2
```

本课只有一个包，两种选择结果相近；目标带多个未安装的本地依赖时，区别才明显。

| 参数 | 用途 | 条件与边界 |
| --- | --- | --- |
| `--packages-select` | 选指定包 | 不自动补构建所有源码依赖 |
| `--packages-up-to` | 选目标及其依赖 | 适合构建依赖链 |
| `--symlink-install` | 能链接的文件使用符号链接 | 入口、依赖、接口或安装结构改变仍要重建 |
| `--parallel-workers` | 控制同时执行的包任务数 | 不等于每个 C++ 编译器只开相同数量线程 |
| `--cmake-clean-cache` | 清除 CMake 缓存再配置 | 针对 CMake 包，本课 Python 包无需使用 |

课堂提问：增量构建是否保证所有未改包都完全跳过？

参考答案：不是，构建任务可能仍被调用，底层工具再判断需要更新的产物。

教师说明：`colcon clean` 来自额外扩展，不是所有环境都有的核心命令。本课不让学生用未核对的 `colcon clean -a`，也不通过删除整个工作空间来修复错误。参考 [colcon build 参数](https://colcon.readthedocs.io/en/released/reference/verb/build.html)。

> **按修改范围选包，按故障类型选参数，构建命令才有明确目的。**

---

## 第 20 页｜环境变量生效

> **构建完成只说明产物已经放在磁盘上。source 是把查找路径告诉当前终端，让 ROS 知道这个新包在哪里。**

### 操作：完成运行闭环

A 是一个新终端，执行：

```bash
source /opt/ros/humble/setup.bash
source install/setup.bash
ros2 pkg prefix intro_demo
ros2 pkg executables intro_demo
ros2 run intro_demo intro_talker
```

预期包前缀指向本课 install，入口列表有 `intro_talker`，随后每秒打印递增编号。

B 执行：

```bash
source /opt/ros/humble/setup.bash
source install/setup.bash
ros2 topic echo /intro/chatter
```

C 执行：

```bash
source /opt/ros/humble/setup.bash
source install/setup.bash
ros2 node info /intro_talker
ros2 topic info /intro/chatter --verbose
printenv AMENT_PREFIX_PATH
```

预期 B 收到与 A 编号一致的消息，C 能查到发布节点与端点。查看 `AMENT_PREFIX_PATH`，解释 overlay 位于已有 ROS 安装之上。

### source 的范围

它改变当前 shell 的环境，随后从该 shell 启动的进程继承这些设置；不会自动修改其他已经打开的终端。`setup.bash` 可加载关联的底层环境，`local_setup.bash` 主要加载当前前缀，使用后者时需要先准备 underlay。

课堂提问：源码改了，但运行中节点的文本没有变，是不是 source 没成功？

参考答案：不一定。运行进程通常已经导入旧模块，需要停止并重启；安装方式和修改类型也决定是否要重建。

教师说明：同名包 overlay 可能覆盖查找结果，但不保证所有混合依赖都安全兼容。课堂采用临时 source，保持终端环境可解释。

> **构建生成产物，加载配置路径，启动验证行为，这三步各有作用。**

---

## 第 21 页｜编译规范与最佳实践

> **遇到构建失败，先找到最早的有效错误，再判断它属于源码、依赖、安装入口还是环境。不要让后面的连锁报错把我们带偏。**

### 操作：建立固定排查顺序

在 C 执行：

```bash
pwd
colcon list
python3 -m py_compile src/intro_demo/intro_demo/intro_talker.py
cat src/intro_demo/package.xml
cat src/intro_demo/setup.cfg
ls log/latest_build/intro_demo
```

成功完成前面构建后，可以打开最近日志：

```bash
cat log/latest_build/intro_demo/stderr.log
cat log/latest_build/intro_demo/stdout_stderr.log
```

不同构建工具的日志内容可能不同，空的 stderr 不等于已经验证了运行行为。

| 现象 | 首先检查 | 检查方法与下一步 |
| --- | --- | --- |
| colcon 看不到包 | 当前目录、package.xml | `pwd`、`colcon list`；返回工作空间根目录 |
| Python 语法报错 | 首个 SyntaxError 的文件行号 | `py_compile`；修复缩进或符号后重建 |
| Package not found | 是否加载 install | `ros2 pkg prefix intro_demo`；当前终端重新 source |
| No executable found | 入口和安装位置 | 查 setup.py、setup.cfg；重建后再查 executables |
| 模块导入失败 | 模块路径与依赖 | 核对入口拼写、package.xml，检查 rclpy 与 std_msgs |
| 老版本程序仍运行 | 原进程、安装路径 | 停止旧进程，确认包前缀后重新启动 |

### 小练习

任务：把第 18 页消息前缀改成 `课堂练习`。操作：编辑源码，按本课构建流程重建并加载，在 A 重启节点，B 查看消息。验收：话题名与类型不变，文本变了，编号仍递增。

提问：为什么不直接改 install 中的文件？

参考答案：源码才是维护对象；安装目录的修改可能被构建覆盖，或因符号链接影响源码，容易失去修改边界。

教师说明：`--symlink-install` 对符合条件的 Python 源文件可减少重建需要，但修改入口、依赖或安装资源时仍要重建。简单统一重建流程适合入门教学。

> **排查先定位环节，再修复具体条件，不靠反复清空目录碰运气。**

---

## 第 22 页｜节点发现机制（Discovery）

> **节点首先要找到彼此，之后端点才能匹配。Domain ID 把实验分在不同通信域，像给不同课堂分房间；房间不同，即使话题同名也看不到彼此。**

### 操作：验证域隔离

先停止 A 的自建节点和 B 的 echo。本演示所有终端只使用同一台 WSL 主机。

A：

```bash
export ROS_DOMAIN_ID=20
ros2 run demo_nodes_cpp talker
```

B：

```bash
export ROS_DOMAIN_ID=21
ros2 run demo_nodes_cpp listener
```

预期 B 收不到 A 的消息。B 停止 listener，改域后重启：

```bash
export ROS_DOMAIN_ID=20
ros2 run demo_nodes_cpp listener
```

预期开始收到消息。必须在新进程启动前设置环境变量，改变 shell 变量不会改掉已运行进程的域。

### 操作：查询与恢复

C：

```bash
export ROS_DOMAIN_ID=20
ros2 daemon stop
ros2 node list
ros2 topic list -t
echo "$ROS_DOMAIN_ID"
```

停止 daemon 是让 CLI 在当前域重新建立查询状态，不是重启通信中心。`node list` 查询可能会自动重新启动 daemon。

演示结束，A/B 按 `Ctrl+C` 停止节点，三个终端分别 `unset ROS_DOMAIN_ID`，C 再执行 `ros2 daemon stop`。查询和新进程回到默认域 0。

课堂提问：两台电脑 ping 得通，ROS 2 就一定互相发现吗？

参考答案：不一定，还要检查域、发现配置、组播与网络、防火墙及本机限制等条件。不要把关防火墙作为通用第一步。

教师说明：Domain ID 是通信域隔离机制，不是身份认证和安全隔离。ROS 1 Master 失效也不等于所有已建立连接立刻终止。参考 [ROS_DOMAIN_ID 说明](https://docs.ros.org/en/humble/Concepts/Intermediate/About-Domain-ID.html)。

> **发现问题先查进程启动环境，再查网络和端点匹配。**

---

## 第 23 页｜QoS 服务质量

> **最后把通信检查收成一条完整流程：先确认能发现，再确认名字和类型，再对照 QoS，最后观察数据。历史数据能否交给晚来的订阅者，也是 QoS 要解决的问题。**

### 1. 历史与持久性

KEEP_LAST 的 depth 控制保留样本数量；TRANSIENT_LOCAL 支持存活发布端向符合条件的晚加入订阅者提供缓存数据。它不是保存到磁盘的数据库，发布者退出后不能依赖该缓存继续存在。

VOLATILE 不要求向新订阅者补发连接前的数据，不代表运行中完全没有队列。发布端和订阅端的 depth 不必相同，也不是所有 QoS 策略都要求相等。

### 2. 操作：只发一次，再晚加入接收端

A 发布一次并继续保持进程存活；这里 `--times 1` 配合 `--keep-alive`，便于观察晚加入行为：

```bash
ros2 topic pub --help
ros2 topic pub /intro/config std_msgs/msg/String "{data: mode_A}" \
  --qos-reliability reliable --qos-durability transient_local \
  --qos-depth 1 --times 1 --wait-matching-subscriptions 0 --keep-alive 60
```

`--wait-matching-subscriptions 0` 明确让 A 不等待订阅者就发布；否则 Humble 在 `--times` 模式下默认等待至少一个匹配订阅者，实验就不再是“先发布、后接入”。等 A 明确打印第一条消息后，B 在 60 秒存活窗口内启动：

```bash
ros2 topic echo /intro/config --qos-reliability reliable \
  --qos-durability transient_local --once
```

预期 B 仍收到 `mode_A`，说明不是必须等下一次发布。若发行版补丁的 CLI 行为不同，先以本机 `--help` 核对；关键实验条件是“只发布一次、发布进程存活、双方使用兼容持久性”。

C：

```bash
ros2 topic info /intro/config --verbose
```

检查发布端持久性与深度。A 退出后，不保证再次接入还能收到旧样本。先前的 `--once` 往往伴随进程退出，不能拿已经退出的发布者来验证晚加入缓存。

### 3. 预设与重要修正

| Profile | 课堂应讲清的含义 |
| --- | --- |
| 默认话题 | 常见为 KEEP_LAST、深度 10、RELIABLE、VOLATILE |
| 传感器数据 | 常见为 BEST_EFFORT、较小深度、VOLATILE，强调新鲜数据 |
| 参数服务 | RELIABLE、VOLATILE，队列较大；不是自动保存参数或重启恢复 |

教师说明：PPT 将 Parameters 预设描述为“本地持久化、重启同步”不准确。不要据此让学生期待节点重启后自动恢复参数。可靠传输也不能代替任务应答、超时处理和安全设计。参考 [Humble QoS 概念与兼容规则](https://docs.ros.org/en/humble/Concepts/Intermediate/About-Quality-of-Service-Settings.html) 以及 [RMW 预设源码](https://github.com/ros2/rmw/blob/humble/rmw/include/rmw/qos_profiles.h)。

### 4. 本课验收练习（放在末页，不另造页码）

1. **工程闭环**：从第 8、18、20 页完成包创建、构建、source 和运行。验收：`ros2 pkg executables intro_demo` 找到入口，echo 看到递增字符串。
2. **改名部署**：用第 15 页的方法把官方 talker/listener 搬到 `/robot0`。验收：节点和 `/robot0/chatter` 均出现，订阅者收到数据，并能解释节点名与话题名的区别。
3. **通信排查**：重做第 16 页 BEST_EFFORT 发布、RELIABLE 订阅实验。验收：用 verbose 指出不兼容的一项，改订阅配置后恢复数据。
4. **域隔离**：重做第 22 页。验收：不同域收不到，同域收到，能说明环境变量为何必须在进程启动前设置。

清理：结束所有持续运行的课堂命令；各终端恢复 Domain ID，保留 `src` 与构建产物以供后续复习。

课堂提问：看到 `/intro/config` 但 echo 没有数据，应该立刻改发布代码吗？

参考答案：先查发布者是否存活、是否发过样本、持久性是否兼容，以及域和类型；定位到具体环节后再修改。

> **本课的终点不是背命令，而是用“工程结构—构建加载—发现匹配—数据验证”解释一次完整运行。**

---

## 教师核对资料

使用 rosdep 前，先初始化并更新规则；已初始化时跳过 init。


- [Humble 命令行环境配置](https://docs.ros.org/en/humble/Tutorials/Beginner-CLI-Tools/Configuring-ROS2-Environment.html)：每个终端加载环境、发行版变量。
- [Humble 节点教程](https://docs.ros.org/en/humble/Tutorials/Beginner-CLI-Tools/Understanding-ROS2-Nodes/Understanding-ROS2-Nodes.html)：节点、重命名与查询。
- [Humble 话题教程](https://docs.ros.org/en/humble/Tutorials/Beginner-CLI-Tools/Understanding-ROS2-Topics/Understanding-ROS2-Topics.html)：计算图、类型、发布、频率。
- [Humble Python 发布订阅教程](https://docs.ros.org/en/humble/Tutorials/Beginner-Client-Libraries/Writing-A-Simple-Py-Publisher-And-Subscriber.html)：rclpy、依赖与 console_scripts。
- [Humble 工作空间教程](https://docs.ros.org/en/humble/Tutorials/Beginner-Client-Libraries/Creating-A-Workspace/Creating-A-Workspace.html)：underlay、overlay 与构建。
- [ROS 2 设计变化](https://design.ros2.org/articles/changes.html)：Master、节点与进程、多语言等架构区别。
- [Humble QoS 概念](https://docs.ros.org/en/humble/Concepts/Intermediate/About-Quality-of-Service-Settings.html)：可靠性、持久性、兼容规则。
- [Humble topic pub 源码](https://github.com/ros2/ros2cli/blob/humble/ros2topic/ros2topic/verb/pub.py)：times、keep-alive、QoS 参数。
- [Humble RMW QoS 预设源码](https://github.com/ros2/rmw/blob/humble/rmw/include/rmw/qos_profiles.h)：默认、传感器与参数 Profile。
- [colcon 构建命令参考](https://colcon.readthedocs.io/en/released/reference/verb/build.html)：构建与日志参数。
- [REP-2000](https://www.ros.org/reps/rep-2000.html)：发行版目标平台与支持周期。

验证说明：已读取目标 PPT 的全部 23 页文字并逐页对应；已对讲义中的 Python 完整代码执行语法检查，对页码、代码围栏及关键名称进行静态检查。ROS 2 命令参数与重要修正对照 Humble 官方教程及源码核对。当前编写环境是 Windows，未在目标 WSL/Humble 中启动节点或完成通信实验；实际频率、发现、QoS 缓存与图形显示需教师按上述流程预演，不能将静态检查当成真实运行测试。
