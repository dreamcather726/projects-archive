# ROS 2 话题通信进阶与消息类型：逐页教师讲义

对应课件：`2026.7.29 ROS2 话题通信进阶与消息类型学习手册.pptx`，共 34 页。

环境：WSL Ubuntu 22.04、ROS 2 Humble。

这份讲义严格按照 PPT 页码展开。每页都写成可以直接在课堂上讲的口语稿，并配有类比、板书、提问、命令和收尾总结。PPT 中个别容易引起误解的说法，也在原页位置给出了教师说明。

## 上课前准备

### 执行目录与课前准备

**本课所有命令均从整套讲义的根目录执行**，也就是同时能看到 `第一课`、`第二课`、`第三课` 等文件夹的位置。请先在 WSL Ubuntu 的 Bash 中进入此目录；每个新终端都保持在该目录。若终端已位于 `第一课`，先执行 `cd ..` 回到讲义根目录。

Python 程序直接使用已提供的 `第一课/课堂实验/` 文件，无需复制到另一份脚本目录。第三课配置直接使用 `第三课/课堂实验/config/`。只有需要 colcon 构建的 ROS 包才由准备脚本写入旁边的 `../ros2_ws/src/`，构建在子 Shell 中进行，完成后当前终端仍位于讲义根目录。

```bash
source /opt/ros/humble/setup.bash
test -d "第一课/课堂实验" || { echo "请先进入整套讲义根目录"; exit 1; }
mkdir -p tf_debug
sudo apt update && sudo apt install -y ros-humble-demo-nodes-cpp ros-humble-std-msgs ros-humble-rosidl-default-generators ros-humble-rmw-cyclonedds-cpp python3-colcon-common-extensions python3-rosdep && \
python3 prepare_course.py 1 --workspace ../ros2_ws && \
(cd ../ros2_ws && colcon build --packages-select robot_interfaces --symlink-install) && \
source ../ros2_ws/install/setup.bash
```

节点持续运行时，后面的查询命令应在另一个终端执行。各终端使用相同 ROS_DOMAIN_ID，先停止旧实验再启动新实验。

建议准备三个 WSL 终端：

```text
终端 A：运行发布者
终端 B：运行订阅者
终端 C：执行调试命令
```

每个新终端先执行：

```bash
source /opt/ros/humble/setup.bash
echo "$ROS_DISTRO"
echo "${ROS_DOMAIN_ID:-0}"
```

如果课堂已经有工作空间，还要执行：

```bash
if [ -f ../ros2_ws/install/setup.bash ]; then
  source ../ros2_ws/install/setup.bash
fi
```

---

## 第 1 页｜ROS 2 话题通信进阶与消息类型学习手册

这页你可以把它讲成：

> **前面我们已经会创建发布者和订阅者了，但“会写代码”只解决了最表面的问题。今天我们要继续向下追：两个节点为什么能自动找到对方？话题明明存在，为什么有时仍然收不到数据？机器人自己的状态，又应该怎样设计成 Python 和 C++ 都能使用的消息？**

先不要急着解释答案，可以把今天的课程画成三条线：

```text
第一条线：消息怎样传
第二条线：消息里面装什么
第三条线：怎样定义自己的消息
```

对应到 PPT：

```text
话题通信与 DDS
      ↓
QoS 与调试
      ↓
std_msgs 标准消息
      ↓
自定义 msg 与编译
```

这里可以问学生：

> 如果 `/scan` 能在 `ros2 topic list` 中看到，但是 `ros2 topic echo /scan` 一直没有输出，你会先改代码，还是先检查通信配置？

学生现在不一定能完整回答。告诉他们：

> **这节课学完以后，我们要能够按顺序定位问题，而不是看到没数据就盲目改程序。**

这页最后可以用一句话总结：

> **今天的目标，是从“会调用 ROS 2 API”走到“能解释通信过程、能选择消息类型、能独立排查问题”。**

---

## 第 2 页｜课程目录

这页你可以把它讲成：

> **今天的内容分成三章，而且三章之间是有前后关系的：先理解数据怎样传，再理解数据用什么格式传，最后自己定义数据格式。**

可以在黑板上写：

```text
通信机制
   ↓
标准消息
   ↓
自定义消息
```

### 第一章解决什么问题

第一章是“话题通信进阶”。

要回答：

```text
发布者和订阅者如何发现彼此？
DDS 在中间做了什么？
QoS 为什么会导致有话题却没数据？
出现问题时应该先查什么？
```

### 第二章解决什么问题

第二章是 `std_msgs` 标准消息。

要回答：

```text
Float32、String、Bool 有什么共同特点？
Header 为什么经常出现在传感器消息里？
MultiArray 为什么能用，却不一定适合长期项目？
什么时候应该使用现成消息？
```

### 第三章解决什么问题

第三章是自定义消息与编译配置。

要回答：

```text
.msg 文件怎样编写？
为什么必须编译？
package.xml 和 CMakeLists.txt 分别负责什么？
Python、C++ 如何使用同一个消息？
```

这里可以提前说明一个容易混淆的地方：

> PPT 中提到“编译期强类型校验”，对 C++ 最直观。Python 也会使用生成的消息类，但很多字段赋值问题是在程序运行时才报错。

这页最后可以这样收：

> **第一章讲通道，第二章讲通道中的数据，第三章讲怎样设计自己的数据。后面的内容都沿着这条顺序展开。**

---

## 第 3 页｜第一章：话题通信进阶

这是一张章节过渡页，不要只念标题。可以这样讲：

> **第一章先把代码放到一边。我们先站在整个系统的角度，看一条消息从发布者出发，经过哪些层，最后怎样触发订阅者的回调。**

在黑板上画最简单的模型：

```text
Publisher
    ↓
  Topic
    ↓
Subscriber
```

然后问学生三个问题：

```text
1. 发布者是否知道订阅者的 IP 地址？
2. 订阅者是否需要先向一个 ROS Master 注册？
3. 话题名相同，是否一定能收到数据？
```

先不公布全部答案，只提示：

```text
ROS 2 通过 DDS 完成发现
话题名只是匹配条件之一
消息类型与 QoS 也会影响通信
```

这页最后可以说：

> **接下来先从发布订阅模型为什么适合机器人系统讲起，然后逐层进入 DDS、发现机制和 QoS。**

---

## 第 4 页｜发布订阅模式的本质：三维解耦

这页你可以把它讲成：

> **发布订阅最大的价值，是发布者只负责产生数据，订阅者只负责使用数据。双方不需要把彼此写死在代码里。PPT 把这种关系概括为时间、空间和流量三个维度的解耦。**

用激光雷达举例：

```text
雷达驱动发布 /scan
        ↓
建图节点订阅
避障节点订阅
RViz 订阅
rosbag2 订阅
```

### 1. 时间解耦

发布者和订阅者可以先后启动。

例如：

```text
先启动雷达
过 10 秒再启动 RViz
双方完成发现后开始通信
```

它们不需要在同一行代码中同时创建，也不需要互相调用函数。

但这里要提醒学生：

> **时间解耦不代表默认会补发之前的数据。默认 `VOLATILE` 模式下，后启动的订阅者只能接收匹配成功之后的新消息。历史数据是否补发，要看后面讲的 Durability。**

### 2. 空间解耦

雷达驱动一般不需要知道：

```text
订阅者在哪台电脑
订阅者叫什么节点名
订阅者内部使用 Python 还是 C++
```

它只需要按照约定发布指定话题和类型。

可以类比广播电台：

> 电台只负责在某个频道播出，收音机调到这个频道即可。电台不需要提前保存每一台收音机的地址。

### 3. 流量解耦

一个发布者可以对应多个订阅者：

```text
        ┌─ 建图
/scan ──┼─ 避障
        ├─ RViz
        └─ 录包
```

后来增加一个诊断节点，一般不需要修改雷达驱动代码。

### 课堂提问

可以问：

> 如果把建图算法从 A 换成 B，雷达驱动是否也必须重写？

答案：只要话题名、消息类型和 QoS 仍然兼容，雷达驱动通常不需要修改。

### 补充命令行对照实验

目的：观察订阅者退出不会自动停止生产者。A 用官方 talker，B listener，C 查询：
```bash
# A
ros2 run demo_nodes_cpp talker
# B
ros2 run demo_nodes_cpp listener
# C
ros2 topic info /chatter
```
B Ctrl+C，C 再查 info，观察订阅者数量减少，A 仍发布。B 重启后恢复接收。不要把这种解耦解释为资源完全不受影响；DDS 缓存与网络仍可能受订阅端情况影响。实验后停止 A/B。

这页最后可以总结：

> **发布订阅把“谁产生数据”和“谁使用数据”分开，所以机器人系统能够方便地增加、替换和重启节点。**

---

## 第 5 页｜四种通信模式对比

这页你可以把它讲成：

> **ROS 2 不只有 Topic。选择通信模式时，要先判断数据是持续流动、一次请求，还是一个需要反馈和取消的长任务。**

可以用同一台机器人上的四个例子解释。

### 1. Topic：持续的数据流

例如：

```text
/scan
/camera/image_raw
/odom
/battery_state
```

这些数据不断产生，可能有多个消费者，适合 Topic。

Topic 的思考方式是：

> “我持续发布当前数据，谁需要谁来订阅。”

### 2. Service：一次请求，一次响应

例如：

```text
查询一次设备状态
清零里程计
切换一次工作模式并返回结果
```

客户端发出请求，服务端处理后返回响应。

PPT 写“同步阻塞”，课堂上要补充：

> **Service 表达的是请求和响应关系。客户端既可以同步等待，也可以使用异步调用 API。**

### 3. Action：耗时任务

例如：

```text
导航到目标点
机械臂移动到指定姿态
执行一段长时间巡检任务
```

Action 除了目标和最终结果，还能：

```text
持续返回进度
中途取消
报告任务状态
```

### 4. Parameter：节点配置

例如：

```text
最大速度
避障距离阈值
控制器增益
是否启用某功能
```

参数是节点管理配置的一套机制，其查询和设置常通过参数服务完成。

### 课堂判断题

可以逐个问学生：

```text
摄像头画面                → Topic
请求保存一次地图          → Service
导航到教室门口并显示进度  → Action
把最大速度改成 0.5 m/s    → Parameter
```

不要仅根据动词判断。例如“启动电机”可能需要确认结果和安全反馈，实际设计要看业务要求。

这页最后可以总结：

> **持续数据用 Topic，一问一答用 Service，耗时且需要反馈的任务用 Action，运行配置用 Parameter。**

---

## 第 6 页｜ROS 2 完整通信栈

这页你可以把它讲成：

> **我们在 Python 中只写了一句 `publisher.publish(msg)`，但这条消息真正到达网络之前，还会经过多个层次。每一层只负责自己的工作。**

先按 PPT 画出通信栈：

```text
业务节点 Node
      ↓
rclpy / rclcpp
      ↓
     rcl
      ↓
     rmw
      ↓
Fast DDS / Cyclone DDS / 其他实现
      ↓
传输协议与操作系统网络
```

### 1. 业务节点

这是我们自己写的逻辑：

```python
msg.data = 'hello'
self.publisher.publish(msg)
```

节点关心“发什么”，一般不直接处理数据包怎样分片、重传和发现。

### 2. rclpy 与 rclcpp

它们是语言层 API：

```text
rclpy  → Python
rclcpp → C++
```

所以 Python 和 C++ 写法不同，但都进入同一套 ROS 2 通信体系。

### 3. rcl

`rcl` 是语言无关的 C 层核心接口。可以把它理解成多个客户端库共同使用的基础层。

### 4. rmw

`rmw` 的全称是 ROS Middleware Interface。

它像一个转换插座：

```text
ROS 2 上层统一接口
        ↓
不同 DDS / 中间件实现
```

上层业务代码不必为每一种 DDS 重写一套发布逻辑。

### 5. DDS 与传输

DDS 实现负责发现、匹配、序列化、传输和 QoS 等工作。RTPS 是 DDS 互操作中常见的线协议。

这里要提醒：

> **这张图表示职责分层，不代表每条消息都机械地经过一个独立的 RTPS 软件模块。同机通信还可能使用进程内通信或共享内存优化。**

### 课堂比喻

可以类比寄快递：

```text
业务节点       → 写好要寄的内容
rclpy/rclcpp   → 填写统一格式的寄件单
rcl / rmw      → 对接不同快递系统
DDS 与网络     → 真正分拣和运输
```

这页最后可以总结：

> **我们写的是上层业务，ROS 2 通过 rcl 和 rmw 屏蔽底层差异，再由具体中间件完成发现与传输。**

---

## 第 7 页｜DDS 核心角色

这页你可以把它讲成：

> **进入 DDS 以后，最重要的是认识四个角色：谁加入通信域、按什么通道匹配、谁写数据、谁读数据。**

先建立对应关系：

```text
DDS Domain Participant  → 参与通信域的实体
DDS Topic               → 名称和数据类型定义的逻辑通道
DDS DataWriter          → 写数据的端点
DDS DataReader          → 读数据的端点
```

### 1. Domain Participant

Participant 表示一个参与 DDS 通信域的实体。

ROS 2 常通过：

```bash
echo "$ROS_DOMAIN_ID"
```

查看当前域编号。未显式设置时通常使用 0。

不同 Domain ID 的节点通常互相隔离：

```text
Domain 0 中的节点  ✕  Domain 5 中的节点
```

但不要告诉学生“一个 ROS 节点必定对应一个 Participant”。具体映射由 RMW 实现决定。

### 2. Topic

DDS 匹配不能只看名字，还要看类型：

```text
话题名称 + 消息类型
```

例如双方都使用 `/temperature`，但一端是 `Float32`，另一端是 `String`，不能按同一个消息接口正常通信。

### 3. DataWriter

DataWriter 是写端。ROS 2 的 Publisher 会通过底层写端把序列化后的消息交给中间件。

每个端点可以有自己的 QoS，例如：

```text
RELIABLE
BEST_EFFORT
TRANSIENT_LOCAL
```

### 4. DataReader

DataReader 是读端。收到的数据进入相应缓存，ROS 2 执行器再调度订阅回调。

### 课堂提问

问学生：

> 两个节点在同一个 Domain，话题名也一样，是否一定可以通信？

答案：还要检查消息类型与 QoS 是否兼容。

PPT 中给出端口计算示意，可以用于说明 Domain 会影响底层通信，但不要把单一公式讲成全部 DDS 端口规则。

这页最后可以总结：

> **Participant 决定加入哪个通信域，Topic 提供名称与类型，DataWriter 写，DataReader 读，QoS 决定双方能否按要求匹配。**

---

## 第 8 页｜DDS 发现机制：PDP 与 EDP

这页你可以把它讲成：

> **ROS 2 节点不需要先向 ROS 1 Master 报到，它们通过 DDS 发现机制找到彼此。发现分成两层：先发现“网络里有谁”，再发现“对方提供或需要什么数据”。**

整条链路可以画成：

```text
节点启动
   ↓
PDP：发现参与者
   ↓
EDP：交换端点信息
   ↓
检查 Topic 名称、类型和 QoS
   ↓
匹配成功
   ↓
开始传输数据
```

### 1. PDP：参与者发现

PDP 可以先理解成“找到同一个通信域里有哪些参与者”。

交换的信息包括：

```text
身份标识
网络地址
Domain 信息
```

可以类比进入教室后先点名：

> 先知道谁在教室里，还没有开始讨论每个人要发送什么数据。

### 2. EDP：端点发现

发现参与者以后，再交换 DataWriter 和 DataReader 信息：

```text
话题名称
消息类型
QoS 配置
```

可以类比点名结束后再统计：

```text
谁提供 /scan？
谁需要 /scan？
双方使用的消息类型是否相同？
接收要求能否由发送端满足？
```

### 为什么 `topic list` 看得到却没有数据

因为：

```text
发现节点或话题
不等于
端点一定兼容并成功传输
```

可能原因包括：

```text
QoS 不兼容
发布端暂时没有真正 publish
消息只发布过一次，订阅者加入太晚
网络只能部分发现，数据路径仍有问题
```

### 教师说明

局域网默认配置常用多播发现，但 DDS 也能配置单播、静态发现或 Discovery Server。不要把“必定广播”和“绝对没有中心服务”讲成所有部署方式的规则。

这页最后可以总结：

> **PDP 先发现参与者，EDP 再发现读写端点；名称、类型和 QoS 兼容以后，数据才真正开始流动。**

---

## 第 9 页｜主流 DDS 实现对比

这页你可以把它讲成：

> **DDS 是标准，Fast DDS、Cyclone DDS、Connext DDS 等是对这个标准的具体实现。它们就像不同厂商生产的浏览器，都遵循共同标准，但功能、性能和工具不完全相同。**

### 1. Fast DDS

由 eProsima 开发，开源，ROS 2 中使用广泛，并提供 Discovery Server 等功能。

### 2. Cyclone DDS

由 Eclipse 基金会项目维护，开源，也常用于 ROS 2。很多项目会在自己的硬件和数据规模下对它进行性能测试。

### 3. Connext DDS

由 RTI 提供，工业应用较多，授权方式需要按具体项目和版本核查。

### 4. GurumDDS

商业 DDS 实现，在特定行业和项目中使用。

这里不要让学生死记“谁最快”。应该告诉他们：

> **DDS 的表现与消息大小、频率、网络、操作系统、QoS 和版本都有关系。没有脱离测试条件的绝对性能排名。**

### 怎样切换 RMW 实现

如果已经安装 Cyclone DDS 对应的 RMW，可以在启动节点前设置：

```bash
# 已在课前安装 ros-humble-rmw-cyclonedds-cpp；停止旧节点后在各业务终端切换
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
```

查看：

```bash
echo "$RMW_IMPLEMENTATION"
```

然后重新启动 ROS 2 节点。

注意：

```text
设置环境变量 ≠ 自动安装对应实现
通信相关终端应使用兼容的配置
切换后需要重新启动进程
```

### 课堂提问

> 为什么切换 DDS 实现通常不需要改 `publisher.publish(msg)`？

因为上层 API 通过 `rmw` 抽象了底层差异。

这页最后可以总结：

> **DDS 是标准，具体 DDS 产品负责实现；ROS 2 用 RMW 把上层业务和底层实现分开。**

---

## 第 10 页｜话题通信完整流程

这页你可以把它讲成：

> **前面认识了通信栈和发现机制，现在把一次完整话题通信串起来：从节点启动，一直到订阅回调真正执行。**

整条链路可以画成：

```text
节点启动
   ↓
创建 Participant 与读写端点
   ↓
PDP / EDP 发现
   ↓
检查名称、类型、QoS
   ↓
发布者调用 publish()
   ↓
序列化与传输
   ↓
订阅端接收并反序列化
   ↓
Executor 调度回调
   ↓
业务代码处理消息
```

### 阶段一：节点启动与发现

发布端和订阅端启动以后，DDS 先完成参与者与端点发现。

可以强调：

```text
发现成功，只代表双方知道彼此存在
端点兼容，才代表可能交换数据
```

### 阶段二：检查匹配条件

主要检查：

```text
话题名是否一致
消息类型是否一致
QoS 是否兼容
```

### 阶段三：发布消息

应用层调用：

```python
self.publisher.publish(msg)
```

消息交给 ROS 2 和中间件，经过序列化后进入传输过程。

### 阶段四：订阅与回调

订阅端收到消息后，执行器负责调度回调：

```python
def callback(msg):
    print(msg.data)
```

如果没有 `spin` 或执行器没有运行，回调就不会被正常调度。

### 教师说明

PPT 把 `publish()` 描述成“一定立即返回”，这句话过于绝对。通常发布 API 不会等待订阅业务处理结束，但在不同 RMW、QoS、缓存和资源条件下，调用行为可能不同。

`RELIABLE` 会利用确认与重传提高传送保障，但不能保证应用永远不会丢状态，也不能代替控制系统的反馈、超时和故障处理。

这页最后可以总结：

> **发现负责找到对方，匹配负责确认双方能否通信，DDS 负责传输，Executor 最后把收到的消息交给用户回调。**

---

## 第 11 页｜话题通信编程模型：四要素

这页你可以把它讲成：

> **写一个完整的话题节点，至少要理解四个对象：Node 提供运行环境，Publisher 发送，Subscription 接收，Spin 让回调真正运行。**

### 1. Node：通信对象的容器

节点可以提供：

```text
发布与订阅
日志
参数
时钟
服务与动作等能力
```

Python 常见写法：

```python
class DemoNode(Node):
    def __init__(self):
        super().__init__('demo_node')
```

一个进程可以运行多个节点，一个节点也可以创建多个发布者和订阅者。不要把“节点”简单等同于“进程”。

### 2. Publisher：发送端

创建发布者时要给出：

```text
消息类型
话题名称
QoS 或队列深度
```

例如：

```python
self.pub = self.create_publisher(String, '/demo', 10)
```

### 3. Subscription：接收端

订阅者还要绑定回调：

```python
self.sub = self.create_subscription(
    String,
    '/demo',
    self.callback,
    10
)
```

收到消息以后，执行：

```python
def callback(self, msg):
    self.get_logger().info(msg.data)
```

### 4. Spin 与 Executor

主程序通常写：

```python
rclpy.spin(node)
```

可以把 `spin` 类比成值班人员：

> 它持续等待定时器、订阅消息和服务请求，一旦有事件，就安排对应回调执行。

如果创建了订阅者却没有运行执行器，消息即使到达底层，用户回调也不会正常执行。

### 课堂提问

> `callback()` 为什么不需要我们自己手动调用？

因为回调由 Executor 在事件到达时调度。

### 教师说明

节点名最好保持清晰且避免冲突，但 ROS 2 图中并非所有场景都强制节点名全局唯一。多线程执行器是否真正并发，还受回调组和语言运行时等因素影响。

这页最后可以总结：

> **Node 提供环境，Publisher 和 Subscription 建立数据通道，Spin 驱动事件，回调处理业务。**

---

## 第 12 页｜话题调试工具大全

这页你可以把它讲成：

> **调试话题不要一上来就改代码。ROS 2 已经提供了一套命令，可以从“有没有话题”一路查到“数据频率和带宽是否正常”。**

先在终端 A 产生一个持续话题：

```bash
ros2 topic pub -r 1 /demo std_msgs/msg/String "{data: hello}"
```

然后在终端 B 依次讲下面的命令。

### 1. 查看话题列表

```bash
ros2 topic list
```

同时显示消息类型：

```bash
ros2 topic list -t
```

它解决的问题是：

> “系统中有没有这个话题？名字是否写对？”

### 2. 查看话题端点信息

```bash
ros2 topic info /demo
```

查看更详细的 QoS：

```bash
ros2 topic info /demo --verbose
```

它能看到发布者、订阅者数量和端点 QoS。

### 3. 查看消息内容

```bash
ros2 topic echo /demo
```

预期看到：

```yaml
data: hello
---
```

按 `Ctrl+C` 结束。

### 4. 模拟发布

```bash
ros2 topic pub --once /test std_msgs/msg/String "{data: test_message}"
```

它适合在没有真实发布节点时测试订阅逻辑。

### 5. 测量接收频率

```bash
ros2 topic hz /demo
```

如果发布频率为 1 Hz，输出应接近 1 Hz，但会受采样窗口和系统调度影响。

### 6. 查看接收带宽

```bash
ros2 topic bw /demo
```

它显示该命令作为订阅者观察到的数据量，不等于整个网络接口的总带宽。

### 7. 根据类型反查话题

```bash
ros2 topic find std_msgs/msg/String
```

### 8. 查看指定话题类型

```bash
ros2 topic type /demo
```

### 课堂演示顺序

建议严格按：

```text
list
 ↓
type / info
 ↓
echo
 ↓
hz / bw
```

如果选项记不清：

```bash
ros2 topic echo -h
ros2 topic pub -h
```

### 教师说明

`ros2 topic hz` 和 `bw` 是工具端实际接收到的统计值，会受到 QoS、网络与机器负载影响，也不能直接代表端到端延迟。

### 补充命令行对照实验

目的：改变发布频率而保持名称和内容，观察测量变化。先停止原 /demo 发布者，A：
```bash
ros2 topic pub /demo std_msgs/msg/String "{data: rate_test}" --rate 2
```
B 测量：
```bash
ros2 topic hz /demo
```
观察多个采样周期，退出 B/A；A 改 --rate 10，B 重新启动 hz。预期频率从约 2Hz 到约 10Hz。观察窗口、CPU 和 QoS 会影响结果，不能把 hz 当端到端延迟。测完 Ctrl+C，避免两个生产者混在同一话题。

这页最后可以总结：

> **先用 list 确认存在，再用 info 和 type 查匹配条件，用 echo 看内容，最后用 hz 和 bw 看运行质量。**

---

## 第 13 页｜通信故障标准排查流程

这页你可以把它讲成：

> **排查通信故障像医生问诊，要按顺序缩小范围。每执行一个命令，都要知道它排除了哪一类问题。**

把流程写成：

```text
话题存在吗？
    ↓
发布者和订阅者存在吗？
    ↓
类型与 QoS 兼容吗？
    ↓
发布端真的在发吗？
    ↓
跨机器网络能通吗？
```

### 第一步：确认话题是否存在

```bash
ros2 topic list
```

如果没有目标话题，先检查：

```text
节点是否启动
节点是否已经退出或报错
话题名称是否拼错
是否使用了不同 ROS_DOMAIN_ID
发布者是否真的创建
```

还可以看节点：

```bash
ros2 node list
```

### 第二步：确认端点数量

```bash
ros2 topic info /demo
```

例如：

```text
Publisher count: 1
Subscription count: 0
```

这表示有发布端，但当前没有订阅端。

如果 Publisher count 是 0，要回到发布节点检查；如果订阅端数量是 0，要检查订阅节点是否启动、话题名是否一致。

### 第三步：检查类型和 QoS

```bash
ros2 topic info /demo --verbose
```

重点看：

```text
Topic type
Reliability
Durability
History
Depth
```

常见现象：

```text
发布者 BEST_EFFORT
订阅者要求 RELIABLE
→ 可靠性不兼容，收不到数据
```

### 第四步：检查消息内容

```bash
ros2 topic echo /demo
```

可能出现三种结果：

```text
完全没输出   → 继续查通信或发布行为
有输出但值错 → 查发布端业务逻辑
时间戳异常   → 查时钟源和采样时间
```

### 第五步：多机网络排查

先确认：

```text
双方 ROS_DOMAIN_ID 是否一致
IP 网络是否互通
DDS 发现配置是否一致
防火墙是否允许所用 DDS 流量
```

WSL 场景还要注意：

```text
WSL 1 与 WSL 2 网络模式不同
NAT、镜像网络和局域网多播表现可能不同
同一 WSL 内能通，不代表跨 Windows 或跨主机自动能通
```

不要把“关闭防火墙”当成第一步。先确认问题范围，再按实际 DDS 配置放行需要的流量。

### 课堂故障题

给出：

```text
topic list 能看到 /scan
info 显示 1 个发布者、1 个订阅者
echo 没有数据
发布端 BEST_EFFORT
订阅端 RELIABLE
```

让学生回答：应该先改哪里？

答案：先调整订阅端 QoS，使可靠性请求与发布端兼容。

这页最后可以总结：

> **先查图，再查端点，再查类型与 QoS，最后才查业务内容和网络；每一步都在缩小问题范围。**

---

## 第 14 页｜QoS 服务质量策略

这页你可以把它讲成：

> **QoS 决定的不是消息内容，而是消息应该以什么方式被保存和传递。不同机器人数据，对“快、稳、是否保留历史”的要求并不相同。**

先用两个场景对比：

```text
激光雷达：每秒几十帧，旧帧很快失去价值
地图快照：可能只发布一次，后启动节点也需要得到
```

如果两者都使用完全相同的策略，就很难同时满足需求。

### ROS 1 的对比背景

ROS 1 常见通信使用 TCPROS，也支持 UDPROS。课堂可以概括为：

```text
TCPROS 更重视可靠传输
UDPROS 更重视及时性
```

ROS 2 借助 DDS，把更多策略拆成可组合的 QoS Policy。

### ROS 2 QoS 在管什么

可以先报出几类问题：

```text
消息丢了要不要重传？          Reliability
新订阅者要不要拿到旧消息？    Durability
队列保留多少条？              History + Depth
多久没更新算异常？            Deadline
一条消息多久以后算过期？      Lifespan
发布者是否仍然存活？          Liveliness
```

### 三个典型选择

高频传感器：

```text
允许少量丢帧
更重视及时拿到最新数据
常用 BEST_EFFORT
```

地图或静态配置：

```text
更新少
迟加入者也需要当前版本
常考虑 TRANSIENT_LOCAL
```

系统健康监测：

```text
期望按周期更新
超时要触发异常处理
可使用 Deadline 或 Liveliness 事件
```

### 教师说明

PPT 把控制指令描述成“使用 RELIABLE 就能杜绝失控”，这个说法不准确。可靠传输不能保证消息永不过期，也不能证明设备已经执行。安全控制仍要配合：

```text
命令有效期
执行反馈
看门狗
超时停机
状态机与故障处理
```

这页最后可以总结：

> **QoS 没有统一的最优配置，它要根据数据是否允许丢失、是否怕延迟、是否需要历史记录来选择。**

---

## 第 15 页｜核心 QoS：History、Depth 与 Reliability

这页你可以把它讲成：

> **History 和 Depth 决定队列怎样保存消息，Reliability 决定发送端和接收端采用怎样的交付要求。一个管缓存，一个管传递。**

### 1. KEEP_LAST

例如：

```text
KEEP_LAST，depth = 3
```

队列中只保留最近三条：

```text
发布 1、2、3  → [1, 2, 3]
再发布 4      → [2, 3, 4]
```

当订阅者处理不过来时，更早的数据会被新数据替换。

适合关注当前状态的场景：

```text
温度
位置
设备心跳
传感器连续数据
```

### 2. KEEP_ALL

`KEEP_ALL` 表示尽量保留所有尚未处理的样本，但仍受 DDS 资源限制。

不要告诉学生：

```text
KEEP_ALL = 无限内存
```

更准确的说法是：

> 中间件在配置的资源限制内保留所有样本。生产系统仍要考虑发送速度、消费速度和内存。

### 3. Depth 怎样估算

PPT 给出：

```text
depth ≥ 处理延迟 × 发布频率
```

例如 100 Hz 发布，处理可能阻塞 0.05 秒：

```text
100 × 0.05 = 5
```

可以把 5 当成起始估算，但实际还要留出抖动余量，并考虑消息大小和内存。

### 4. RELIABLE

RELIABLE 会利用确认和重传机制提高消息交付保障。

适合：

```text
低频但重要的状态
配置更新
不能轻易跳过的业务事件
```

代价可能包括额外网络开销、缓存压力和等待。

### 5. BEST_EFFORT

BEST_EFFORT 尽力发送，不要求每个样本都被确认和重传。

适合：

```text
激光雷达
摄像头
高频 IMU
```

丢掉一帧后，很快会有更新的数据。

### 课堂提问

> 100 Hz 的激光数据丢了一帧，和“切换到急停状态”的消息丢了一条，后果是否相同？

让学生从业务后果出发选择策略。

### 教师说明

`KEEP_ALL` 不要求必须和 RELIABLE 绑定才能匹配。`RELIABLE` 也不等于零丢失、零延迟，更不能替代应用层确认。

这页最后可以总结：

> **History 和 Depth 决定保留哪些样本，Reliability 决定是否要求可靠交付；先看业务后果，再定策略。**

---

## 第 16 页｜核心 QoS：Durability 与其他策略

这页你可以把它讲成：

> **Durability 解决的是“后来加入的人，能不能拿到以前发布的数据”。Deadline、Lifespan 和 Liveliness 则分别关注更新周期、消息有效期和发布者活性。**

### 1. VOLATILE

场景：

```text
发布者先发送一条消息
订阅者之后才启动
```

如果是 `VOLATILE`：

```text
过去的消息不补发
订阅者只接收匹配后的新消息
```

它适合连续更新的数据，例如摄像头或 IMU。

### 2. TRANSIENT_LOCAL

发布者在本地保留一定历史样本。迟加入订阅者也使用兼容的 `TRANSIENT_LOCAL` 后，可以取得发布者仍然保存的历史数据。

典型场景：

```text
地图节点发布一次地图
导航节点稍后启动
导航节点仍要获得当前地图
```

这里要强调三个条件：

```text
发布端使用 TRANSIENT_LOCAL
迟加入订阅端也请求 TRANSIENT_LOCAL
发布者仍在运行，而且历史样本仍在缓存中
```

它不是把消息永久保存到硬盘。发布者退出后，这份本地缓存通常也不再可用。

### 3. Deadline

Deadline 表示期望的最大更新间隔。

例如传感器应每 100 ms 更新：

```text
连续超过预期时间没有新消息
       ↓
触发 Deadline 相关事件
       ↓
系统可以报告传感器可能掉线
```

### 4. Lifespan

Lifespan 是消息的最长有效时间。

例如控制或定位消息只在 0.5 秒内有效，超过时间后就不应继续交给下游当成新数据处理。

### 5. Liveliness

Liveliness 关注发布者是否仍被认为存活。它可以帮助系统发现发布端失联，并触发主备切换、告警或资源释放。

### 课堂提问

> 发布地图的节点发完数据后立刻退出，半分钟以后启动导航节点，它一定能收到 TRANSIENT_LOCAL 历史吗？

答案：通常不能，因为保存样本的发布者已经退出。

### 补充命令行对照实验

目的：严格验证晚加入缓存。先停原发布者；A 不等待接收端，只先发一次并存活 60 秒：
```bash
ros2 topic pub /lesson1/static std_msgs/msg/String "{data: map_revision_1}"   --qos-durability transient_local --qos-reliability reliable --qos-depth 1   --times 1 --wait-matching-subscriptions 0 --keep-alive 60
```
看到 A 已发布后，B 再启动：
```bash
ros2 topic echo /lesson1/static --qos-durability transient_local --once
```
预期得到旧样本。C info --verbose 核对发布者仍存在。发布端退出后再接入不保证收到；不要把 CLI --once 已退出的发布者用来验证此实验。

这页最后可以总结：

> **Durability 管历史，Deadline 管更新间隔，Lifespan 管消息是否过期，Liveliness 管发布者是否还活着。**

---

## 第 17 页｜QoS 匹配规则

这页你可以把它讲成：

> **发布端是在“提供服务质量”，订阅端是在“提出最低要求”。只有发布端提供的能力能够满足订阅端要求，双方才会建立匹配。**

可以先写：

```text
Publisher = Offered，提供方
Subscriber = Requested，请求方
```

### 1. Reliability 匹配

```text
发布 BEST_EFFORT  + 订阅 BEST_EFFORT  → 可以
发布 BEST_EFFORT  + 订阅 RELIABLE     → 不可以
发布 RELIABLE     + 订阅 BEST_EFFORT  → 可以
发布 RELIABLE     + 订阅 RELIABLE     → 可以
```

原因可以这样解释：

> 发布者如果只承诺“尽力而为”，就无法满足订阅者提出的“必须可靠”要求。

### 2. Durability 匹配

```text
发布 VOLATILE         + 订阅 VOLATILE         → 可以
发布 VOLATILE         + 订阅 TRANSIENT_LOCAL  → 不可以
发布 TRANSIENT_LOCAL  + 订阅 VOLATILE         → 可以，只收新消息
发布 TRANSIENT_LOCAL  + 订阅 TRANSIENT_LOCAL  → 可以，可接收保留历史
```

### 3. Deadline 匹配

如果发布者承诺最长每 100 ms 发布一次，而订阅者能接受最长 200 ms：

```text
订阅者要求 200 ms
发布者提供 100 ms
→ 发布者提供得更频繁，可以满足
```

反过来，订阅者要求 50 ms，发布者只能承诺 100 ms，就不兼容。

### 4. PPT 中 Depth 公式的纠正

PPT 写：

```text
Pub Depth ≥ Sub Depth 才能匹配
```

这条不能按公式讲。

更准确的说法：

> **History 和 Depth 控制各端怎样缓存样本，不存在“发布端 Depth 必须大于订阅端 Depth 才能连接”的通用 QoS 匹配规则。两个端点的 Depth 不同，也可能正常匹配。**

建议上课时直接在 PPT 这一行旁边标注“此处不作为匹配条件”。

### 实际诊断

```bash
ros2 topic info /scan --verbose
```

让学生重点对比发布端和订阅端的 Reliability、Durability。

### 课堂判断

```text
Pub: RELIABLE + VOLATILE
Sub: BEST_EFFORT + VOLATILE
```

可以通信。

```text
Pub: BEST_EFFORT + VOLATILE
Sub: RELIABLE + VOLATILE
```

可靠性不兼容。

### 补充命令行对照实验

目的：只改变可靠性，复现不兼容再修复。A：
```bash
ros2 topic pub /lesson1/qos std_msgs/msg/String "{data: sample}" --rate 2 --qos-reliability best_effort
```
B：
```bash
ros2 topic echo /lesson1/qos --qos-reliability reliable
```
C：
```bash
ros2 topic info /lesson1/qos --verbose
```
预期 B 不接收或报告不兼容。B Ctrl+C 改 best_effort，预期收到。再保留可靠性一致而让两端 depth 不同，查询可匹配，证明没有 PubDepth≥SubDepth 的通用要求。每轮先停旧端点，结束 A/B。

这页最后可以总结：

> **订阅者的要求不能超过发布者能够提供的质量；排查“有话题无数据”时，先查 Reliability 和 Durability。**

---

## 第 18 页｜QoS 预设 Profile 与最佳实践

这页你可以把它讲成：

> **QoS 参数很多，ROS 2 给常见场景准备了一些预设。预设不是必须照搬的答案，但可以作为合理起点。**

### 1. 默认话题 Profile

Humble 中常见默认值：

```text
KEEP_LAST
Depth = 10
RELIABLE
VOLATILE
```

Python 中把整数 `10` 传给 `create_publisher()`，常见含义就是创建 depth 为 10 的默认 QoS。

### 2. Sensor Data Profile

传感器数据通常：

```text
KEEP_LAST
较小 Depth
BEST_EFFORT
VOLATILE
```

Python 可直接使用：

```python
from rclpy.qos import qos_profile_sensor_data

self.sub = self.create_subscription(
    LaserScan,
    '/scan',
    self.callback,
    qos_profile_sensor_data
)
```

这里的设计思路是：

> 与其等待丢失旧帧重传，不如尽快得到最新帧。

### 3. Parameters Profile

参数机制基于服务，需要可靠传输，并使用较大的队列深度来应对请求暂时积压。

### 4. System Default

`system_default` 让底层 RMW/DDS 使用自己的默认值。

要提醒学生：

```text
不同 RMW 的 system default 可能不同
不要把它当成固定配置
```

### 场景化选择

```text
雷达、相机、IMU
→ sensor_data 作为起点

普通状态广播
→ default 或按业务定制

地图和静态配置
→ RELIABLE + TRANSIENT_LOCAL，通常只保留少量最新样本
```

### 调试方法

先看当前端点：

```bash
ros2 topic info /scan --verbose
```

如果通信异常，可以先让双方使用已知兼容的配置，打通后每次只调整一个参数并观察变化。

不要把 PPT 场景表中的数字当成所有项目的固定答案。队列深度需要结合消息频率、大小、处理时间和内存测试。

这页最后可以总结：

> **预设 Profile 帮我们快速开始，真正的 QoS 仍要根据发布端配置和业务需求验证。**

---

## 第 19 页｜第二章：std_msgs 标准消息类型

这是一张章节过渡页，可以这样讲：

> **第一章回答了“消息怎样到达”。第二章开始回答“到达的消息里面有什么”。发布者和订阅者必须对数据结构有共同理解，这个共同约定就是消息接口。**

先写一个最简单的接口：

```text
std_msgs/msg/Float32

float32 data
```

然后问学生：

> 如果我只告诉你 `data = 25.5`，你知道它是温度、电压、速度，还是电池电量吗？

这会引出第二章的核心：

```text
标准消息使用方便
但简单结构不一定包含足够业务语义
```

接下来按 PPT 顺序认识：

```text
std_msgs 的定位
标量类型
MultiArray
Header 和时间
特殊消息与强类型
适用边界
```

这页最后可以说：

> **会选消息类型，比单纯记住消息名字更重要。**

---

## 第 20 页｜std_msgs 的定位与设计原则

这页你可以把它讲成：

> **`std_msgs` 提供一组简单、通用的消息接口。它们很适合入门、快速验证和传递单一值，但并不负责表达所有机器人业务语义。**

### 1. 最简单的例子

查看 Float32：

```bash
ros2 interface show std_msgs/msg/Float32
```

会看到：

```text
float32 data
```

Python 使用：

```python
from std_msgs.msg import Float32

msg = Float32()
msg.data = 25.5
```

### 2. 为什么字段经常叫 data

很多标量消息只有一个字段：

```text
Bool     → bool data
Int32    → int32 data
Float64  → float64 data
String   → string data
```

这样接口很简单，也方便快速调试。

### 3. 简单消息的局限

如果 `/temperature` 只发布：

```yaml
data: 25.5
```

接收者还不知道：

```text
单位是摄氏度还是华氏度？
数据什么时候采集？
来自哪个传感器？
是否有效？
```

当这些信息影响业务时，应使用更合适的现成接口，或定义有语义的自定义消息。

### 4. 工具兼容的正确理解

ROS 2 工具通过接口类型系统理解消息，并非只有 `std_msgs` 才能录包或查看。自定义消息只要正确构建并在环境中可发现，也可以被 `ros2 topic echo` 和 rosbag2 使用。

### 教师说明

PPT 把 `std_msgs` 描述成所有复杂消息的“原子单元”，可以作为入门类比，但技术上复杂 `.msg` 可以直接使用内置字段类型，不需要先把每个字段包装成 `std_msgs/Float32`。

这页最后可以总结：

> **`std_msgs` 适合简单通用的数据交换；数据一旦需要时间、坐标、单位或多个关联字段，就要考虑更有语义的接口。**

---

## 第 21 页｜基础标量消息：14 种标准类型

这页你可以把它讲成：

> **这一页类型很多，不需要学生死背每一个范围。重点是先按布尔、整数、浮点和字符串分类，再根据范围、精度和语义做选择。**

### 1. Bool

```text
bool data
```

适合开关、使能状态：

```yaml
data: true
```

### 2. 有符号整数

```text
Int8
Int16
Int32
Int64
```

有正数也有负数。选型时确认范围是否足够。

例如计数器可以使用 Int32，但具有明确状态含义的数据，长期项目更适合用自定义字段和常量表达。

### 3. 无符号整数

```text
UInt8
UInt16
UInt32
UInt64
```

只表示非负值。例如字节数据经常使用 UInt8。

PPT 中的 `Byte`、`Char` 在 ROS 2 已标为弃用，新接口优先考虑 `UInt8`、`Int8` 或更有语义的自定义消息。

### 4. 浮点数

```text
Float32 → 4 字节，约 6～7 位十进制有效数字
Float64 → 8 字节，约 15～17 位十进制有效数字
```

Float64 精度更高，但消息更大。根据计算精度和接口约定选择，不是所有数据都必须用 Float64。

### 5. String

```text
string data
```

适合文本标签、简单日志或演示数据。不要为了省事把结构化数值都拼成字符串，否则接收端还要手工解析，也失去类型检查。

### 课堂演示

```bash
ros2 interface show std_msgs/msg/Int32
ros2 interface show std_msgs/msg/Float32
ros2 interface show std_msgs/msg/String
```

发布温度：

```bash
ros2 topic pub --once /temperature \
  std_msgs/msg/Float32 \
  "{data: 25.5}"
```

### 课堂提问

```text
温度 25.5 用 Int32 合适吗？
像素灰度 0～255 可以考虑什么类型？
纳秒级时间为什么更推荐标准 Time，而不是裸 Int64？
```

答案重点放在“语义和标准接口”，不只看能否装下这个数字。

### 教师说明

Python 使用生成的消息类，但赋值类型或范围错误通常在运行时暴露。`Float32` 也不能精确表示所有十进制小数。

### 补充命令行对照实验

目的：看强类型值域检查，不只是读类型名。B 先接收合法样本，A 单次发布：
```bash
# B
ros2 topic echo /lesson1/integer std_msgs/msg/Int32
# A
ros2 topic pub /lesson1/integer std_msgs/msg/Int32 "{data: 42}" --once
ros2 topic pub /lesson1/integer std_msgs/msg/Int32 "{data: 2147483648}" --once
```
预期合法值 42 被接收，超出 int32 最大值的输入在填充/类型检查时报错，不生成合法样本。若关闭 Python 断言或修改实现，报错形式可能不同，验收重点是该值不属于接口值域。结束 B。

这页最后可以总结：

> **先看数据有没有正负，再看范围和精度，最后确认单个 `data` 字段能否表达完整语义。**

---

## 第 22 页｜数组类型消息：MultiArray

这页你可以把它讲成：

> **MultiArray 可以传一串数，也能描述多维布局。但它只告诉接收者“这些数怎样排列”，不一定告诉接收者“这些数在业务上代表什么”。**

先看结构：

```bash
ros2 interface show std_msgs/msg/Float64MultiArray
```

会看到类似：

```text
std_msgs/MultiArrayLayout layout
float64[] data
```

### 1. data

`data` 是一维连续数组：

```python
msg.data = [1.0, 2.0, 3.0]
```

但只看这三个数字，不知道它们是：

```text
x、y、z 坐标
RGB 三个通道
三次温度测量
三个电机速度
```

### 2. layout

`layout` 中主要有：

```text
dim[]
data_offset
```

每个维度包含：

```text
label   维度名称
size    维度长度
stride  跨越该维度时对应的元素跨度
```

例如 2×3 矩阵最终仍按一维数组传输，布局帮助接收端还原行列关系。

### 3. 适合什么场景

```text
快速原型
没有强业务语义的一维数值序列
读懂已有项目中的旧接口
```

### 4. 为什么复杂状态不建议直接用数组

假设：

```python
msg.data = [1.0, 90.0, 0.0, 2.5]
```

接收者必须记住：

```text
第 0 项是机器人编号
第 1 项是电量
第 2 项是运行状态
第 3 项是速度
```

字段顺序一改，所有接收端都可能误解。自定义消息可直接写：

```text
int32 robot_id
float32 battery
bool running
float32 speed
```

### 教师说明

ROS 2 中 std_msgs 的 MultiArray 系列已标为弃用。新接口优先采用有明确语义的消息；图像和点云先查 `sensor_msgs/Image`、`PointCloud2` 等专用接口。

PPT 的“超过 5 个话题必须自定义”和“MultiArray 一定更慢”不是 ROS 规范。应根据数据关系、语义和实际测试判断。

### 补充命令行对照实验

目的：给 2×3 数组填 layout，并证明尺寸语义仍由应用检查。A 持续发布，B 接收：
```bash
# A
ros2 topic pub /lesson1/matrix std_msgs/msg/Float64MultiArray   "{layout: {dim: [{label: rows, size: 2, stride: 6}, {label: cols, size: 3, stride: 3}], data_offset: 0}, data: [1.0, 2.0, 3.0, 4.0, 5.0, 6.0]}" --rate 1
# B
ros2 topic echo /lesson1/matrix --once
```
预期数组仍是平铺六项，layout 描述两行三列；不是 XML 或 YAML 自动变二维矩阵。A 改 data 只保留五项仍可能构成合法消息，但布局与长度语义不一致，应由接收应用校验。本类型已弃用，此实验用于读懂旧接口，不作为新接口推荐。结束 A。

这页最后可以总结：

> **MultiArray 能表达数组形状，却不天然表达业务含义；数据字段有明确名称时，自定义消息通常更清楚。**

---

## 第 23 页｜Header 与时间类型

这页你可以把它讲成：

> **机器人数据只有数值往往不够，还必须回答两个问题：这份数据是什么时候采集的？它属于哪个坐标系？Header 就是在消息里携带这两个关键信息。**

先看定义：

```bash
ros2 interface show std_msgs/msg/Header
```

核心结构：

```text
builtin_interfaces/Time stamp
string frame_id
```

### 1. stamp：数据对应的时间

`Time` 包含：

```text
int32 sec
uint32 nanosec
```

Python 中常见写法：

```python
msg.header.stamp = self.get_clock().now().to_msg()
```

但传感器数据最好使用真正的采样时刻，而不是经过大量处理以后才随手写一个发布时间。

为什么重要？

```text
相机与雷达数据融合
定位和里程计对齐
计算消息延迟
回放 rosbag2 数据
```

都依赖正确时间。

### 2. frame_id：数据属于哪个坐标系

例如：

```text
laser_frame
camera_link
base_link
odom
map
```

同一个障碍物点，在 `laser_frame` 和 `map` 中的坐标数字可能完全不同。`frame_id` 就是在说明“这组坐标应在哪个参考系下解释”。

### 3. Time 与 Duration

```text
Time     → 一个具体时刻
Duration → 一段时间长度
```

例如：

```text
10:30:05.200  → Time
持续 0.5 秒   → Duration
```

### 4. 仿真时间

使用：

```python
self.get_clock().now()
```

可以跟随 ROS 时钟配置。启用 `use_sim_time` 后，节点可以使用仿真系统发布的时钟。直接用 Python `time.time()` 不会自动遵循 ROS 仿真时间。

### 教师说明

把 Header 放在物理世界相关消息的首字段，是常见设计建议，不是所有消息的语法强制。只有确实需要时间或坐标语义时才加入。

引用 `std_msgs/Header` 需要声明 `std_msgs` 依赖；如果直接使用 `builtin_interfaces/Time`，则要声明相应依赖。

这页最后可以总结：

> **stamp 告诉我们数据发生在什么时候，frame_id 告诉我们数据在哪个坐标系中，二者共同为传感器融合和坐标变换提供依据。**

---

## 第 24 页｜特殊消息与强类型机制

这页你可以把它讲成：

> **除了普通数值，std_msgs 还提供颜色和空消息等特殊接口。更重要的是，ROS 2 会根据接口定义生成带类型的代码，让通信双方使用同一份数据契约。**

### 1. ColorRGBA

查看定义：

```bash
ros2 interface show std_msgs/msg/ColorRGBA
```

结构：

```text
float32 r
float32 g
float32 b
float32 a
```

通常按 0.0～1.0 使用：

```python
color.r = 1.0
color.g = 0.0
color.b = 0.0
color.a = 1.0
```

表示完全不透明的红色。

这里要说明：0～1 是接口使用约定，生成的消息类不一定自动把超范围数值拦住。

### 2. Empty

```bash
ros2 interface show std_msgs/msg/Empty
```

它没有数据字段，只表达“事件发生了”。

例如：

```text
触发一次重置
通知一次开始录制
发送一个简单心跳事件
```

还要区分：

```text
std_msgs/msg/Empty  → 话题消息
std_srvs/srv/Empty  → 空请求、空响应的服务
```

### 3. 强类型生成流程

可以画成：

```text
.msg 接口定义
      ↓
转换与代码生成
      ↓
C++ 消息类型 + Python 消息类型
      ↓
发布者与订阅者使用同一接口
```

如果发布者声明：

```text
std_msgs/msg/Float32
```

订阅者却声明：

```text
std_msgs/msg/String
```

它们不会把同一段字节随意解释成不同结构，而是按照类型系统分别匹配。

### 4. 跨语言

Python：

```python
from std_msgs.msg import Float32
msg = Float32()
msg.data = 3.14
```

C++：

```cpp
#include "std_msgs/msg/float32.hpp"
std_msgs::msg::Float32 msg;
msg.data = 3.14f;
```

写法不同，接口相同，因此可以跨语言通信。

### 教师说明

“编译期类型检查”在 C++ 中最明显。Python 的消息类也有限定字段，但很多错误在赋值或运行时才出现。类型系统能保证结构一致，却不能判断“电量是否应该大于 100”这类业务规则。

这页最后可以总结：

> **消息接口像一份数据合同，代码生成让不同语言遵守同一结构；结构一致不代表业务数值一定合理。**

---

## 第 25 页｜std_msgs 的适用边界与设计决策

这页你可以把它讲成：

> **选择消息类型时，不要只问“能不能把数据塞进去”，还要问“接收者能不能只看接口就理解数据含义”。**

### 1. 适合使用 std_msgs 的情况

如果只传一个简单值：

```text
开关状态         → Bool
一条文本提示     → String
一个临时测试数值 → Float32 或 Float64
```

这种场景字段少、语义简单，使用标准消息很方便。

### 2. 快速原型

项目刚开始时，可以先用：

```bash
ros2 topic pub --once /temperature \
  std_msgs/msg/Float32 \
  "{data: 25.5}"
```

快速验证话题链路。

但原型稳定后，如果需要时间、单位、传感器编号和有效状态，就要重新评估接口。

### 3. 不适合拆成多个散装话题的情况

假设机器人状态包含：

```text
robot_id
battery_pct
running
task_id
```

如果拆成四个话题：

```text
/robot_id
/battery
/running
/task_id
```

会遇到：

```text
四条消息到达时间可能不同
接收者难以判断它们是否属于同一状态快照
话题数量增加
字段关系藏在业务代码里
```

更清楚的方式是一个自定义 `RobotStatus`。

### 4. 先找已有专用接口

不要一看到复杂数据就立刻自定义。

先检查 ROS 2 已有接口：

```text
位置、速度、姿态  → geometry_msgs
图像、雷达、IMU   → sensor_msgs
里程计、路径、地图 → nav_msgs
可视化标记        → visualization_msgs
```

已有标准接口通常更容易接入 RViz、导航和其他生态工具。

### 5. 四个判断问题

可以让学生逐个回答：

```text
是否只有一个简单字段？
数据是否具有明确业务语义？
是否已有标准消息可以表达？
这些字段是否必须作为同一时刻的整体一起传输？
```

如果多个字段强关联，而且没有合适的现成接口，就应该考虑自定义消息。

### 教师说明

PPT 提到“关联话题超过 5 个就必须自定义”，只能作为提醒，不能当成 ROS 规范。两个话题也可能应当合并，十个话题也可能确实代表十种独立数据。关键是字段之间有没有共同生命周期和整体语义。

这页最后可以总结：

> **简单单值可以用 std_msgs；已有行业接口先复用；多个强关联字段需要一起表达时，再设计自定义消息。**

---

## 第 26 页｜第三章：自定义 msg 与编译配置

这是一张章节过渡页，可以这样讲：

> **第二章已经看到，单个 `data` 字段有时表达不了完整的机器人状态。第三章我们就从零设计一个 `RobotStatus`，并让 ROS 2 真正识别和使用它。**

先提出需求：

```text
机器人编号
运行状态
电量百分比
是否正在充电
二维位置
航向角
采样时间与坐标系
```

如果都塞进多个 Float32 或 MultiArray，接收端很难读懂。因此设计：

```text
RobotStatus.msg
```

后面按照 PPT 顺序完成：

```text
第 27 页  文件与包规范
第 28 页  可用数据类型
第 29 页  完整消息定义
第 30 页  编译生成原理
第 31 页  五步开发流程
第 32 页  package.xml 与 CMakeLists.txt
第 33 页  编译、验证与调用
```

这里可以先告诉学生最终目标：

```python
from robot_interfaces.msg import RobotStatus

msg = RobotStatus()
msg.battery_pct = 90.0
```

现在这段代码还不能凭空运行，因为 `RobotStatus` 必须先经过接口生成。

这页最后可以说：

> **接下来不是只写一个文本文件，而是完整走通“定义接口、生成代码、安装类型、节点调用”的链路。**

---

## 第 27 页｜自定义消息开发规范

这页你可以把它讲成：

> **自定义消息首先要放在正确的包和目录中。ROS 2 构建工具只有按照约定找到 `.msg`，才能为它生成各语言代码。**

### 1. 文件后缀与格式

消息文件使用：

```text
.msg
```

内容是一行一个字段：

```text
类型 字段名
```

例如：

```text
int32 robot_id
float32 battery
bool running
```

### 2. 存放目录

标准目录结构：

```text
robot_interfaces/
├── CMakeLists.txt
├── package.xml
└── msg/
    └── RobotStatus.msg
```

构建工具会根据 `CMakeLists.txt` 中登记的路径处理消息文件。

### 3. 文件命名

消息文件名使用 PascalCase：

```text
RobotStatus.msg
BatteryState.msg
MotorCommand.msg
```

字段名使用 snake_case：

```text
battery_pct
is_charging
task_id
```

### 4. 为什么接口包使用 ament_cmake

创建包：

```bash
mkdir -p ../ros2_ws/src
if [ ! -d ../ros2_ws/src/robot_interfaces ]; then
  ros2 pkg create --destination-directory ../ros2_ws/src --build-type ament_cmake robot_interfaces
fi
```

因为消息生成需要调用 CMake 中的：

```cmake
rosidl_generate_interfaces(...)
```

Python 节点仍然可以使用生成的消息。通常把接口和业务拆成两个包：

```text
ros2_ws/src/
├── robot_interfaces/  → ament_cmake，定义并生成消息
└── robot_control/     → ament_python，编写 Python 节点
```

### 5. 接口包与业务包分开的好处

多个包可以依赖同一份接口：

```text
robot_interfaces
   ├── 被感知节点使用
   ├── 被控制节点使用
   └── 被监控节点使用
```

消息定义的位置更清楚，也能避免每个业务包重复定义相似结构。

### 教师说明

接口包与业务包分离是推荐组织方式，不是说消息字段改变以后业务代码完全不受影响。字段名、类型或结构发生变化时，依赖它的发布端和订阅端往往也要修改并重新构建。

这页最后可以总结：

> **`.msg` 放在 ament_cmake 接口包的 `msg/` 目录，文件用 PascalCase，字段用 snake_case，再通过构建配置登记。**

---

## 第 28 页｜msg 文件支持的数据类型

这页你可以把它讲成：

> **自定义消息不是只能写几个整数。字段可以是基础类型、其他包的消息、数组，也可以继续嵌套自定义消息。**

### 1. 基础类型

常用类型：

```text
bool
int8 / int16 / int32 / int64
uint8 / uint16 / uint32 / uint64
float32 / float64
string
```

示例：

```text
uint32 robot_id
float32 battery_pct
bool is_charging
string mode_name
```

### 2. 引用其他消息

例如加入 Header：

```text
std_msgs/Header header
```

如果使用外部包的类型，就要在：

```text
package.xml
CMakeLists.txt
```

同时声明依赖。

### 3. 可变长数组

```text
float64[] samples
```

每条消息中的元素数量可以不同。

Python：

```python
msg.samples = [1.0, 2.0, 3.0, 4.0]
```

### 4. 定长数组

```text
float64[2] position
```

表示固定有两个元素，例如平面坐标：

```text
position[0] = x
position[1] = y
```

定长数组能直接表达维度约束，但不要简单承诺“任何语言都会在编译期检查所有越界问题”或“性能一定更快”。实际行为取决于生成语言和使用方式。

### 5. 嵌套自定义消息

假设已经定义：

```text
MotorState.msg
```

另一个消息可以引用：

```text
robot_interfaces/MotorState left_motor
robot_interfaces/MotorState right_motor
```

这样可以把复杂接口拆成可复用结构。

### 课堂练习

让学生判断写法：

```text
bool running
float32 battery_pct
float64[2] position
std_msgs/Header header
```

分别属于基础类型、定长数组还是外部消息。

### 教师说明

Header 常用于需要时间和坐标语义的数据，但不是所有自定义消息都必须加入。先确认业务是否真的需要。

这页最后可以总结：

> **基础类型描述字段，数组描述多个同类值，外部消息和嵌套消息帮助复用已有结构。**

---

## 第 29 页｜编写规范与 RobotStatus 示例

这页你可以把它讲成：

> **一个好的 `.msg` 不只是能通过编译，还应该让使用者一眼看懂字段含义、单位、范围和状态码。接口本身就是文档的一部分。**

### 1. 先写消息用途

```text
# 机器人状态，约定以 1 Hz 发布
```

告诉使用者这是什么消息，以及预计怎样使用。

### 2. Header 放在前面

```text
std_msgs/Header header
```

它为状态提供时间戳与参考坐标系。

### 3. 使用常量表达状态

```text
uint8 STATUS_IDLE=0
uint8 STATUS_RUN=1
uint8 status
```

这样业务代码不需要到处写没有含义的数字：

```python
msg.status = RobotStatus.STATUS_RUN
```

比下面更容易理解：

```python
msg.status = 1
```

### 4. 字段带单位和范围

完整示例：

```text
# 机器人状态，约定以 1 Hz 发布
std_msgs/Header header

# 运行状态
uint8 STATUS_IDLE=0
uint8 STATUS_RUN=1
uint8 status

# 电量百分比，范围约定为 [0.0, 100.0]
float32 battery_pct

# 是否正在充电
bool is_charging

# 平面位置 [x, y]，单位：m
float64[2] position

# 航向角，单位：rad
float64 yaw
```

PPT 画面中 `std_msgs/Headerheader`、`uint8status` 是排版时丢了空格。真正的 `.msg` 必须写成：

```text
std_msgs/Header header
uint8 status
```

### 5. 怎样检查接口是否自解释

让学生只看定义，回答：

```text
battery_pct 的单位和范围是什么？
position 两个元素分别是什么？
yaw 使用度还是弧度？
status=1 表示什么？
```

如果接口或注释无法回答，说明文档还不清楚。

### 6. 为什么不直接用多个 Float32

自定义消息一次传递同一个状态快照：

```text
时间戳
运行状态
电量
位置
航向
```

字段名称也让接收端代码更容易阅读。

这页最后可以总结：

> **类型决定能装什么，字段名说明数据含义，注释补充单位与范围，常量消除业务代码中的魔法数字。**

---

## 第 30 页｜消息编译生成原理

这页你可以把它讲成：

> **我们写的 `.msg` 文件不是 Python 或 C++ 能直接使用的代码，它更像一张“数据结构设计图”。ROS 2 编译时会把这张设计图自动转换成 Python、C++ 都能调用的消息类。**

整条链路可以先画成：

```text
RobotStatus.msg
      ↓
解析消息定义
      ↓
转换成 IDL
      ↓
rosidl 生成代码
   ↙        ↘
C++        Python
      ↓
编译 / 安装
      ↓
节点可以 import / include
```

### 1. 第一步：解析 `.msg`

假设写了：

```text
int32 robot_id
float32 battery
bool running
```

构建时先检查：

```text
字段类型是否支持
字段名称是否合法
引用的其他消息是否存在
语法是否正确
```

可以类比 C 结构体：

```c
struct RobotStatus
{
    int robot_id;
    float battery;
    bool running;
};
```

这一阶段在理解和检查结构，还没有运行节点。

课堂上可以查看文件：

```bash
cat ../ros2_ws/src/robot_interfaces/msg/RobotStatus.msg
```

### 2. 第二步：转换成 IDL

IDL 可以解释为：

> **一种不绑定 Python 或 C++ 的接口说明语言。**

因为 ROS 2 需要同时连接：

```text
Python
C++
DDS 类型系统
其他受支持语言
```

所以先把 ROS 的消息定义转换成中立的接口描述：

```text
ROS 2 的 .msg 写法
        ↓
标准化的中间接口描述
        ↓
IDL
```

初学阶段不用展开 OMG IDL 语法，只要记住：

> **IDL 是 ROS 2 接口定义与 DDS 类型系统之间的重要桥梁。**

### 3. 第三步：生成不同语言代码

只定义一次：

```text
RobotStatus.msg
```

Python 可以这样使用：

```python
from robot_interfaces.msg import RobotStatus

msg = RobotStatus()
msg.status = RobotStatus.STATUS_RUN
msg.battery_pct = 85.0
msg.is_charging = False
```

C++ 可以这样使用：

```cpp
#include "robot_interfaces/msg/robot_status.hpp"

robot_interfaces::msg::RobotStatus msg;
msg.status = robot_interfaces::msg::RobotStatus::STATUS_RUN;
msg.battery_pct = 85.0f;
msg.is_charging = false;
```

两种语言写法不同，使用的是同一个接口定义，因此可以相互通信。

可以给学生一个比喻：

> `.msg` 是产品图纸，Python 和 C++ 是使用不同工具的工厂。ROS 2 根据同一张图纸，生成适合不同语言使用的零件。

### 4. 第四步：编译和安装

回到工作空间：

使用 rosdep 前，先初始化并更新规则；已初始化时跳过 init。

编译接口包：

```bash
(cd ../ros2_ws && colcon build --packages-select robot_interfaces)
```

编译后工作空间常见目录：

```text
src/      我们编写的源码和接口定义
build/    构建过程的中间内容
install/  安装后的包、语言绑定与类型支持
log/      构建日志
```

重点是 `install/`。Python 模块、C++ 头文件和类型支持等最终会进入安装空间。

### 5. 为什么编译后要 source

```bash
source ../ros2_ws/install/setup.bash
```

可以先问学生：

> 消息已经生成了，当前终端怎样知道它放在哪里？

`source` 会把这个工作空间加入当前 Shell 的 ROS 环境，让工具与程序找到新包和新接口。

执行后：

```bash
ros2 interface show robot_interfaces/msg/RobotStatus
```

才能从当前环境查到定义。

如果没有 source，可能看到：

```text
Package 'robot_interfaces' not found
```

或消息类型无法解析。问题可能不是没编译，而是当前终端还没有加载工作空间。

### 6. 为什么每个新终端都要 source

终端 A 执行：

```bash
if [ -f ../ros2_ws/install/setup.bash ]; then
  source ../ros2_ws/install/setup.bash
fi
```

只改变终端 A 的环境。重新打开终端 B，它不会自动继承终端 A 后来执行的命令，所以终端 B 也要 source。

可以类比：

> `source` 像给当前终端发一张新的包路径地图。新开的终端没有拿到这张地图，就不知道自定义消息在哪里。

可以按需要把工作空间加入 `~/.bashrc`，但课堂环境中建议先手动 source，让学生看清环境叠加过程。

### 7. 为什么接口包通常使用 ament_cmake

接口生成通过 CMake 配置调用：

```cmake
rosidl_generate_interfaces(...)
```

推荐结构：

```text
robot_interfaces → ament_cmake，负责接口生成
robot_control    → ament_python，负责 Python 节点
```

这不代表 Python 不能使用自定义消息。Python 业务包只需要依赖已经生成好的接口包。

### 这页最后可以用一句话总结

> **`.msg` 是消息设计图，rosidl 把它变成各语言能够使用的代码，colcon 完成构建和安装，source 再让当前终端找到这些新接口。**

最好记的口诀：

> **先写 `.msg`，再生成接口；编译放进 `install`，source 以后才能找到。**

---

## 第 31 页｜全流程开发步骤

这页你可以把它讲成：

> **上一页理解了原理，这一页把它变成可执行的五个步骤：建包、写消息、配构建、编译环境、验证调用。**

### 第一步：创建接口包

```bash
mkdir -p ../ros2_ws/src
if [ ! -d ../ros2_ws/src/robot_interfaces ]; then
  ros2 pkg create --destination-directory ../ros2_ws/src --build-type ament_cmake robot_interfaces
fi
```

创建完成后应看到：

```text
robot_interfaces/
├── CMakeLists.txt
├── package.xml
├── include/
└── src/
```

如果包已经存在，不要重复创建。

### 第二步：创建 msg 文件

```bash
mkdir -p ../ros2_ws/src/robot_interfaces/msg
```

创建：

```text
msg/RobotStatus.msg
```

课前准备已复制完整定义；若需重新准备，执行 `python3 prepare_course.py 1`。修改接口时，完成下一页的构建配置再编译。

### 第三步：配置构建文件

修改：

```text
package.xml
CMakeLists.txt
```

告诉构建系统：

```text
需要 rosidl 生成器
消息依赖 std_msgs
需要生成 RobotStatus.msg
```

课前准备已提供完整构建文件，下一页逐行讲解。若自行手写，必须先完成下一页的配置，再执行下面的构建。

### 第四步：编译并加载环境

回到工作空间根目录：

```bash
(cd ../ros2_ws && colcon build --packages-select robot_interfaces)
```

成功后：

```bash
source ../ros2_ws/install/setup.bash
```

### 第五步：验证接口

```bash
ros2 interface show robot_interfaces/msg/RobotStatus
```

再查是否登记：

```bash
ros2 interface list | grep robot_interfaces
```

### 常见错误怎样定位

找不到包：

```text
是否编译成功？
当前终端是否 source？
包名是否拼对？
```

找不到依赖类型：

```text
package.xml 是否声明依赖？
CMakeLists.txt 是否 find_package？
rosidl_generate_interfaces 是否写 DEPENDENCIES？
```

编译成功但没有接口：

```text
消息文件是否列入 rosidl_generate_interfaces？
文件名大小写是否完全一致？
```

这页最后可以总结：

> **接口开发始终沿着五步走：创建包、编写 msg、配置依赖、编译并 source、最后用 interface 命令验证。**

---

## 第 32 页｜package.xml 与 CMakeLists.txt

这页你可以把它讲成：

> **`package.xml` 负责声明“这个包是什么、依赖谁”；`CMakeLists.txt` 负责安排“构建时查找什么、生成哪些接口”。两份文件缺一不可。**

### 1. package.xml 的作用

可以把它理解成包的身份证和依赖清单。

在 `ros2 pkg create` 生成的完整文件中，保留包名、版本、维护者、许可证和 `<export>`，并加入关键依赖：

```xml
<buildtool_depend>ament_cmake</buildtool_depend>

<buildtool_depend>rosidl_default_generators</buildtool_depend>
<exec_depend>rosidl_default_runtime</exec_depend>

<depend>std_msgs</depend>

<member_of_group>rosidl_interface_packages</member_of_group>
```

逐项解释：

```text
ament_cmake
→ 这个包使用 ament_cmake 构建

rosidl_default_generators
→ 构建阶段生成不同语言接口代码

rosidl_default_runtime
→ 运行阶段使用生成的接口支持

std_msgs
→ RobotStatus.msg 引用了 std_msgs/Header

rosidl_interface_packages
→ 声明这个包属于 ROS 接口包组
```

### 2. CMakeLists.txt 的作用

完整的关键部分：

```cmake
cmake_minimum_required(VERSION 3.8)
project(robot_interfaces)

find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(std_msgs REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/RobotStatus.msg"
  DEPENDENCIES std_msgs
)

ament_package()
```

### 3. find_package

```cmake
find_package(std_msgs REQUIRED)
```

表示构建时必须找到 `std_msgs`。

如果 `.msg` 还引用 `geometry_msgs/Pose`，就需要额外添加：

```cmake
find_package(geometry_msgs REQUIRED)
```

### 4. rosidl_generate_interfaces

```cmake
rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/RobotStatus.msg"
  DEPENDENCIES std_msgs
)
```

这里明确告诉构建系统：

```text
为当前包生成接口
处理 msg/RobotStatus.msg
消息中依赖 std_msgs
```

以后增加：

```text
msg/MotorState.msg
```

也要加入列表：

```cmake
rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/RobotStatus.msg"
  "msg/MotorState.msg"
  DEPENDENCIES std_msgs
)
```

### 5. ament_package 的位置

```cmake
ament_package()
```

通常放在 `CMakeLists.txt` 末尾，完成 ament 包配置。

### PPT 代码需要补充的地方

PPT 片段使用了 `std_msgs`，因此应当有：

```cmake
find_package(std_msgs REQUIRED)
```

PPT 还列出了 `geometry_msgs`，但当前 RobotStatus 示例没有使用它，所以本例不必机械添加。只有消息定义真正引用它时才增加依赖。

### 课堂排错

如果报错类似：

```text
Could not find a package configuration file provided by "std_msgs"
```

检查 ROS 基础环境是否 source，以及 `find_package`、依赖声明是否正确。

如果提示找不到某个消息类型，检查：

```text
.msg 中包名/类型名是否正确
package.xml 是否声明
CMake 是否 find_package
DEPENDENCIES 是否列出
```

这页最后可以总结：

> **package.xml 声明依赖关系，CMakeLists.txt 执行接口生成；消息引用了哪个外部包，两边都要配完整。**

---

## 第 33 页｜验证方法与 Python 调用

这页你可以把它讲成：

> **编译没有报错还不算结束。我们要先验证接口是否被 ROS 2 发现，再用命令行测试通信，最后才放进 Python 节点。**

### 1. 编译接口包

```bash
(cd ../ros2_ws && colcon build --packages-select robot_interfaces)
```

看到类似：

```text
Summary: 1 package finished
```

说明构建完成。

### 2. 加载工作空间

```bash
source ../ros2_ws/install/setup.bash
```

每一个用于测试的终端都要加载：

```bash
source /opt/ros/humble/setup.bash
source ../ros2_ws/install/setup.bash
```

### 3. 查看接口定义

```bash
ros2 interface show robot_interfaces/msg/RobotStatus
```

预期能看到 Header、常量和全部字段。

确认接口已登记：

```bash
ros2 interface list | grep robot_interfaces
```

### 4. 命令行发布和订阅

终端 B 先运行：

```bash
ros2 topic echo /status
```

终端 A 发布：

```bash
ros2 topic pub --once /status \
  robot_interfaces/msg/RobotStatus \
  "{header: {frame_id: 'map'}, status: 1, battery_pct: 90.0, is_charging: false, position: [1.0, 2.0], yaw: 0.5}"
```

这条命令主要验证消息格式与链路，`stamp` 没有填写有效采样时间。真实节点应设置时间戳。

终端 B 应看到类似：

```yaml
header:
  stamp:
    sec: 0
    nanosec: 0
  frame_id: map
status: 1
battery_pct: 90.0
is_charging: false
position:
- 1.0
- 2.0
yaw: 0.5
---
```

### 5. Python 发布端

```python
from robot_interfaces.msg import RobotStatus

msg = RobotStatus()
msg.header.stamp = self.get_clock().now().to_msg()
msg.header.frame_id = 'map'
msg.status = RobotStatus.STATUS_RUN
msg.battery_pct = 90.0
msg.is_charging = False
msg.position = [1.0, 2.0]
msg.yaw = 0.5

self.publisher.publish(msg)
```

### 6. Python 订阅回调

```python
def status_callback(self, msg):
    self.get_logger().info(
        f'status={msg.status}, '
        f'battery={msg.battery_pct:.1f}%, '
        f'position={list(msg.position)}'
    )
```

### 7. 完整最小节点

```python
import rclpy
from rclpy.node import Node
from robot_interfaces.msg import RobotStatus


class StatusDemo(Node):
    def __init__(self):
        super().__init__('status_demo')

        self.publisher = self.create_publisher(
            RobotStatus, '/status', 10)

        self.subscription = self.create_subscription(
            RobotStatus, '/status', self.status_callback, 10)

        self.timer = self.create_timer(1.0, self.publish_status)

    def publish_status(self):
        msg = RobotStatus()
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.header.frame_id = 'map'
        msg.status = RobotStatus.STATUS_RUN
        msg.battery_pct = 90.0
        msg.is_charging = False
        msg.position = [1.0, 2.0]
        msg.yaw = 0.5
        self.publisher.publish(msg)

    def status_callback(self, msg):
        self.get_logger().info(
            f'status={msg.status}, battery={msg.battery_pct:.1f}%')


def main():
    rclpy.init()
    node = StatusDemo()
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
```

启动配套完整程序（终端 A；先停止命令行 /status 发布者）：

```bash
python3 第一课/课堂实验/status_demo.py
```

### 8. PPT 示例需要统一的地方

PPT 第 33 页出现：

```text
STATUS_RUNNING
battery_level
STATUS_ERROR
```

但第 29 页定义使用的是：

```text
STATUS_RUN
battery_pct
```

消息中也没有 `STATUS_ERROR`。课堂必须使用同一份定义，否则 Python 会报属性不存在。当前讲义统一使用第 29 页版本。

### 验收标准

```text
interface show 能显示完整字段
topic echo 能收到命令行消息
Python 能成功 import RobotStatus
发布后回调能打印状态与电量
```

### 补充命令行对照实验

目的：把接口可用、类型检查和运行数据分开。已完成 robot_interfaces 构建并在所有终端 source；先停止自建 StatusDemo，避免多发布者。
```bash
ros2 interface show robot_interfaces/msg/RobotStatus
python3 -c "from robot_interfaces.msg import RobotStatus; m=RobotStatus(); print(m)"
```
再按本页先启动指定类型 echo，单次发布正确字段。将 battery_pct 故意写成 battery_level，预期 CLI 字段填充报错；改回后通信恢复。名称必须遵从本课接口，不照抄 PPT 中另一套字段。若把 stamp 留为零，仅证明结构和链路，不证明采样时间正确。

这页最后可以总结：

> **先用 interface 命令证明类型存在，再用 topic 命令证明链路能通，最后才把消息放进业务代码。**

---

## 第 34 页｜课程总结与学习路径

这页你可以把它讲成：

> **今天的 34 页内容，最终可以收成三个问题：消息怎样找到对方，消息用什么结构表达，我们怎样定义并生成自己的消息。**

### 1. 第一章：话题通信机制

学生应该能说清：

```text
发布订阅为什么能降低节点耦合
ROS 2 通信栈中 rcl、rmw、DDS 的位置
PDP 和 EDP 分别发现什么
Publisher、Subscription、Spin 怎样配合
```

出现“有话题无数据”时，按：

```text
list
 ↓
info --verbose
 ↓
type / QoS
 ↓
echo
 ↓
网络与业务逻辑
```

逐步排查。

### 2. QoS

让学生解释：

```text
History / Depth  → 怎样缓存
Reliability      → 是否要求可靠交付
Durability       → 是否保留历史给迟加入者
Deadline         → 更新间隔要求
Lifespan         → 消息有效期
Liveliness       → 发布者活性
```

还要记住：

> **QoS 看的是业务需求和兼容关系，没有一个配置适合所有话题。**

### 3. 标准消息

学生应该知道：

```text
简单单值可以使用 std_msgs
时间和坐标语义常用 Header
图像、点云、位姿先查已有专用接口
强关联字段适合放入自定义消息
```

### 4. 自定义消息

完整链路：

```text
设计字段
   ↓
编写 .msg
   ↓
配置 package.xml 与 CMakeLists.txt
   ↓
colcon build
   ↓
source install/setup.bash
   ↓
interface show
   ↓
Python / C++ 调用
```

### 5. 回到第一页面的问题

现在再问：

> 话题能看见，却没有数据，可能是什么原因？

学生至少应答出：

```text
没有真正发布
话题名或类型不一致
QoS 不兼容
只发布过一次，订阅者加入太晚
Domain 或网络发现配置不同
订阅节点没有 spin
```

### 课后练习

给 `RobotStatus.msg` 增加：

```text
uint32 task_id
```

然后完成：

```text
重新编译接口包
source 工作空间
用 interface show 检查字段
修改 Python 发布端赋值
修改订阅回调打印 task_id
```

学生会亲眼看到：消息接口变化以后，使用它的业务代码也要同步调整。

### 这门课最后一句话

> **先理解通信，再选择接口；标准消息够用就复用，不够用就清楚地定义；遇到问题按层排查，不靠猜。**

---

## 教师核对资料

使用 rosdep 前，先初始化并更新规则；已初始化时跳过 init。


- [ROS 2 Humble：QoS 设置与兼容矩阵](https://github.com/ros2/ros2_documentation/blob/humble/source/Concepts/Intermediate/About-Quality-of-Service-Settings.rst)
- [ROS 2 Humble：创建自定义 msg/srv](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Beginner-Client-Libraries/Custom-ROS2-Interfaces.rst)
- [ROS 2 common_interfaces 源码](https://github.com/ros2/common_interfaces)
- [ROS 2 Humble：std_msgs 消息清单](https://docs.ros.org/en/humble/p/std_msgs/__message_definitions.html)
