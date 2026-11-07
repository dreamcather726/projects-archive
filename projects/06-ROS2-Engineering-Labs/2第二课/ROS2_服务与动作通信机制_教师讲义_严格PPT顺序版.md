# ROS 2 服务与动作通信机制：逐页教师讲义

对应课件：`2026.8.3 ROS2 服务与动作通信机制学习手册.pptx`，共 30 页。

环境：WSL Ubuntu 22.04、ROS 2 Humble。承接第一课的话题通信、QoS 和自定义消息。

这份讲义严格按照 PPT 页码展开。每页都写成可以直接在课堂上讲的口语稿，并配有类比、板书、提问、命令和收尾总结。PPT 中容易引起误解的说法，在原页位置给出教师说明。第 22 页的全景对比保留在原位置，不移到第三部分。

## 上课前准备

### 执行目录与课前准备

**本课所有命令均从整套讲义的根目录执行**，也就是同时能看到 `第一课`、`第二课`、`第三课` 等文件夹的位置。请先在 WSL Ubuntu 的 Bash 中进入此目录；每个新终端都保持在该目录。若终端已位于 `第二课`，先执行 `cd ..` 回到讲义根目录。

Python 程序直接使用已提供的 `第二课/课堂实验/` 文件，无需复制到另一份脚本目录。第三课配置直接使用 `第三课/课堂实验/config/`。只有需要 colcon 构建的 ROS 包才由准备脚本写入旁边的 `../ros2_ws/src/`，构建在子 Shell 中进行，完成后当前终端仍位于讲义根目录。

```bash
source /opt/ros/humble/setup.bash
test -d "第二课/课堂实验" || { echo "请先进入整套讲义根目录"; exit 1; }
mkdir -p tf_debug
sudo apt update && sudo apt install -y ros-humble-example-interfaces ros-humble-action-msgs ros-humble-std-srvs ros-humble-std-msgs ros-humble-rosidl-default-generators python3-colcon-common-extensions python3-rosdep && \
python3 prepare_course.py 2 --workspace ../ros2_ws && \
(cd ../ros2_ws && colcon build --packages-select robot_interfaces --symlink-install) && \
source ../ros2_ws/install/setup.bash
```

节点持续运行时，后面的查询命令应在另一个终端执行。各终端使用相同 ROS_DOMAIN_ID，先停止旧实验再启动新实验。

建议准备三个 WSL 终端：

```text
终端 A：运行服务端或动作服务端
终端 B：运行客户端或发送目标
终端 C：查看节点、服务、动作和隐藏话题
```

每个新终端先执行：

```bash
source /opt/ros/humble/setup.bash
echo "$ROS_DISTRO"
echo "${ROS_DOMAIN_ID:-0}"
```

如果已有工作空间，再执行：

```bash
if [ -f ../ros2_ws/install/setup.bash ]; then
  source ../ros2_ws/install/setup.bash
fi
```

课堂命令和 Python 脚本都在 WSL 的 Ubuntu 终端中执行。Windows PowerShell 不直接执行这些 Bash 命令。

保存本课演示脚本前，先创建目录：

```bash
mkdir -p tf_debug
```

本课贯穿使用的名称：

| 示例 | 通信名称 | 接口类型 | 用途 |
| --- | --- | --- | --- |
| 两数相加 | `/add_two_ints` | `example_interfaces/srv/AddTwoInts` | 先验证 Service 请求与响应 |
| 自定义机器人指令 | `/robot_command` | `robot_interfaces/srv/RobotCommand` | 练习 `.srv` 定义和编译 |
| 分步计数任务 | `/count_steps` | `robot_interfaces/action/CountSteps` | 演示目标、反馈、结果与取消 |

前一个服务示例直接使用现成接口，后两个接口在本课中定义。`CountSteps` 只是软件计数模拟，不控制真实机器人。

先检查现成接口是否可用：

```bash
ros2 interface show example_interfaces/srv/AddTwoInts
```

预期看到：

```text
int64 a
int64 b
---
int64 sum
```

如果提示找不到包，先检查是否加载 Humble 环境。确实未安装时，可在课前安装：

```bash
sudo apt update
sudo apt install ros-humble-example-interfaces
```

教师在课前可按第 13、24 页准备自定义接口，按第 11、23 页保存演示脚本。课堂讲解仍然按页码顺序进行。

---

## 第 1 页｜ROS2 服务与动作通信机制学习手册

这页你可以把它讲成：

> **上一课我们解决了“数据怎样持续传给其他节点”。这一课换两个问题：我发出一个请求，怎样知道处理结果？我让机器人执行一个长任务，怎样查看进度，又怎样在中途叫停？**

先用同一台机器人串起三种场景：

```text
激光雷达持续发送扫描数据      → Topic
查询一次设备状态并取得结果    → Service
导航到门口，查看进度并可取消  → Action
```

类比可以这样说：

> 话题像收听广播，服务像到窗口办理一次业务，动作像委托快递配送：先受理订单，途中查看进度，最后确认送达，也可能申请取消。

不要急着讲 API，先问：

> 导航任务可能持续一分钟。如果只用一个请求和一个最终响应，我们还缺少哪些信息？

引导学生答出：当前进度、是否受理、中途取消、最终成功或失败。

板书：

```text
短请求：请求 → 处理 → 响应
长任务：目标 → 受理 → 执行与反馈 → 终态与结果
```

这页最后可以总结：

> **今天要学会给一次请求建立响应闭环，给一个长任务建立完整的生命周期。**

---

## 第 2 页｜课程目录

这页你可以把它讲成：

> **今天先讲 Service，再讲 Action，最后回到业务场景选择通信方式。Action 用到了服务和话题，所以先把服务讲清楚，后面的组合就容易理解。**

### 第一部分解决什么问题

```text
Client 与 Server 怎样配合？
请求与响应怎样一一对应？
同步与异步调用有什么区别？
为什么回调中等待响应会卡住？
.srv 怎样定义、编译和验证？
```

### 第二部分解决什么问题

```text
为什么长任务需要 Action？
Goal、Feedback、Result 分别装什么？
三个服务和两个话题怎样配合？
任务有哪些状态？
取消请求怎样变成真正的取消？
.action 怎样定义并使用？
```

### 第三部分解决什么问题

```text
怎样根据数据流、响应和任务生命周期选型？
导航、视觉、机械臂怎样组合使用三种机制？
怎样避免把所有通信都做成同一种接口？
```

教师可以说明：PPT 目录列出了较细的知识点，后面的页面会合并讲解；实际教学顺序以页码为准。

课堂提问：

> 如果一个服务客户端用了异步调用，这个接口会自动具备进度反馈和取消能力吗？

先保留答案，到第 17 页再回顾。

这页最后可以总结：

> **Service 讲一次问答，Action 讲一个任务，选型部分讲怎样把它们放进机器人系统。**

---

## 第 3 页｜第一部分：服务通信机制

这是一张章节过渡页，可以这样讲：

> **先从一个最简单的任务开始：客户端给出两个整数，服务端算出它们的和。这里没有持续数据流，也不需要上报执行进度，我们只需要知道这次请求的答案。**

板书：

```text
Client                         Server
  │──── Request：a=2, b=3 ──────→│
  │                              │ 计算
  │←──── Response：sum=5 ─────────│
```

问学生：

> 如果两个客户端同时发出请求，服务端返回的结果应该交给谁？

提示：系统必须关联“哪一个响应属于哪一个请求”。这也是后面不能把 Service 简单理解成两个普通话题的原因。

这页最后可以说：

> **接下来从角色、名称和类型开始，再看一次请求在底层怎样完成。**

---

## 第 4 页｜1.1 服务通信核心模型

这页你可以把它讲成：

> **Service 是 ROS 2 中的请求与响应接口。客户端主动提出问题，服务端执行处理，再返回这次请求对应的结果。它可以提供类似远程函数调用的使用体验。**

### 1. 请求与响应组成一次交互

以加法服务为例：

```text
请求：a、b
处理：计算 a+b
响应：sum
```

正常完成的一次调用对应一次响应。如果节点崩溃、网络故障或回调无法运行，客户端也可能一直收不到响应，应用需要管理等待时间。

### 2. 名称和类型共同约定接口

双方约定：

```text
服务名称：/add_two_ints
服务类型：example_interfaces/srv/AddTwoInts
```

可以类比办理业务的窗口：名称告诉你去哪个窗口，类型告诉你申请表和回执单上有哪些字段。

名称相同而类型不同，仍然不能正常按同一个接口调用。

### 3. 请求可以同步等待，也可以异步获取

Python 中常见：

```python
future = client.call_async(request)
```

它返回 Future，表示一个尚未完成的结果。具体等待方式在第 9 页展开。

### 教师说明

PPT 中“同步阻塞机制”描述的是一种客户端使用方式。Service 本身支持异步调用，课堂不能把“服务”与“同步阻塞”直接画等号。

PPT 中“建立临时链路，响应完成即释放”也不宜按 TCP 短连接讲解。服务端和客户端的通信实体通常持续存在，一次响应结束的是这次请求的交互，系统不需要每次销毁并重建所有端点。

课堂提问：

> 加法服务返回一次结果之后，服务端是不是就退出了？

答案：一般继续运行，等待下一次请求。

这页最后可以总结：

> **Service 用名称和类型约定一次问答，用请求关联机制把响应送回对应的客户端调用。**

---

## 第 5 页｜核心角色与通信要素

这页你可以把它讲成：

> **一个服务要工作起来，需要四个要素：Server 提供处理逻辑，Client 发出请求，名称确定调用对象，类型规定输入和输出。**

### 1. Server：服务端

服务端创建服务并绑定回调：

```python
self.service = self.create_service(
    AddTwoInts, '/add_two_ints', self.handle_request
)
```

回调接收请求，填写响应并返回：

```python
def handle_request(self, request, response):
    response.sum = request.a + request.b
    return response
```

一个 Node 可以创建多个服务对象，每个服务对象都有自己的名称、类型和回调。

### 2. Client：客户端

客户端针对一个名称和类型创建：

```python
self.client = self.create_client(AddTwoInts, '/add_two_ints')
```

需要调用另一个名称或类型时，通常再创建一个 Client 对象。一个节点可以持有多个 Client。

没有发现服务端时，可以等待服务可用，也可以按应用策略报告不可用；不能概括成“必定立即失败”。

### 3. Service Name：服务名称

名称可以使用命名空间组织：

```text
/robot_1/camera/set_enabled
/robot_2/camera/set_enabled
```

相对名称会结合节点命名空间展开，绝对名称以 `/` 开头。排查时看展开后的完整名称。

教师提醒：同名服务端可能被创建出来，但 ROS 2 服务不提供通用的同名负载均衡保证。设计中应让一个完整服务名对应明确的提供者，避免同名服务端造成不确定的处理行为。

### 4. Service Type：服务类型

```text
AddTwoInts.srv
├─ Request：int64 a、int64 b
└─ Response：int64 sum
```

Python 和 C++ 可以使用同一接口。C++ 类型使用受编译器检查，Python 的很多字段赋值问题在运行时暴露。

课堂提问：

> 一个节点提供“查询状态”和“设置模式”，是不是必须拆成两个节点？

答案：不必须，可以在同一个节点创建两个服务，是否拆分由职责和部署需求决定。

这页最后可以总结：

> **节点承载服务对象，服务对象绑定名称和类型，回调负责把请求变成响应。**

---

## 第 6 页｜完整通信流程与同步特性

这页你可以把它讲成：

> **我们按“启动、发现、发送、执行、响应”把一次服务调用走完。PPT 画的是同步等待场景，但前半段通信过程在异步调用中同样存在。**

板书：

```text
服务端启动并创建服务
        ↓
客户端发现服务可用
        ↓
客户端构造并发送 Request
        ↓
服务端 Executor 调度服务回调
        ↓
回调填写并返回 Response
        ↓
客户端 Executor 处理响应，完成 Future
        ↓
客户端使用结果
```

### 1. 等服务可用与等响应是两件事

```python
client.wait_for_service(timeout_sec=3.0)
```

它等待服务端可用，不保证下一次请求一定在三秒内完成。

```python
rclpy.spin_until_future_complete(node, future, timeout_sec=3.0)
```

它运行执行器并等待 Future，三秒后可能仍未得到结果；调用后需要检查 `future.done()`。

### 2. 同步等待占用当前调用路径

如果当前线程一直等响应，这个线程就无法继续执行其他业务。但节点能否处理别的回调，还取决于执行器线程和回调组配置。

可以类比窗口办事：排队等回执的人暂时不能去办下一件事，而其他窗口是否还能工作，要看有没有工作人员和资源。

### 3. 超时后结果可能仍然产生

板书：

```text
客户端停止等待
      ≠
服务端停止执行
```

例如客户端等了三秒就退出，但服务端四秒后才完成“保存文件”。文件仍可能保存成功。

### 教师说明

PPT 的“注册端口”“零 CPU 消耗”“超时自动抛异常”都不宜讲成所有实现的统一规则。ROS 2 通过中间件发现服务，不要求每个服务绑定一个独立固定端口；节点等待期间仍可能运行执行器；Humble 的 `rclpy.Client.call()` 没有这里假定的自动业务超时参数。

本课采用 `call_async()`，由应用明确检查等待结果。

课堂提问：

> 没收到“扣款成功”的响应，能否直接再扣一次？

答案：不能仅凭超时判断服务端未执行，要结合幂等设计或业务请求编号。

### 补充命令行对照实验

目的：区分无服务响应与业务返回失败。服务端使用第 11 页完整 add_server.py（教师可提前准备）。A 未启动时，B：
```bash
timeout --signal=INT 3s ros2 service call /add_two_ints example_interfaces/srv/AddTwoInts "{a: 2, b: 3}"
```
预期等待服务、3 秒退出；timeout 返回 124 属于预期等待结束，不是服务计算出失败。A 启动服务端后重新 call，预期 sum=5。自定义服务的 success=false 则是收到业务响应，和没有响应不同。

这页最后可以总结：

> **发现服务、发送请求、等待响应属于不同阶段；超时表达客户端没有按时得到结果，不能证明服务端没有执行。**

---

## 第 7 页｜服务通信的四大核心特性

这页你可以把它讲成：

> **服务最值得记住的，是按需调用、请求响应、结果关联和一次交互的明确边界。它适合处理一件能够较快给出结果的事情。**

### 1. 同步或异步地取得响应

同步方式中，调用方停在当前步骤等待；异步方式中，调用方先取得 Future，之后再处理结果。

两种方式都使用同一份 `.srv` 定义。

### 2. 呼叫与响应

服务端通常在收到请求后才产生对应响应，不像传感器话题那样周期性向所有订阅者推送。

```text
Topic：每秒报告一次电量
Service：现在查询一次电量
```

### 3. 双向交互闭环

客户端发送输入，服务端返回处理结果。响应可以带业务状态：

```text
success=false
message="设备忙，当前不允许切换模式"
```

收到响应只说明通信完成，业务是否成功还要看响应字段。

### 4. 单次交互边界

一次请求有对应的一次结果。这个边界便于记录：谁调用、何时开始、用了多久、结果是什么。

### 教师说明

PPT 中“单次原子性交互”应理解为一次请求与响应的组织形式，不能推导出数据库事务原子性。服务回调也可能先改变设备状态，再因异常未返回正常响应。

服务端可以保存内部状态。例如“设置模式”结束后，设备的新模式会持续存在，不能说“无状态残留”。

课堂判断：

```text
一次设置成功后，相机持续开启     → 可以有内部状态
每秒查询 100 次代替状态广播      → 先考虑 Topic 是否更合适
收到响应但 success=false        → 通信完成，业务失败
```

这页最后可以总结：

> **服务把一次业务调用组织成明确的输入和输出，应用仍要定义成功、失败和副作用的含义。**

---

## 第 8 页｜服务的底层 DDS 实现原理

这页你可以把它讲成：

> **在本课讨论的 DDS 中间件中，Service 仍然利用读写端点传递数据。可以先把它理解为请求方向和响应方向两条数据通路，再加上请求关联与服务发现。**

板书：

```text
客户端 Requester                  服务端 Replier
请求写端 ───── Request 通路 ─────→ 请求读端
响应读端 ←──── Reply 通路 ───────── 响应写端
```

### 1. 请求通路

客户端把 `a=2、b=3` 以及必要的关联信息送到服务端。

### 2. 响应通路

服务端计算 `sum=5`，将响应与原请求关联起来。客户端才能知道哪个 Future 应该完成。

### 3. 为什么不直接手写两个话题

如果自己实现，还需要处理：

```text
多个请求同时在途时的编号
响应属于哪个客户端和哪次请求
服务提供者是否可用
请求和响应的类型约定
本地等待与异常处理
```

Service API 封装了通用通信部分，但业务超时、重试和去重策略仍需要应用设计。

### 4. 默认 QoS 的含义

服务默认 Profile 使用 Reliable、Volatile。Reliable 有助于可靠交付，Volatile 避免重启后的服务端把陈旧请求作为历史数据重新接收。

不能把 Reliable 理解成“每条指令必定成功执行一次”。节点故障和业务副作用仍然需要处理。

[核对：Humble 服务默认 QoS](https://github.com/ros2/ros2_documentation/blob/humble/source/Concepts/Intermediate/About-Quality-of-Service-Settings.rst)。

### 教师说明

“两个 DDS Topic”是常见 DDS RMW 下的理解模型，底层名称和具体映射由实现决定。不要拿 ROS 层的 `ros2 topic list` 去要求所有 Service 请求与响应都以普通 ROS Topic 形式显示。

PPT 中“强制可靠”“超时自动检测”过于绝对。默认配置与可配置能力要区分，业务超时也不由 `.srv` 自动提供。

课堂提问：

> 同时发出 10 个请求，响应顺序与发送顺序不同，会不会必然把答案配错？

答案：正常关联机制按请求身份匹配，不靠响应到达顺序猜测。

这页最后可以总结：

> **DDS 提供底层传输，Service 在其上建立请求与响应的关联语义，应用负责业务层的超时和可靠处理。**

---

## 第 9 页｜同步调用 vs 异步调用

这页你可以把它讲成：

> **同步和异步，主要区别是发出请求之后怎样安排调用方的执行。使用异步 API，可以先拿到 Future，再让执行器接收响应。**

板书：

```text
同步：发请求 → 等响应 → 使用结果
异步：发请求 → 得到 Future → 继续工作 → 处理完成的结果
```

### 1. Future 是结果占位符

```python
future = client.call_async(request)
```

刚返回时，`future` 不是 `Response`。只有 Future 完成后，才能读取它对应的响应或异常。

### 2. 完整最小异步请求示例

保存为 `第二课/课堂实验/add_client.py`。第 11 页提供配套服务端；先启动服务端，再运行此脚本。

```python
import rclpy
from rclpy.node import Node
from example_interfaces.srv import AddTwoInts


def main():
    rclpy.init()
    node = Node('add_client')
    client = node.create_client(AddTwoInts, '/add_two_ints')
    future = None

    try:
        if not client.wait_for_service(timeout_sec=3.0):
            node.get_logger().error('服务端尚未可用')
            return

        request = AddTwoInts.Request()
        request.a = 2
        request.b = 3
        future = client.call_async(request)
        node.get_logger().info('请求已发出，当前持有的是 Future')

        rclpy.spin_until_future_complete(node, future, timeout_sec=3.0)
        if not future.done():
            client.remove_pending_request(future)
            node.get_logger().error('等待响应超时，服务端可能仍在执行')
            return

        response = future.result()
        node.get_logger().info(f'2 + 3 = {response.sum}')
    except Exception as exc:
        node.get_logger().error(f'调用异常：{exc}')
    finally:
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == '__main__':
    main()
```

运行：

```bash
python3 第二课/课堂实验/add_client.py
```

这里的 `call_async()` 是异步发送，后面的 `spin_until_future_complete()` 仍然等待完成，同时调度回调。脚本主动选择“先发请求，再在主流程里等结果”。

### 3. 在持续运行的节点中使用完成回调

已有客户端节点还可以采用以下局部写法：

```python
future = self.client.call_async(request)
future.add_done_callback(self.handle_response)

def handle_response(self, future):
    try:
        response = future.result()
        self.get_logger().info(f'sum={response.sum}')
    except Exception as exc:
        self.get_logger().error(f'调用失败：{exc}')
```

节点继续 `spin()`，完成回调才有机会运行。工程中另加定时器或截止时间来管理超时与在途请求。

### 教师说明

PPT 的 `call_sync()` 是伪代码，Humble 的 rclpy 同步方法名是 `call()`。rclcpp 常用 `async_send_request()`，可以在合适的主流程中等待其结果。

ROS 2 Humble 官方指南推荐异步服务调用。不要因为某个操作“很快”，就把同步调用放进回调中；死锁风险取决于执行器和回调组，而不只是计算耗时。[核对：Humble 同步与异步调用指南](https://github.com/ros2/ros2_documentation/blob/humble/source/How-To-Guides/Sync-Vs-Async.rst)。

课堂提问：

> 改成 `call_async()` 后，马上写 `response.sum` 为什么不对？

答案：当前拿到的是 Future，响应尚未准备好。

这页最后可以总结：

> **异步发送先返回 Future，执行器负责接收响应，程序在 Future 完成后再使用结果。**

---

## 第 10 页｜服务与话题的选型对比

这页你可以把它讲成：

> **先判断交互含义，再选择 API。持续报告当前数据适合 Topic，提出一次问题并取得答案适合 Service。**

| 对比维度 | Topic | Service |
| --- | --- | --- |
| 业务关系 | 发布数据，订阅者使用 | 发出请求，服务端返回响应 |
| 数据组织 | 一条条消息，可持续也可单次 | 每次调用包含 Request 和 Response |
| 使用方式 | 发布与订阅回调 | 同步等待或异步调用 |
| 接口依赖 | 名称、类型、QoS | 名称、类型、服务可用性 |
| 多节点关系 | 可有多个发布者和订阅者 | 一个明确提供者可服务多个客户端 |
| 原生业务回执 | 不提供请求响应语义 | 提供对应请求的响应 |
| 常见用途 | 雷达、图像、里程计、状态流 | 查询、设置、清零、短时触发 |

### 同一设备可以同时用两种方式

```text
/camera/image_raw     → Topic：持续图像
/camera/set_enabled   → Service：设置开关并返回结果
```

如果希望“相机打开后再启动识别”，服务响应可以作为下一步的依据。

教师要追问：返回成功表示驱动接受设置，还是已经收到第一帧图像？这应由接口约定明确。

### 教师说明

客户端通常不需要硬编码服务端 IP，ROS 2 通过发现机制找到服务。PPT 中“必须知道网络地址”应改为知道名称和接口、并能够发现可用服务。

Topic 的发布调用也不能绝对表述为“立即返回、完全不等待任何底层条件”。本页重点是发布者不等待订阅者的业务处理结果。

课堂判断：

```text
实时电量曲线               → Topic
现在查询一次电量           → Service
设置工作模式并返回是否成功 → Service
一段需要取消的巡检任务     → 后面讲的 Action
```

这页最后可以总结：

> **Topic 表达数据分发，Service 表达一次问答；同一设备可以按不同需求同时提供两种接口。**

---

## 第 11 页｜服务编程模型：回调组与并发控制

这页你可以把它讲成：

> **服务回调不会因为收到网络数据就自动占到一个线程。它能不能执行，由 Executor 调度；它能不能和别的回调同时执行，还受到回调组的限制。**

先板书两个问题：

```text
Executor：有几个可执行回调的工作线程？
Callback Group：哪些回调允许同时执行？
```

### 1. MutuallyExclusive：互斥回调组

同一组内的回调不能同时执行。节点默认回调组是互斥组。

可以类比只有一把钥匙的设备间：同组回调需要轮流进去。

教师提醒：互斥只限制组内并发，不保证所有共享数据天然安全。如果其他回调组或工作线程也访问数据，仍要设计同步机制。执行器也不保证所有回调严格按触发先后顺序执行。

### 2. Reentrant：可重入回调组

允许组内回调并发，甚至允许同一个回调的多次执行重叠。

```python
from rclpy.callback_groups import ReentrantCallbackGroup

self.group = ReentrantCallbackGroup()
```

回调组应保存在节点成员中，避免丢失引用。

可重入组不会自动创建额外线程。还需要合适的执行器，业务也要处理共享资源竞争。

### 3. 为什么回调内同步等待可能死锁

以定时器回调调用服务为例：

```text
定时器进入回调，占住唯一执行线程
        ↓
同步调用服务，等待响应
        ↓
响应到了，但客户端处理响应也需要执行器
        ↓
唯一线程仍在等待，无法处理响应
        ↓
永久等待
```

即使服务端在另一进程，客户端自身也可能因为无法处理响应而卡住。

多线程执行器也不是单独的万能解法：如果等待回调与客户端仍在同一个互斥组，组的限制仍可能阻止响应处理。

### 4. 优先用异步完成后续步骤

```text
回调发起 call_async()
        ↓
登记完成回调并及时返回
        ↓
执行器处理其他工作和服务响应
        ↓
完成回调继续后续业务
```

不要在原回调里紧接着忙等 `future.done()`，也不要反复嵌套 spin 来“硬等”。

如果业务确实需要线程并发，执行器线程数和回调组应一起设计。[核对：Humble 回调组指南](https://github.com/ros2/ros2_documentation/blob/humble/source/How-To-Guides/Using-callback-groups.rst)。

### 5. 配套的最小服务端

保存为 `第二课/课堂实验/add_server.py`：

```python
import rclpy
from rclpy.node import Node
from example_interfaces.srv import AddTwoInts


class AddServer(Node):
    def __init__(self):
        super().__init__('add_server')
        self.service = self.create_service(
            AddTwoInts, '/add_two_ints', self.handle_request
        )

    def handle_request(self, request, response):
        response.sum = request.a + request.b
        self.get_logger().info(
            f'{request.a} + {request.b} = {response.sum}'
        )
        return response


def main():
    rclpy.init()
    node = AddServer()
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

终端 A：

```bash
mkdir -p tf_debug
python3 第二课/课堂实验/add_server.py
```

终端 B 运行第 9 页的客户端，预期得到 `2 + 3 = 5`。

这个服务端回调很短，单线程执行器就能完成示范，不需要为加法额外引入并发。

课堂提问：

> 把执行器改成四线程，但所有回调仍放在同一个互斥组，是否能四个回调一起执行？

答案：该组的回调仍然受到互斥限制。

这页最后可以总结：

> **线程决定执行能力，回调组决定并发许可；回调内发出异步请求后及时返回，才能给响应处理留下机会。**

---

## 第 12 页｜自定义 srv 文件开发规范

这页你可以把它讲成：

> **上一课的 `.msg` 描述一份消息，这一课的 `.srv` 在同一个文件里描述输入和输出。中间一行 `---` 就是 Request 与 Response 的分界。**

### 1. 文件位置与命名

```text
robot_interfaces/
├─ msg/
│  └─ RobotStatus.msg       # 如果上一课已创建
├─ srv/
│  └─ RobotCommand.srv
├─ CMakeLists.txt
└─ package.xml
```

接口文件使用大驼峰命名，字段采用 `snake_case`。接口包使用 `ament_cmake` 与 rosidl 工具链，Python 业务包也能导入生成的接口。

### 2. 按 PPT 写出请求与响应

`RobotCommand.srv`：

```text
# Request：课堂演示的任务状态指令
# 允许值：start / stop / pause
string command
# 业务任务编号，约定 >= 0
int32 task_id
---
# 是否完成本次状态操作
bool success
# 成功说明或失败原因
string message
```

强调必须真实写出独占一行的 `---`，不能只在注释中标“Response”。

板书：

```text
上半段：调用者需要提供什么？
下半段：处理者需要返回什么？
```

### 3. success 与 message 的作用

```text
success=true   → 业务操作成功
success=false  → 请求已处理，但操作失败
message        → 给出原因
```

不是所有服务必须有这两个字段。例如 `AddTwoInts` 返回 `sum` 就足够。工程中还可以用结构化错误码，避免客户端解析中文文本来判断错误类型。

### 4. 单一职责与类型复用

PPT 的 `RobotCommand` 可以用来理解格式。真正的业务设计可以进一步拆为“设置任务状态”“查询任务状态”等清晰接口，或把字符串命令改成有常量约束的枚举字段。

对于现成的空请求触发、布尔设置等需求，先查看：

```bash
ros2 interface show std_srvs/srv/Trigger
ros2 interface show std_srvs/srv/SetBool
```

复杂字段可以引用已有消息。例如传位姿时复用 `geometry_msgs/Pose`，并在构建文件声明依赖。

### 5. 幂等设计

类比灯的开关：

```text
“设为开启”     → 连续执行两次，最终仍然开启
“切换一次开关” → 连续执行两次，又回到原状态
```

前者更容易在超时重试时保证效果一致。业务任务编号可以辅助查重，但只定义 `task_id` 字段不会自动实现去重。

课堂练习：

> “电量查询”接口中，机器人编号放在哪一段，电量百分比放在哪一段？

答案：机器人编号在 Request，电量与查询状态在 Response。

这页最后可以总结：

> **`.srv` 用一条分隔线划清输入和输出，字段与注释说明业务含义，幂等性由服务端逻辑实现。**

---

## 第 13 页｜srv 文件编译配置原理

这页你可以把它讲成：

> **`.srv` 是接口设计图，不能直接当 Python 类使用。rosidl 读取这张图，生成请求、响应及类型支持代码，安装后节点才能导入。**

板书：

```text
RobotCommand.srv
      ↓
解析 Request / Response
      ↓
IDL 与语言代码生成
      ↓
Python 模块 / C++ 头文件 / 类型支持
      ↓
构建并安装到工作空间
      ↓
source → import / include
```

### 1. 创建或复用接口包

如果第一课已经创建 `robot_interfaces`，直接在现有包中增加 `srv/`；不要重复创建，也不要删除已有的 `RobotStatus.msg` 配置。

如果还没有该包：

```bash
mkdir -p ../ros2_ws/src
if [ ! -d ../ros2_ws/src/robot_interfaces ]; then
  ros2 pkg create robot_interfaces --destination-directory ../ros2_ws/src --build-type ament_cmake
fi
```

创建服务目录：

```bash
mkdir -p ../ros2_ws/src/robot_interfaces/srv
```

课前准备已写入第 12 页完整定义；若需重建，执行 `python3 prepare_course.py 2`。服务文件为：

```text
./src/robot_interfaces/srv/RobotCommand.srv
```

### 2. package.xml 声明依赖

在现有 `<package>` 内补齐以下配置，不重复添加已有条目：

```xml
<buildtool_depend>ament_cmake</buildtool_depend>
<build_depend>rosidl_default_generators</build_depend>
<exec_depend>rosidl_default_runtime</exec_depend>
<member_of_group>rosidl_interface_packages</member_of_group>
```

生成器负责构建接口，runtime 是生成接口运行时所需的依赖，接口组标记声明这个包的接口包身份。

`RobotCommand.srv` 只用基础字段，因此不需要仅为它添加 `std_msgs`。如果保留上一课的 `RobotStatus.msg` 且其中使用 `std_msgs/Header`，则保留：

```xml
<depend>std_msgs</depend>
```

还要保留 `package.xml` 原有名称、版本、维护者、许可证和 `ament_cmake` 导出配置。

### 3. CMakeLists.txt 注册接口

只包含本课 `.srv` 的新包可使用：

```cmake
cmake_minimum_required(VERSION 3.8)
project(robot_interfaces)

find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "srv/RobotCommand.srv"
)

ament_export_dependencies(rosidl_default_runtime)
ament_package()
```

如果承接第一课，同时注册已有消息：

```cmake
cmake_minimum_required(VERSION 3.8)
project(robot_interfaces)

find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(std_msgs REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/RobotStatus.msg"
  "srv/RobotCommand.srv"
  DEPENDENCIES std_msgs
)

ament_export_dependencies(rosidl_default_runtime)
ament_package()
```

`rosidl_generate_interfaces()` 汇总该包接口；`DEPENDENCIES` 声明被接口字段引用的外部消息包。`ament_package()` 放在后面完成包注册。

### 4. 编译、加载与检查

```bash
(cd ../ros2_ws && colcon build --packages-select robot_interfaces)
source ../ros2_ws/install/setup.bash
ros2 interface show robot_interfaces/srv/RobotCommand
```

检查 Python 导入：

```bash
python3 -c "from robot_interfaces.srv import RobotCommand; print(RobotCommand.Request())"
```

对应 C++ 引用：

```cpp
#include "robot_interfaces/srv/robot_command.hpp"
```

### 教师说明

PPT 的 CMake 代码存在排版和转义痕迹，实际文件应写 `${PROJECT_NAME}`，不带多余反斜杠。示例注册名也要与本课的 `RobotCommand.srv` 一致。

生成的接口类型不等于完整业务节点。Server 和 Client 的行为来自 rclpy/rclcpp API 与我们编写的回调。[核对：Humble 自定义接口教程](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Beginner-Client-Libraries/Custom-ROS2-Interfaces.rst)。

课堂提问：

> 编译成功，为什么新终端还是无法导入 `RobotCommand`？

答案：新终端可能没有 source 工作空间。

这页最后可以总结：

> **package.xml 声明依赖，CMake 注册生成规则，colcon 执行构建，source 让当前终端找到新接口。**

---

## 第 14 页｜服务调试体系与故障排查

这页你可以把它讲成：

> **服务调用出问题时，先用命令行确认名称、类型和服务端行为，再去看客户端。手动调用可以帮助我们缩小问题范围。**

### 1. 查看服务名称和类型

先保持第 11 页服务端运行：

```bash
ros2 service list
ros2 service list -t
ros2 service type /add_two_ints
```

`list` 列的是服务名称，不是服务节点。参数服务也可能出现在列表中。

### 2. 按接口反查服务

```bash
ros2 service find example_interfaces/srv/AddTwoInts
```

预期找到 `/add_two_ints`。

### 3. 查看请求和响应字段

```bash
ros2 interface show example_interfaces/srv/AddTwoInts
```

先看定义，再写 YAML，避免猜字段名。

### 4. 手动调用

```bash
ros2 service call /add_two_ints example_interfaces/srv/AddTwoInts "{a: 2, b: 3}"
```

预期响应中 `sum: 5`。要区分工具显示“请求已发送”和真正显示 Response。

### 5. 查看提供服务的节点

```bash
ros2 node list
ros2 node info /add_server
```

`node info` 可帮助确认该节点声明了哪些服务。

### 6. 自定义接口也要连接真实服务端

`interface show` 成功只证明类型可用，并不证明 `/robot_command` 已启动。

完成第 13 页后，可保存以下完整脚本为 `第二课/课堂实验/robot_command_server.py`：

```python
import rclpy
from rclpy.node import Node
from robot_interfaces.srv import RobotCommand


class CommandServer(Node):
    def __init__(self):
        super().__init__('robot_command_server')
        self.tasks = {}
        self.service = self.create_service(
            RobotCommand, '/robot_command', self.handle_request
        )

    def handle_request(self, request, response):
        if request.task_id < 0:
            response.success = False
            response.message = 'task_id 必须 >= 0'
        elif request.command not in ('start', 'stop', 'pause'):
            response.success = False
            response.message = 'command 只允许 start / stop / pause'
        else:
            # 软件状态模拟：同一任务重复设为同一状态，效果不变
            self.tasks[request.task_id] = request.command
            response.success = True
            response.message = (
                f'任务 {request.task_id} 状态设为 {request.command}'
            )
        self.get_logger().info(response.message)
        return response


def main():
    rclpy.init()
    node = CommandServer()
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

终端 A 加载接口环境并启动：

```bash
source ../ros2_ws/install/setup.bash
python3 第二课/课堂实验/robot_command_server.py
```

终端 B 同样加载环境，然后测试：

```bash
source ../ros2_ws/install/setup.bash
ros2 service call /robot_command robot_interfaces/srv/RobotCommand "{command: 'start', task_id: 1}"
ros2 service call /robot_command robot_interfaces/srv/RobotCommand "{command: 'unknown', task_id: 1}"
```

第一条应返回成功，第二条应返回 `success: false` 和原因。模拟仅更新内存中的任务状态，不代表真实硬件执行结果。

### 故障排查顺序

```text
服务是否可发现？
    ↓
完整名称和类型是否正确？
    ↓
命令行调用能否得到响应？
    ↓
服务端是否 spin，回调是否阻塞或异常？
    ↓
客户端 Future、执行器与回调组是否正确？
    ↓
Domain、网络和 QoS 是否兼容？
```

CLI 成功而程序失败时，重点看客户端环境、名称、字段、spin 和等待方式。

CLI 也无响应时，先查看服务端日志和执行器，不要只反复重试。

### 教师说明

Humble 自定义接口生成在 `ament_cmake` 包中配置，不是仅在 `setup.py` 注册一个 `.srv` 就能生成。Python 业务包的安装配置与接口生成是两件事。

课堂故障题：

> 服务能列出来，发请求却一直等，服务器也没有打印回调日志，先检查什么？

答案：先检查服务器是否 spin、执行器是否被其他回调占用，再查名称、类型与通信条件。

### 补充命令行对照实验

目的：相同任务重复设状态，观察幂等；再对照非法输入。A 保持本页 robot_command_server.py；B：
```bash
ros2 service call /robot_command robot_interfaces/srv/RobotCommand "{command: start, task_id: 1}"
ros2 service call /robot_command robot_interfaces/srv/RobotCommand "{command: start, task_id: 1}"
ros2 service call /robot_command robot_interfaces/srv/RobotCommand "{command: start, task_id: -1}"
```
预期前两次成功且软件任务状态不重复累加，第三次 success=false 并给原因。C 用 node info /robot_command_server 查接口。此例仅更新内存状态，不证明硬件命令幂等。结束 A；下个动作实验需要换服务端。

这页最后可以总结：

> **先看接口是否存在，再用 CLI 验证业务；把接口生成、服务运行和客户端等待三个环节分开检查。**

---

## 第 15 页｜服务通信最佳实践

这页你可以把它讲成：

> **一个服务能返回结果，只完成了最小功能。工程中还要考虑非法输入、重复调用、超时和版本变化。**

### 1. 服务职责保持清晰

优先表达明确操作：

```text
查询一次状态
设定目标模式
清除一项故障
触发一次短时保存
```

需要长时间进度和取消的任务，后面用 Action 组织。

### 2. 服务端独立检查参数

客户端检查是为了尽早提示，服务端检查才是业务执行前的约束。

例如：

```text
task_id 是否在允许范围？
command 是否有效？
设备当前状态是否允许该操作？
当前是否存在冲突任务？
```

接口类型只能限制数据格式，不会自动证明业务合法。

### 3. 幂等与有限重试

对“设为某状态”可以设计重复请求的相同效果。对“取出一件货物”等有副作用的操作，需要业务请求 ID、处理记录或明确的重复请求规则。

Reliable 的底层重传与应用主动再次提交请求要区分。ROS Service 不替业务实现“恰好执行一次”。

### 4. 超时按业务测量

分别管理：

```text
发现服务的等待时间
请求响应的等待时间
重试次数与重试间隔
最大在途请求数
```

PPT 的“典型耗时两三倍”可作为课堂讨论起点，实际应依据耗时分布、最坏情况和业务截止时间确定。

超时后记录“结果尚未确认”，不要自动宣称“操作失败且未执行”。

### 5. 控制请求载荷与调用链

大文件不适合反复塞进短请求。可以返回文件标识或地址，同时明确访问方式与有效期。

过长的 `A → B → C → D` 调用链会累积延迟和故障依赖。拆分流程时仍要定义每一步的完成、失败和补偿行为。

异步 API 帮助减少调用方阻塞，不会自动减少服务端计算量，也不会自动把任意长任务变成合适的 Service。

### 6. 日志和版本演进

日志建议关联服务名称、业务任务编号、耗时、业务结果和异常原因。

接口文件变化后，要重新生成，并让调用双方使用一致的定义。PPT 中“新增字段、保留旧字段就向后兼容”不能作为 ROS 2 接口兼容性的保证；需要按中间件、类型支持与部署版本验证，必要时使用新接口类型或新服务名称。

课堂提问：

> 服务端已经执行成功，但响应丢失，客户端又调用一次，怎样避免重复副作用？

答案：通过幂等操作或持久化的业务请求去重，并明确重复请求的响应规则。

这页最后可以总结：

> **好的服务接口要明确职责、输入约束和错误结果，也要让超时与重试产生可解释的行为。**

---

## 第 16 页｜第二部分：动作通信机制

这是一张章节过渡页，可以这样讲：

> **现在从“问一个问题”切换到“交付一个任务”。导航到门口可能需要几十秒，这期间客户端想知道任务是否受理、机器人走到哪里，以及能不能改变主意。**

板书：

```text
一次问答：Request → Response

一个任务：Goal
            ↓
          受理
            ↓
       执行 + Feedback
            ↓
       终态 + Result
```

问学生：

> 把导航服务改成异步调用，是否就能显示“已经走了 50%”？

答案：不会自动有进度反馈。异步改变等待方式，Action 提供任务交互模型。

这页最后可以说：

> **接下来用 Action 给长任务定义目标、过程、终结和取消。**

---

## 第 17 页｜为什么需要动作通信？

这页你可以把它讲成：

> **服务可以异步调用，话题也可以传递任务信息。但如果我们需要受理、反馈、取消和最终结果，Action 已经提供了一套标准组织方式。**

### 1. Service 只有请求与最终响应

把“导航到门口”放进一个服务：

```text
请求：门口坐标
等待 40 秒
响应：是否到达
```

即便客户端异步等待，这个 `.srv` 本身仍没有定义中间进度或标准取消协议。

客户端等不及停止等待，也不代表导航服务端停止运动。

### 2. Topic 可以拼出任务流程，但要自己约定

例如自行设计：

```text
/task/command
/task/progress
/task/result
/task/cancel
```

接着还要解决任务编号、受理响应、状态迁移、结果关联和取消规则。

Topic 的 Reliable 配置提供传输保障，不能直接当作“机器人接受并完成指令”的业务确认。

### 3. Action 的价值

```text
接受还是拒绝目标？
哪个目标正在执行？
当前进度是什么？
取消请求是否受理？
最终属于成功、取消还是中止？
这次任务的结果数据是什么？
```

类比快递：收到订单、配送途中、取消申请和最终签收，分别有不同含义。

### 教师说明

PPT 把 Service 的局限写成“长任务一定阻塞界面”，课堂应改为：同步等待可能阻塞调用方；异步 Service 避免这种等待，却仍不提供 Action 的完整生命周期。

也不要说 Topic“发后即焚”。话题有 QoS 和持久性配置，只是没有原生的任务请求与结果关联协议。

课堂回顾第 2 页问题：

> 异步 Service 与 Action 最关键的区别是什么？

答案：一个改变等待方式，另一个定义任务的目标、反馈、结果、状态和取消交互。

这页最后可以总结：

> **长任务需要的交互越完整，使用 Action 的价值就越明显。**

---

## 第 18 页｜三段式通信架构

这页你可以把它讲成：

> **Action 从业务上分成目标、反馈和结果：目标说明要做什么，反馈说明正在做到哪一步，结果说明最后做成了什么。**

用本课计数示例统一说明：

```text
Goal：一共完成 10 步
Feedback：当前完成 1、2、3……步
Result：最终完成多少步，以及结束说明
```

### 1. Goal：输入

客户端发送必要任务参数。

```text
导航：目标位姿
机械臂：目标姿态或轨迹
计数：目标步数
```

提交时由 Action 协议关联 Goal UUID，不需要为了基本关联能力在 `.action` 的用户 Goal 中手写同样的字段。

### 2. Feedback：过程

服务端按业务需要发布：

```text
剩余距离
当前阶段
已完成步数
进度百分比
```

Feedback 可选。频率由服务端实现决定，Action 不会自动生成进度。

### 3. Result：最终输出

结果字段根据任务设计，例如：

```text
最终完成步数
业务错误码
结束原因
产物标识
```

协议层还提供最终状态。`SUCCEEDED`、`CANCELED`、`ABORTED` 与用户 Result 数据一起帮助客户端理解结束原因。

### 一个很容易记反的地方

业务过程常按：

```text
Goal → Feedback → Result
```

但 `.action` 文件定义顺序固定为：

```text
Goal
---
Result
---
Feedback
```

可以在黑板上把“执行顺序”和“文件顺序”并排写。第 23 页实际写文件时再核对。

课堂提问：

> “还剩 2 米”与“因为障碍无法到达”分别放在哪里？

答案：剩余距离是 Feedback，最终失败原因是 Result，同时协议终态通常是 `ABORTED`。

这页最后可以总结：

> **目标定义输入，反馈描述过程，结果交付输出；接口文件按 Goal、Result、Feedback 编写。**

---

## 第 19 页｜底层 5 个通信通道

这页你可以把它讲成：

> **Action 在 ROS 层由三个服务和两个话题组成。服务负责需要请求与响应的交互，话题负责反馈和状态分发。**

以 `/count_steps` 为例：

| 通道 | ROS 名称 | 用途 |
| --- | --- | --- |
| 提交目标服务 | `/count_steps/_action/send_goal` | 提交 Goal，取得接受或拒绝的响应 |
| 获取结果服务 | `/count_steps/_action/get_result` | 按 Goal ID 获取终态和用户结果 |
| 取消目标服务 | `/count_steps/_action/cancel_goal` | 请求取消，取得取消受理响应 |
| 反馈话题 | `/count_steps/_action/feedback` | 发布 Goal ID 与用户反馈 |
| 状态话题 | `/count_steps/_action/status` | 发布各目标的协议状态 |

板书：

```text
Action = 3 个 Service + 2 个 Topic
       + Goal ID 关联 + 状态机 + 统一 API
```

### 1. 提交目标的响应只说明是否受理

```text
accepted=true
```

表示服务端愿意处理该目标，不表示任务成功完成。

### 2. 获取结果不必等到终态之后才发请求

目标受理后，可以立即调用 `get_result_async()`。服务端在结果准备好后返回，客户端的 Future 也在那时完成。

### 3. 取消服务返回的是受理信息

取消响应说明哪些目标进入了 `CANCELING`，实际完成取消要看最终状态和结果。

### 4. 状态与反馈不同

```text
Status：EXECUTING             → 协议状态
Feedback：current_step=4      → 用户业务进度
```

进入 `EXECUTING` 之后，状态可以不变，而反馈继续更新。

### 5. 默认隐藏的名字

`_action` 让这些实体默认不显示在普通列表中。

第 24 页启动示例后，终端 C 可以执行：

```bash
ros2 service list --include-hidden-services -t
ros2 topic list --include-hidden-topics -t
```

这页讲的是五个 ROS 通信实体，不是五个 DDS 底层 Topic。DDS RMW 下三个服务各自还有请求与响应映射，不能混用层级计数。

### 教师说明

Service 的响应提供明确交互回执，不等于分布式事务的“强一致性”保证。取消请求也不限制为每个任务只能发送一次。

默认状态话题通常使用 Reliable、Transient Local、Keep Last 1，方便新加入的观察者取得最近状态；反馈 QoS 可配置，发布和订阅双方仍需兼容。[核对：ROS 2 Action 设计文档](https://design.ros2.org/articles/actions.html)。

课堂提问：

> 只看到 `accepted=true`，能否宣布“机器人已到达”？

答案：不能，还要获取最终状态和结果。

### 补充命令行对照实验

目的：观察一个动作对应的多个通信对象。教师已构建第 24 页接口并保存第 23 页完整 count_steps_server.py；A 启动服务端后 C：
```bash
ros2 action list -t
ros2 action info /count_steps
ros2 service list --include-hidden-services -t | grep count_steps
ros2 topic list --include-hidden-topics -t | grep count_steps
```
预期动作名称一个，协议下有 send_goal/get_result/cancel_goal 三服务与 feedback/status 两话题。隐藏选项以本机 --help 核对。类型生成可用不代表服务端已运行，缺对象先查 A 与 source。

这页最后可以总结：

> **三个服务管理目标、结果和取消，两条话题传反馈与状态，Action 把它们统一关联到同一个目标。**

---

## 第 20 页｜完整通信生命周期

这页你可以把它讲成：

> **把一次 Action 看成一张任务单：先受理，接着执行，最后留下明确的终态与结果。客户端既要处理成功路线，也要处理拒绝、取消和中止。**

### 1. 正常成功路线

```text
发送 Goal
    ↓
得到 GoalHandle，检查 accepted
    ↓
立即登记 get_result_async()
    ↓
任务执行，持续接收 Feedback
    ↓
服务端设置 SUCCEEDED 并返回 Result
    ↓
客户端结果 Future 完成
```

### 2. 目标拒绝路线

例如目标步数非法、机器人忙或资源不可用。

```text
发送 Goal → accepted=false → 客户端处理拒绝
```

拒绝目标没有进入已接受目标的状态机，不能继续按“已执行任务”等待成功结果。

### 3. 中途取消路线

```text
目标已接受并执行
    ↓
客户端发送 cancel_goal_async()
    ↓
服务端允许取消，进入 CANCELING
    ↓
执行逻辑检测取消请求，完成停止与清理
    ↓
设置 CANCELED，返回部分结果
```

### 4. 执行失败路线

任务开始后发现无法完成，服务端应明确设置 `ABORTED`，返回原因。

异常处理不能只打印错误而忘记结束目标。客户端否则可能继续等待。

### 5. 资源清理与结果缓存

任务结束后清理设备和临时资源，结果数据通常按可配置时间缓存，供已登记的请求或后续查询使用。

不能承诺结果永久可取；客户端晚于缓存期限查询，可能得到未知目标状态。

### 教师说明

PPT 的“先收到终态广播，再发结果请求”是可理解的流程示意，但常见客户端在受理后立即请求结果，无需先轮询状态话题。

异步 API 不会自动保证界面流畅。如果客户端反馈回调做耗时工作，或服务端执行回调占满执行器，仍可能延迟其他事件。

课堂提问：

> 接受目标后马上 `get_result_async()`，是否会把任务提前结束？

答案：不会，它登记等待结果，任务仍按服务端逻辑执行。

这页最后可以总结：

> **受理、执行和结束各有自己的含义，结果等待可以提前登记，取消完成要由服务端明确确认。**

---

## 第 21 页｜动作状态机详解

这页你可以把它讲成：

> **每个被接受的目标都有自己的状态机。状态说明任务处于哪个阶段，反馈说明这个阶段已经完成多少。状态由服务端维护，客户端观察与处理。**

### 1. 状态码与含义

| 数值 | 常量 | 含义 |
| --- | --- | --- |
| 0 | `STATUS_UNKNOWN` | 未知状态，例如不能识别该目标 |
| 1 | `STATUS_ACCEPTED` | 目标已经接受，尚未开始执行 |
| 2 | `STATUS_EXECUTING` | 正在执行 |
| 3 | `STATUS_CANCELING` | 已接受取消请求，正在停止与清理 |
| 4 | `STATUS_SUCCEEDED` | 目标成功完成 |
| 5 | `STATUS_CANCELED` | 取消完成 |
| 6 | `STATUS_ABORTED` | 服务端中止任务 |

前三个活动状态是 `ACCEPTED`、`EXECUTING`、`CANCELING`，三个终态是 `SUCCEEDED`、`CANCELED`、`ABORTED`。

查看真实定义：

```bash
ros2 interface show action_msgs/msg/GoalStatus
```

Python 使用常量，避免凭记忆写数字：

```python
from action_msgs.msg import GoalStatus

if response.status == GoalStatus.STATUS_SUCCEEDED:
    print('任务成功')
```

### 2. 常见状态路线

板书：

```text
ACCEPTED → EXECUTING → SUCCEEDED
                └──→ ABORTED

ACCEPTED 或 EXECUTING
          ↓ 接受取消
       CANCELING
          ↓ 停止与清理完成
        CANCELED
```

这是便于教学的常见路径。取消与自然完成可能竞争，进入 `CANCELING` 后并不保证最终一定是 `CANCELED`；状态机也允许任务在竞争或清理异常时成功或中止。

[核对：Humble 目标状态迁移实现](https://github.com/ros2/rcl/blob/humble/rcl_action/src/rcl_action/goal_state_machine.c)。

### 3. “拒绝”不属于接受后的状态机

```text
目标未被接受 → accepted=false
```

不要给它虚构一个 `REJECTED` GoalStatus 数字。拒绝通过提交目标的响应表示。

### 4. 终态不可恢复为执行中

已经成功、取消或中止的目标，不再继续作为同一个目标运行。重试任务需要提交新目标。

可以类比订单：已结单的订单不能再变回同一次配送中的订单。

### 教师说明

PPT 的“待接受（ACCEPTED）”容易误导，应该讲成“已接受、待执行”。它也不自动意味着业务硬件资源已经锁定，资源预留由实现决定。

PPT 省略的 `CANCELING` 是本页重点补充。`ABORTED` 也不只对应未捕获异常，服务端可以在业务失败时主动中止。[核对：Humble GoalStatus 定义](https://github.com/ros2/rcl_interfaces/blob/humble/action_msgs/msg/GoalStatus.msg)。

课堂提问：

> 取消服务已返回，机器人还在做停止清理，此时用哪个状态？

答案：`CANCELING`，清理完成后才进入 `CANCELED`。

这页最后可以总结：

> **已接受不等于已完成，正在取消不等于取消完成；终态与结果共同说明一次目标如何结束。**

---

## 第 22 页｜三种通信机制全景对比

这页你可以把它讲成：

> **现在把 Topic、Service 和 Action 放到一张表里，比较它们提供什么业务语义。不要用某个固定耗时数字来代替选型判断。**

| 对比维度 | Topic | Service | Action |
| --- | --- | --- | --- |
| 核心交互 | 消息发布与订阅 | 请求与响应 | 目标、反馈与结果 |
| 调用方式 | 发布与订阅回调 | 同步等待或异步 API | 常用异步 API，也有同步等待接口 |
| 业务边界 | 由消息和业务自行定义 | 一次调用 | 一个目标的生命周期 |
| 进度反馈 | 自行定义进度消息 | 单次响应中无独立进度通道 | 有标准 Feedback 通道 |
| 取消交互 | 自行定义取消协议 | 无内置任务取消协议 | 有标准取消请求与受理过程 |
| 状态管理 | 自行定义业务状态 | 自行管理内部业务状态 | 每个已接受目标有状态机 |
| ROS 层实体 | Topic | Service | 三个 Service 与两个 Topic |
| 常见用途 | 传感器、状态、事件 | 查询、设置、短操作 | 导航、轨迹、巡检、对接任务 |

### 1. 同步与异步不是选型的唯一标准

Service 和 Action 都可以使用异步 API。选型重点是需不需要任务受理、进度、取消和终态。

### 2. 不按毫秒、秒、分钟机械划分

```text
一个 0.5 秒任务，如果要求进度和取消，也可使用 Action
一个 2 秒查询，如果只是一次结果，也可能使用异步 Service
长时间持续运行的雷达，仍然使用 Topic 传数据
```

Service 应尽量短小，避免用长服务回调占住执行器。

### 3. 取消与抢占分开理解

Action 标准化了取消协议；“新目标抢占旧目标”需要服务端定义调度策略，第 25 页展开。

### 教师说明

PPT 表中的“完全异步”“默认同步”“原生自动抢占”“无状态”需要按上表解释。通信机制不限定设备是否保存状态，也不自动保证回调不阻塞。

这一页虽然已经做全景比较，仍保留在第二部分原来的第 22 页。第三部分会用决策树与场景继续练习。

课堂判断：

```text
每秒广播电机温度               → Topic
查询一次驱动故障码             → Service
机械臂执行一条可取消的轨迹     → Action
单次通知“有人按了按钮”         → 可以使用 Topic
```

这页最后可以总结：

> **三种机制表达不同的交互关系，业务的生命周期和确认需求比单纯的耗时更能决定选型。**

---

## 第 23 页｜动作编程模型

这页你可以把它讲成：

> **Action Server 负责决定接不接、能不能取消、具体怎样执行。Action Client 负责发送目标、接收反馈、请求结果，并在需要时申请取消。**

### 1. 服务端的核心回调

```text
goal_callback：校验目标，接受或拒绝
cancel_callback：判断取消请求是否允许
execute_callback：执行任务、发反馈、设终态、返回结果
```

如果需要延迟调度，还可自定义 `handle_accepted_callback`，在目标接受后安排何时执行。本课示例使用默认的立即执行方式。

### 2. 客户端的核心 API

Humble rclpy 中：

```python
goal_future = action_client.send_goal_async(
    goal, feedback_callback=handle_feedback
)

# goal_future 完成后得到 ClientGoalHandle，并检查 accepted
result_future = goal_handle.get_result_async()
cancel_future = goal_handle.cancel_goal_async()
```

这三个 Future 分别对应目标受理、任务结果和取消受理，不要混为一个“任务完成”信号。

反馈回调接收的是封装消息，用户 Feedback 在 `message.feedback` 中。

### 3. 自定义 .action 接口

PPT 本页没有完整列出 `.action` 文件，但为了衔接后页的编译，需要在这里补充定义。

创建目录：

```bash
mkdir -p ../ros2_ws/src/robot_interfaces/action
# 补齐 action、srv、msg 和生成配置（保留额外接口）
python3 "prepare_course.py" 2
```

保存为 `src/robot_interfaces/action/CountSteps.action`：

```text
# Goal：目标步数，服务端约定范围为 [1, 100]
int32 target_steps
---
# Result：最终完成步数与结束说明
int32 completed_steps
string message
---
# Feedback：当前完成步数
int32 current_step
```

再强调文件顺序：`Goal → Result → Feedback`，两个 `---` 都必须独占一行。

这里只使用基础字段，不需要额外引用 `std_msgs`。

生成后的 Python 使用：

```python
from robot_interfaces.action import CountSteps

goal = CountSteps.Goal()
goal.target_steps = 10
```

对应 C++ 头文件为：

```cpp
#include "robot_interfaces/action/count_steps.hpp"
```

### 4. 完整最小动作服务端

保存为 `第二课/课堂实验/count_steps_server.py`。先按第 24 页生成接口，再运行脚本。

```python
import threading
import time

import rclpy
from rclpy.action import ActionServer, CancelResponse, GoalResponse
from rclpy.callback_groups import ReentrantCallbackGroup
from rclpy.executors import MultiThreadedExecutor
from rclpy.node import Node

from robot_interfaces.action import CountSteps


class CountStepsServer(Node):
    def __init__(self):
        super().__init__('count_steps_server')
        self.group = ReentrantCallbackGroup()
        self.lock = threading.Lock()
        self.busy = False
        self.action_server = ActionServer(
            self,
            CountSteps,
            '/count_steps',
            execute_callback=self.execute,
            goal_callback=self.accept_goal,
            cancel_callback=self.accept_cancel,
            callback_group=self.group,
        )

    def accept_goal(self, request):
        if not 1 <= request.target_steps <= 100:
            self.get_logger().warning('拒绝目标：步数必须在 [1, 100] 内')
            return GoalResponse.REJECT
        with self.lock:
            if self.busy:
                self.get_logger().warning('拒绝目标：当前已有任务')
                return GoalResponse.REJECT
            self.busy = True
        return GoalResponse.ACCEPT

    def accept_cancel(self, goal_handle):
        self.get_logger().info('接受取消请求，等待执行逻辑完成清理')
        return CancelResponse.ACCEPT

    def execute(self, goal_handle):
        result = CountSteps.Result()
        result.completed_steps = 0
        try:
            for _ in range(goal_handle.request.target_steps):
                if goal_handle.is_cancel_requested:
                    result.message = '任务已取消，返回已完成步数'
                    goal_handle.canceled()
                    return result
                if not rclpy.ok():
                    result.message = '节点关闭，任务中止'
                    goal_handle.abort()
                    return result

                # 模拟一小步工作，不控制真实硬件
                time.sleep(0.2)
                result.completed_steps += 1
                feedback = CountSteps.Feedback()
                feedback.current_step = result.completed_steps
                goal_handle.publish_feedback(feedback)

            # 处理最后一步期间收到的取消请求
            if goal_handle.is_cancel_requested:
                result.message = '任务已取消'
                goal_handle.canceled()
            else:
                result.message = '全部步骤完成'
                goal_handle.succeed()
            return result
        except Exception as exc:
            result.message = f'执行异常：{exc}'
            if goal_handle.is_active:
                goal_handle.abort()
            return result
        finally:
            with self.lock:
                self.busy = False


def main():
    rclpy.init()
    node = CountStepsServer()
    executor = MultiThreadedExecutor(num_threads=3)
    executor.add_node(node)
    try:
        executor.spin()
    except KeyboardInterrupt:
        pass
    finally:
        executor.shutdown()
        node.action_server.destroy()
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == '__main__':
    main()
```

### 5. 逐段解释，不要求学生背完整代码

```text
accept_goal：拒绝非法步数，忙时拒绝新目标
accept_cancel：允许申请取消
execute：每步检查取消、完成工作、发布反馈
succeed / canceled / abort：明确结束方式
return result：交付用户结果数据
```

`busy` 是本课定义的单任务准入策略，Action 不会自动替我们设置它。锁保护可重入回调之间的共享状态。

示例每 0.2 秒完成一步，反馈约 5 Hz。取消通常要等当前这一小步结束后才能检测，调度延迟还会影响实际响应时间。

### 教师说明

PPT 的“执行回调自动在独立线程运行”不能当作 rclpy 的统一保证。Humble rclpy 将执行工作交给 Executor；本例显式使用多线程执行器与可重入组，让执行、取消等工作有并发机会。

`time.sleep()` 会占住所在工作线程，本例仅用于课堂模拟。真实项目可采用短步骤、非阻塞驱动或受控工作线程，并验证响应能力。不要把这里的多线程直接理解成 Python 密集计算获得线性加速。[核对：Humble ActionServer 实现](https://github.com/ros2/rclpy/blob/humble/rclpy/rclpy/action/server.py)。

课堂提问：

> `return result` 之前忘了 `goal_handle.succeed()`，能否认为目标自动成功？

答案：不能，应该明确设置终态；Humble rclpy 对仍处于活动状态的执行结束会按中止处理。

### 补充命令行对照实验

目的：看正常完成与忙时拒绝。A 运行本页服务器。B 发长目标并观察反馈，C 在它执行时发另一个目标：
```bash
# B
ros2 action send_goal /count_steps robot_interfaces/action/CountSteps "{target_steps: 30}" --feedback
# C：B 已显示 Goal accepted 且尚在执行时
ros2 action send_goal /count_steps robot_interfaces/action/CountSteps "{target_steps: 5}" --feedback
```
预期 B 有 current_step 反馈，C 被忙策略拒绝；B 最后成功且 completed_steps=30。待 B 终结后 C 再发 5 步，预期可接受。时机太晚会接收第二目标，应重新用长任务，不把偶然时序当自动抢占。

这页最后可以总结：

> **目标回调管准入，取消回调管受理，执行回调管过程与终态；执行器和回调组决定这些工作能否及时响应。**

---

## 第 24 页｜编译配置与调试工具

这页你可以把它讲成：

> **`.action` 与上一课的 `.msg`、刚才的 `.srv` 一样，要先生成接口。生成后再启动服务端，用 CLI 看名称、发送目标并观察反馈。**

### 1. package.xml 的 Action 依赖

在第 13 页已有配置基础上补充：

```xml
<depend>action_msgs</depend>
```

保留：

```xml
<buildtool_depend>ament_cmake</buildtool_depend>
<build_depend>rosidl_default_generators</build_depend>
<exec_depend>rosidl_default_runtime</exec_depend>
<member_of_group>rosidl_interface_packages</member_of_group>
```

Action 生成过程中会用到标准动作消息。其他外部消息包只在实际字段引用时加入依赖。

### 2. 在同一个生成调用中追加 .action

只包含本课 Service 和 Action 的包：

```cmake
cmake_minimum_required(VERSION 3.8)
project(robot_interfaces)

find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(action_msgs REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "srv/RobotCommand.srv"
  "action/CountSteps.action"
  DEPENDENCIES action_msgs
)

ament_export_dependencies(rosidl_default_runtime)
ament_package()
```

如果保留第一课的 `RobotStatus.msg`，则增加 `find_package(std_msgs REQUIRED)`，将 `"msg/RobotStatus.msg"` 加入同一个 `rosidl_generate_interfaces()`，并将依赖行写为：

```cmake
DEPENDENCIES std_msgs action_msgs
```

不要为新增 Action 删除已有 `.msg`、`.srv` 条目，也不要在同一个包里机械复制多份同名的生成调用。

### 3. 编译、加载与检查接口

```bash
(cd ../ros2_ws && colcon build --packages-select robot_interfaces)
source ../ros2_ws/install/setup.bash
ros2 interface show robot_interfaces/action/CountSteps
python3 -c "from robot_interfaces.action import CountSteps; print(CountSteps.Goal())"
```

生成产物包括 Goal、Result、Feedback 相关类型、内部服务与反馈封装类型、语言绑定及类型支持。[核对：Humble 自定义 Action 教程](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Intermediate/Creating-an-Action.rst)。

### 4. 启动服务端

终端 A：

```bash
source /opt/ros/humble/setup.bash
source ../ros2_ws/install/setup.bash
python3 第二课/课堂实验/count_steps_server.py
```

本课直接运行保存的脚本，因此不需要给脚本配置 `ros2 run` 入口。正式业务包应安装脚本并声明 `rclpy`、`robot_interfaces` 等依赖。

### 5. 查询动作名称、类型与节点

终端 C：

```bash
ros2 action list -t
ros2 action info /count_steps
ros2 node info /count_steps_server
```

`action info` 查看服务端、客户端及其节点信息，不等于已经确认某个任务成功。

### 6. 发送正常目标并显示反馈

终端 B 加载相同环境后：

```bash
ros2 action send_goal /count_steps robot_interfaces/action/CountSteps "{target_steps: 10}" --feedback
```

预期过程：

```text
目标受理，显示 Goal ID
反馈 current_step 从 1 增加到 10
结果 completed_steps=10
最终状态 SUCCEEDED
```

10 步按模拟耗时约 2 秒，实际时间受运行和调度影响。

再发送非法目标：

```bash
ros2 action send_goal /count_steps robot_interfaces/action/CountSteps "{target_steps: 0}" --feedback
```

预期目标被拒绝，而不是开始执行后再成功。

### 7. 查看状态与反馈隐藏话题

```bash
ros2 topic list --include-hidden-topics -t
ros2 service list --include-hidden-services -t
```

监听状态：

```bash
ros2 topic echo /count_steps/_action/status action_msgs/msg/GoalStatusArray --qos-durability transient_local --qos-reliability reliable
```

监听反馈：

```bash
ros2 topic echo /count_steps/_action/feedback robot_interfaces/action/CountSteps_FeedbackMessage
```

先启动反馈监听，再发目标；默认反馈不承诺补发监听前的数据。状态数组中用 Goal UUID 区分每次任务。

### 8. PPT 命令需要修正的地方

课件的 `ros2 action echo /act` 不是 Humble 标准 `ros2action` 已注册的子命令。应改用 `send_goal --feedback` 或 `ros2 topic echo` 监听隐藏话题。

`ros2 interface show Act` 也不完整，应该填写完整类型 `robot_interfaces/action/CountSteps`。课件的 `/act`、`/node` 都是占位名称，实际使用本课统一名称。

可以现场查看当前工具支持：

```bash
ros2 action --help
ros2 action send_goal --help
```

[核对：Humble ros2action 子命令注册](https://github.com/ros2/ros2cli/blob/humble/ros2action/setup.py)。

### 验收标准

```text
能导入 CountSteps
能发现 /count_steps
正常目标受理并收到反馈
得到正确 completed_steps 与 SUCCEEDED
非法目标被拒绝
```

课堂提问：

> 为什么 `ros2 action list` 看到了动作，普通 `topic list` 却没有显示它的反馈话题？

答案：内部通道使用隐藏名称，要包含隐藏话题再查看。

这页最后可以总结：

> **先生成接口，再运行服务端，再用完整名称与完整类型验证目标、反馈和结果。**

---

## 第 25 页｜动作进阶特性：可取消与可抢占

这页你可以把它讲成：

> **取消是客户端提出“请停止这个目标”；抢占是服务端按照任务调度策略决定“先处理另一个目标”。Action 提供取消协议，具体怎样停止、怎样切换任务，由业务实现决定。**

### 1. 取消的完整过程

```text
客户端 cancel_goal_async()
        ↓
服务端取消回调决定是否允许
        ↓
取消响应返回受理信息
        ↓
执行回调检测 is_cancel_requested
        ↓
停止工作，释放占用资源
        ↓
goal_handle.canceled() 并返回 Result
```

如果服务端没有注册允许取消的回调，Humble rclpy 默认取消回调会拒绝取消。本课示例显式返回 `CancelResponse.ACCEPT`。

如果服务端虽然接受，但执行逻辑不检查标志，任务仍可能继续执行。取消能力需要协议与执行逻辑共同完成。

### 2. 保存一个可复现的取消客户端

以下脚本提交 30 步任务，受理约一秒后申请取消，再取得最终结果。所有等待位于脚本主流程中，没有在 ROS 回调内同步等响应。

保存为 `第二课/课堂实验/count_steps_cancel_client.py`：

```python
import time

import rclpy
from rclpy.action import ActionClient
from rclpy.node import Node
from action_msgs.msg import GoalStatus

from robot_interfaces.action import CountSteps


def main():
    rclpy.init()
    node = Node('count_steps_cancel_client')
    client = ActionClient(node, CountSteps, '/count_steps')

    def feedback_callback(message):
        step = message.feedback.current_step
        node.get_logger().info(f'已完成 {step} 步')

    try:
        if not client.wait_for_server(timeout_sec=3.0):
            node.get_logger().error('动作服务端不可用')
            return

        goal = CountSteps.Goal()
        goal.target_steps = 30
        goal_future = client.send_goal_async(
            goal, feedback_callback=feedback_callback
        )
        rclpy.spin_until_future_complete(node, goal_future, timeout_sec=3.0)
        if not goal_future.done():
            node.get_logger().error('受理响应超时，目标是否受理尚不确定')
            return

        goal_handle = goal_future.result()
        if not goal_handle.accepted:
            node.get_logger().warning('目标被拒绝')
            return

        result_future = goal_handle.get_result_async()
        deadline = time.monotonic() + 1.0
        while rclpy.ok() and not result_future.done():
            if time.monotonic() >= deadline:
                break
            rclpy.spin_once(node, timeout_sec=0.1)

        if not result_future.done():
            cancel_future = goal_handle.cancel_goal_async()
            rclpy.spin_until_future_complete(
                node, cancel_future, timeout_sec=3.0
            )
            if cancel_future.done():
                response = cancel_future.result()
                ids = [info.goal_id for info in response.goals_canceling]
                if goal_handle.goal_id in ids:
                    node.get_logger().info('取消已受理，继续等待最终结果')
                else:
                    node.get_logger().warning('目标未进入取消中，继续查询结果')
            else:
                node.get_logger().warning('取消响应超时，取消是否受理尚不确定')

        rclpy.spin_until_future_complete(
            node, result_future, timeout_sec=3.0
        )
        if not result_future.done():
            node.get_logger().error('最终结果超时，任务结束状态尚未确认')
            return

        response = result_future.result()
        status_names = {
            GoalStatus.STATUS_SUCCEEDED: 'SUCCEEDED',
            GoalStatus.STATUS_CANCELED: 'CANCELED',
            GoalStatus.STATUS_ABORTED: 'ABORTED',
        }
        name = status_names.get(response.status, str(response.status))
        node.get_logger().info(
            f'终态={name}，完成步数={response.result.completed_steps}，'
            f'说明={response.result.message}'
        )
    except Exception as exc:
        node.get_logger().error(f'动作调用异常：{exc}')
    finally:
        client.destroy()
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == '__main__':
    main()
```

保持第 23 页服务端运行，终端 B 执行：

```bash
source ../ros2_ws/install/setup.bash
python3 第二课/课堂实验/count_steps_cancel_client.py
```

预期终态为 `CANCELED`，完成步数小于 30。具体停止在第几步由调度时机决定，不要求恰好为 5。

取消受理后还要读 `result_future`，这一步才确认最终状态。如果运行环境异常导致超时，示例明确记录未知结果，而不声称服务端已经停止。

### 3. 取消与正常结束可能竞争

任务刚好结束时再申请取消，取消请求可能未获准，最终结果也可能是成功。这时客户端应该按实际终态处理。

Humble 的 `ros2 action send_goal` 实现也支持在目标受理后通过 Ctrl+C 尝试取消；课堂核心演示用上面的 Python 客户端，可以明确检查取消响应和最终结果。[核对：Humble send_goal 实现](https://github.com/ros2/ros2cli/blob/humble/ros2action/ros2action/verb/send_goal.py)。

### 4. 抢占是服务端策略

收到新目标时，可以设计：

```text
策略 A：忙时拒绝新目标
策略 B：新目标排队，等待旧目标结束
策略 C：允许多个独立目标并行
策略 D：新目标替换旧目标，先停止并清理旧任务
```

本课实现的是策略 A。它支持取消，但不会因为新目标到达就抢占当前目标。

若要实现优先级抢占，需要明确优先级来源、资源归属、旧目标如何终结，以及新目标何时可以执行。基础 Action API 不会自动比较优先级或发起“强制取消”。

### 教师说明

客户端取消协议与服务端主动中止不同。服务端因调度或故障停止目标，常用 `abort()`；需要报告取消完成时，应遵守目标的取消状态流转，不能在任意状态直接调用 `canceled()`。

软件 Action 取消不等于硬件瞬时停止。任务停止过程应按照设备与业务要求实现，不能把取消回执直接当成硬件已经停止的证明。

课堂提问：

> 当前任务正在执行，另一个客户端发来新目标，本课服务端会怎样做？

答案：因为 `busy=True`，拒绝新目标；不会自动抢占。

### 补充命令行对照实验

目的：尽可能用 CLI 演示取消，同时确认终态。B 发 30 步目标并带 feedback；目标明确受理后，在完成前按 Ctrl+C。Humble send_goal 会尝试发取消，不把终端退出当取消成功。
```bash
# C：先观察协议状态，再让 B 取消
ros2 topic echo /count_steps/_action/status action_msgs/msg/GoalStatusArray
```
预期目标经历执行、取消处理到 CANCELED；具体停止步数依时序。若 CLI 未拿到最终结果，按本页完整取消客户端复现并读 Result，而不是宣布“已停”。结束 C；用本页 Python 客户端作为有响应与结果检查的确定性验收。

这页最后可以总结：

> **取消是受理后协作停止的过程，抢占是业务调度策略；两者都要用最终状态和结果完成闭环。**

---

## 第 26 页｜动作最佳实践与常见问题

这页你可以把它讲成：

> **长任务运行时，我们既要关心任务能否做完，也要关心出问题时能否及时反馈、结束和释放资源。**

### 1. Goal 保持必要且完整

目标要能独立说明任务要求，例如坐标、步数、允许误差或业务选项。

不要把整幅图像或不断变化的传感器数据塞进每个 Goal；高频数据仍可通过 Topic 传递。

### 2. Feedback 轻量、频率有依据

UI 显示进度时，5～10 Hz 可作为起点，实际按任务变化速度和网络负载调整。低频阶段变化也不必强行高频推送。

```text
Feedback：阶段、进度、剩余距离
Topic：图像、点云、连续状态数据
```

不需要的每一步内部调试信息，可以放日志，而不全部作为反馈发送。

### 3. 执行逻辑定期检查取消

```text
检查取消
    ↓
完成一个有界的小步骤
    ↓
发布反馈
    ↓
再次检查取消
```

如果一次步骤持续十秒，取消检查也可能要等十秒。异步客户端不能补救服务端执行逻辑迟迟不检查取消的问题。

### 4. 处理所有出口

客户端至少处理：

```text
服务端不可用
目标被拒绝
目标受理响应超时
任务成功
任务取消
任务中止
最终结果等待超时
```

失败、拒绝和未知状态不是同一件事，要给上层不同的处理依据。

服务端在每条任务结束路径清理资源，并返回结构化结果；工程中还要限制并发任务和结果缓存，避免资源持续增长。

### 5. 常见问题排查

| 现象 | 优先检查 |
| --- | --- |
| 找不到动作 | 服务端进程、完整名称、Domain、网络、环境 |
| 目标受理后无进度 | 执行回调是否运行、是否发布反馈、客户端是否注册回调、QoS |
| 点击取消却持续执行 | 取消是否受理、取消回调是否有机会执行、执行逻辑是否检查标志 |
| 任务看似结束却等不到结果 | 是否设置终态并返回 Result、执行器是否处理结果请求 |
| 新目标莫名被拒绝 | 准入策略、忙标志、资源清理和日志 |
| 客户端仍显示旧状态 | Goal UUID 是否对应、状态更新时间、是否用最终结果更新界面 |

### 6. 结合时间线定位

建议日志至少关联：

```text
Goal UUID 与业务任务编号
目标提交与受理时间
开始执行时间
取消请求与受理时间
进入终态时间
最终结果与错误原因
```

不要仅凭“当前没有反馈”断言目标已经失败；可能只是当前阶段不发布反馈或客户端未收到。

### 7. 性能优化先测量

分别观察目标受理延迟、反馈间隔、取消响应时间、任务计算耗时和带宽。

减少反馈载荷只能解决一部分网络问题；如果执行器线程被长回调占满，应先调整执行方式。复用 ActionServer 和 ActionClient 对象，避免为每个目标反复创建全部通信实体。

课堂故障题：

> 取消回调日志能打印，但任务不停，最可能是哪一层出了问题？

答案：先看取消是否真正被接受，再看执行循环是否检查取消并设置终态。

### 补充命令行对照实验

目的：区分客户端消失与服务器任务停止。保持 A 服务端，C 观察 status，B 运行：
```bash
timeout --signal=TERM 2s ros2 action send_goal /count_steps robot_interfaces/action/CountSteps "{target_steps: 30}" --feedback
```
TERM 用于结束 CLI 进程而不走 Ctrl+C 取消路径；若目标已受理，A 仍可能完成任务，C 观察同一 Goal UUID 的后续状态。timeout=124 不表示服务端取消成功。目标未在两秒内受理时结果未知，需重做并先确认 accepted。只用于软件计数示例，结束后等待目标终态，别立即用新目标推断空闲。

这页最后可以总结：

> **目标准入、过程反馈、取消检查、终态结果和资源清理，都要在正常与异常路径中有明确行为。**

---

## 第 27 页｜第三部分：三种通信机制对比与选型

这是一张章节过渡页，可以这样讲：

> **前面已经知道三种机制各自能做什么，现在回到系统设计。一个机器人通常同时需要数据流、短请求和长任务，所以选型要针对每一条交互分别判断。**

板书：

```text
感知数据：持续产生，多个消费者
设备设置：一次操作，需要响应
任务执行：有开始、过程和结束
```

问学生：

> 导航系统使用了 Action，是不是就不再需要 Topic？

答案：仍然需要雷达、里程计、速度等数据话题，Action 管理导航目标的生命周期。

这页最后可以说：

> **我们按每一条数据和每一次业务交互来选型，再把它们组合成系统。**

---

## 第 28 页｜选型决策树

这页你可以把它讲成：

> **先问数据是不是持续流动，再问是否需要明确响应，最后问有没有任务进度、取消和终态需求。把业务问题问清楚，机制就容易选择。**

### 1. 是否需要跨节点通信

如果同一模块内部直接调用函数已经满足需求，不必为每一个内部函数都创建 ROS 接口。

跨节点通信则继续判断。

### 2. 是持续数据流吗

```text
图像、扫描、IMU、里程计 → Topic
```

如果同时要设置设备或触发任务，可以在 Topic 之外增加 Service 或 Action。

PPT 的“需知道订阅结果”要追问是什么结果：

```text
只想知道有没有订阅端 → 可检查图信息，但不代表业务处理完成
想知道某次操作是否完成 → 需要定义 Service/Action 或明确的确认协议
```

给系统加一个普通 Service，不会自动让每一条 Topic 消息都有业务回执。

### 3. 单次交互是否要求结果

单次通知，例如“检测到某事件”，可以用 Topic。

一次查询或设置，要求明确返回值，适合 Service。

### 4. 是否需要任务生命周期

如果需要目标受理、过程反馈、中途取消和最终状态，优先考虑 Action。

耗时较长而只需要一次响应的操作，可以评估异步 Service，但要考虑服务端执行器占用、超时与结果恢复。如果未来还要加入取消或进度，Action 通常更合适。

板书决策树：

```text
需要跨节点通信？
├─ 否：本地函数或模块接口
└─ 是
   ├─ 持续数据流：Topic
   │  └─ 还需要设备设置或任务：组合 Service / Action
   └─ 单次交互
      ├─ 只通知：Topic
      └─ 需要结果
         ├─ 简短查询或操作：Service
         └─ 需要任务过程、取消或终态管理：Action
```

### 课堂分组选型

给学生四个需求，要求同时说出理由：

```text
1. 连续显示电池电量
2. 设置相机曝光并返回设置结果
3. 巡检五个点位并允许中途停止
4. 按键事件通知多个节点
```

参考答案：Topic、Service（实际设备也可能使用参数机制）、Action、Topic。

参数属于节点配置机制，本课重点比较三种通信，但不能因为表里没列参数，就把所有配置都重新写成自定义服务。

### 教师说明

PPT 的“长耗时但无进度就选异步服务”应加上取消与任务状态需求一起判断。没有进度反馈的长任务也可以使用 Action，Feedback 不是强制必须填满的业务功能。

这页最后可以总结：

> **数据流用 Topic，一次问答用 Service，需要完整任务交互时用 Action，再按系统需求组合。**

---

## 第 29 页｜典型场景与组合使用模式

这页你可以把它讲成：

> **整机系统的通信不会只用一种机制。我们让数据通道持续运行，让短请求处理设置，让任务接口协调长过程。**

### 1. Action + Topic：导航系统

板书：

```text
雷达 ── /scan ───────────→ 感知与导航节点
里程计 ── /odom ─────────→ 定位与导航节点
任务管理器 ── 导航 Action → 导航服务端
导航控制 ── /cmd_vel ────→ 底盘驱动
```

这里的 Topic 名称是常见示意，实际按所用导航系统配置核对。

Action 管目标、进度与结果，Topic 管运行过程中的传感器和控制数据。取消导航后，系统应按控制策略结束运动输出，而不是认为销毁客户端就会停车。

### 2. Service + Topic：视觉系统

```text
配置请求 ── Service / 参数 ─→ 相机驱动
相机驱动 ── 图像 Topic ─────→ 识别、显示、记录节点
```

相机是否成功应用配置，需要在响应含义中约定。图像持续传输与一次配置确认分别处理。

大量图像不放在短服务响应里反复传递。

### 3. Action + Service：机械臂系统

```text
轨迹执行与结果管理  → Action
一次查询或短时设置  → Service / 参数
关节状态持续输出    → Topic
```

本页 PPT 把“急停”列作服务场景，课堂要明确它只是可能存在的软件停止请求。真正有安全要求的急停不能仅凭 ROS 服务 Reliable 配置或成功回执来保证，应由设备及系统的安全设计承担。

软件停止请求也要约定：响应表示请求已受理，还是运动已经停止？这两者不能混用。

### 4. 三者混合：巡检机器人

用一个综合案例让学生组织通信：

```text
传感器节点
  └─ Topic：扫描、图像、电池、里程计

设备驱动节点
  └─ Service / 参数：一次查询、灯光设置、配置

任务管理节点
  └─ Action：按顺序完成多个巡检点

导航节点
  └─ Action：单个目标点导航
```

任务管理器调用下级导航 Action 时，要让流程异步推进，不要在一个长回调里堵住整个节点。

上层巡检被取消时，应明确向当前下级目标传播取消，并等待所需的停止确认。取消上层目标不会自动取消所有下级目标。

### 5. 按业务环节推演

```text
巡检客户端提交路线
    ↓
任务管理器受理并准备任务
    ↓
向导航端发送当前点位目标
    ↓
传感器 Topic 持续运行
    ↓
上报“当前第几个点位”的巡检 Feedback
    ↓
各点位完成后返回巡检 Result
```

如果导航失败，上层要决定重试、跳过或中止；如果取消，先停止当前执行环节并完成资源清理。

### 6. 对 PPT 场景表的补充解释

| PPT 场景 | 教师解释 |
| --- | --- |
| 雷达、IMU、图像、GPS | 连续数据适合 Topic；雷达输出可能是扫描或点云，按实际接口类型选择 |
| 心跳与状态同步 | 可用 Topic，但是否容忍丢包和如何判断失联由业务要求决定 |
| 指令、配置、查询 | 按需用 Service 或参数机制，并明确响应含义 |
| 导航、轨迹、充电对接 | 有任务过程与取消需求时适合 Action |
| 地图构建 SLAM | 连续建图算法通常持续使用 Topic；“启动并完成一次建图流程”可由 Action 管理 |

不要把“地图构建”这个业务名直接当成它所有内部通信的选型答案。

### 7. 性能和扩展性的课堂补充

沿本页组合案例说明：

```text
高带宽数据与任务反馈分开
不同机器人用命名空间区分接口
复用现有传感器和运动接口
限制并发目标和在途请求
按实际瓶颈检查网络、执行器与计算耗时
```

优先降低无意义的重复数据与高频反馈，避免只增加线程却不处理回调组和共享硬件资源。

课堂提问：

> 使用 Action 管理巡检后，摄像头图像是否应该改成 Feedback 发送？

答案：一般仍用图像 Topic；Feedback 报告任务阶段、点位与进度。

这页最后可以总结：

> **Topic 承载持续数据，Service 承载短请求，Action 管理任务；组合接口时还要定义结果与取消怎样跨层传递。**

---

## 第 30 页｜感谢学习

这页你可以把它讲成：

> **今天虽然是理论为主，但每一个原理都要能对应到一个行为：为什么请求能找到自己的响应，为什么异步调用还需要执行器，为什么取消回执后还要等待终态。**

### 1. Service 的知识收束

学生应该能解释：

```text
Client 发 Request，Server 返回 Response
名称与类型约定服务接口
Service 可以异步调用
Future 完成后才能使用响应
回调内同步等待可能阻塞响应处理
.srv 用一行 --- 分开请求和响应
```

### 2. Action 的知识收束

学生应该能解释：

```text
Goal 规定输入，Feedback 描述过程，Result 交付输出
.action 文件顺序是 Goal / Result / Feedback
Action 由三个服务与两个话题组成
目标受理不代表任务完成
取消受理不代表取消完成
抢占由服务端策略实现
终态是成功、取消或中止
```

### 3. 两条开发链路

```text
服务：设计输入输出 → 写 .srv → 配置生成 → 编译与 source
      → 启动 Server → CLI 调用 → Client 使用结果

动作：设计目标结果反馈 → 写 .action → 配置生成 → 编译与 source
      → 启动 ActionServer → 验证受理/反馈/结果/取消
```

### 4. 回到第一页面的问题

现在再问：

> 导航任务用一个异步服务是否就够了？

参考回答：如果仅需一次最终响应，可以评估异步服务；如果要标准的进度、取消和生命周期管理，Action 更适合。

再问：

> 取消响应显示已经受理，为什么还要等待结果？

参考回答：受理后执行逻辑还要停止和清理，只有最终状态与结果才能确认任务怎样结束。

### 课后练习一：验证自定义 Service

沿第 12～14 页完成：

```text
定义并编译 RobotCommand.srv
source 工作空间
查看接口并运行服务端
发送合法和非法请求
区分通信响应与业务 success
```

要求提交两次响应记录，说明失败是输入检查导致，还是没有收到响应。

### 课后练习二：验证 Action 的四条路径

沿第 23～25 页完成：

```text
正常目标：10 步，最终 SUCCEEDED
非法目标：0 步，提交时拒绝
取消目标：30 步，途中取消，检查 CANCELED 和部分结果
忙时目标：第一个任务未结束时提交第二个，检查拒绝行为
```

扩展练习：给 `CountSteps` 的 Goal 增加 `int32 fail_at_step`，约定 0 表示不模拟失败。服务端在指定步骤主动 `abort()`，验证 `ABORTED` 与业务失败说明。

接口变化后重新生成，让服务端和客户端加载相同版本。新增字段不会自动获得跨版本兼容。

### 课后练习三：为一个机器人功能选型

为“相机巡检机器人”列出：

```text
图像与里程计如何传递？
设备配置如何确认？
一次巡检如何受理、反馈、取消和结束？
取消上层巡检时，下层导航怎样停止？
```

要求说明每个接口选择的理由，而不是只写 Topic、Service、Action 三个名称。

### 这门课最后一句话

> **先明确交互需要，再选择通信机制；请求看响应，任务看终态与结果，出现问题就沿接口、执行器和业务逻辑逐层排查。**

---

## 教师核对资料

使用 rosdep 前，先初始化并更新规则；已初始化时跳过 init。


以下是讲义技术说明与命令的官方核对入口。课堂环境统一以 Humble 为准，不把其他发行版新增工具当作本课必备命令。

- [ROS 2 Humble：同步与异步服务调用](https://github.com/ros2/ros2_documentation/blob/humble/source/How-To-Guides/Sync-Vs-Async.rst)
- [ROS 2 Humble：回调组与死锁规避](https://github.com/ros2/ros2_documentation/blob/humble/source/How-To-Guides/Using-callback-groups.rst)
- [ROS 2 Humble：创建自定义 msg/srv](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Beginner-Client-Libraries/Custom-ROS2-Interfaces.rst)
- [ROS 2 Humble：Python Service 与 Client](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Beginner-Client-Libraries/Writing-A-Simple-Py-Service-And-Client.rst)
- [ROS 2 Humble：QoS 设置](https://github.com/ros2/ros2_documentation/blob/humble/source/Concepts/Intermediate/About-Quality-of-Service-Settings.rst)
- [ROS 2 Action 设计文档](https://design.ros2.org/articles/actions.html)
- [ROS 2 Humble：创建自定义 Action](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Intermediate/Creating-an-Action.rst)
- [ROS 2 Humble：Python Action Server 与 Client 教程](https://github.com/ros2/ros2_documentation/blob/humble/source/Tutorials/Intermediate/Writing-an-Action-Server-Client/Py.rst)
- [ROS 2 Humble：GoalStatus 标准状态码](https://github.com/ros2/rcl_interfaces/blob/humble/action_msgs/msg/GoalStatus.msg)
- [ROS 2 Humble：目标状态迁移实现](https://github.com/ros2/rcl/blob/humble/rcl_action/src/rcl_action/goal_state_machine.c)
- [ROS 2 Humble：ActionServer 源码](https://github.com/ros2/rclpy/blob/humble/rclpy/rclpy/action/server.py)
- [ROS 2 Humble：ActionClient 与 GoalHandle 源码](https://github.com/ros2/rclpy/blob/humble/rclpy/rclpy/action/client.py)
- [ROS 2 Humble：Service Client 源码](https://github.com/ros2/rclpy/blob/humble/rclpy/rclpy/client.py)
- [ROS 2 Humble：ros2action 已注册子命令](https://github.com/ros2/ros2cli/blob/humble/ros2action/setup.py)
- [ROS 2 Humble：send_goal 的反馈与取消实现](https://github.com/ros2/ros2cli/blob/humble/ros2action/ros2action/verb/send_goal.py)

教师使用前在目标 WSL 环境按上面的验收步骤预演。本文中的完整 Python 脚本按 Humble API 编写；静态语法检查不能替代实际 ROS 2 通信测试。
