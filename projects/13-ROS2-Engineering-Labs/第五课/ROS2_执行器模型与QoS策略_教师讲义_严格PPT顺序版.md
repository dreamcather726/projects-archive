# ROS 2 执行器模型与QoS策略：逐页教师讲义

对应课件：`2026.8.13 ROS2 执行器模型与 QoS 策略深度解析.pptx`，共 27 页。

严格一页对应一节；讲稿、命令行实验与技术修正均放在原页面位置。

## 上课前准备

### 执行目录与课前准备

**本课所有命令均从整套讲义的根目录执行**，也就是同时能看到 `第一课`、`第二课`、`第三课` 等文件夹的位置。请先在 WSL Ubuntu 的 Bash 中进入此目录；每个新终端都保持在该目录。若终端已位于 `第五课`，先执行 `cd ..` 回到讲义根目录。

Python 程序直接使用已提供的 `第五课/课堂实验/` 文件，无需复制到另一份脚本目录。第三课配置直接使用 `第三课/课堂实验/config/`。只有需要 colcon 构建的 ROS 包才由准备脚本写入旁边的 `../ros2_ws/src/`，构建在子 Shell 中进行，完成后当前终端仍位于讲义根目录。

```bash
source /opt/ros/humble/setup.bash
test -d "第五课/课堂实验" || { echo "请先进入整套讲义根目录"; exit 1; }
mkdir -p tf_debug
sudo apt update && sudo apt install -y ros-humble-rclpy ros-humble-std-msgs python3-colcon-common-extensions python3-rosdep
```

节点持续运行时，后面的查询命令应在另一个终端执行。各终端使用相同 ROS_DOMAIN_ID，先停止旧实验再启动新实验。

所有 Bash 命令都在 WSL Ubuntu 22.04、ROS 2 Humble 中执行。每个新终端先 `source /opt/ros/humble/setup.bash`；使用本地包时再 `source ../ros2_ws/install/setup.bash`。三个终端的 ROS_DOMAIN_ID 一致。

课堂终端：A 运行生产者/主体进程，B 控制或接收，C 查询与记录。持续命令用 `Ctrl+C` 结束；同一个终端不能在前台进程运行时继续执行下一条命令。实验切换前停止上个实验的发布者，避免同名话题、节点或 TF 混入。

随讲义交付的完整源码在 `第五课/课堂实验/`，直接使用这些文件。

前置知识：rclpy、Timer、QoS 基础与多终端操作。准备完整脚本：
```bash
python3 -m py_compile 第五课/课堂实验/executor_demo.py 第五课/课堂实验/qos_probe.py
python3 -c "import rclpy; from rclpy.qos_event import PublisherEventCallbacks, SubscriptionEventCallbacks"
python3 第五课/课堂实验/executor_demo.py --help
python3 第五课/课堂实验/qos_probe.py --help
```
统一节点 /executor_probe、/qos_pub、/qos_sub；话题 /executor/fast 与 /qos/probe。脚本直接运行，无需新建包。executor_probe 快周期 0.1s、慢周期 1s、默认慢任务 0.6s；qos_probe 打印编号和同机 monotonic 样本年龄，不能用于跨机器延迟测量。

---

## 第 1 页｜ROS2 执行器模型与 QoS 策略深度解析

这页你可以把它讲成：

> **今天把“消息已经到达”和“业务已经处理”分开：执行器控制回调何时执行，QoS 控制通信规则。**

### 讲解与例子

承接前课容器和生命周期，使用两个定时器的最小程序观察慢回调是否挡住快回调，再用带编号的消息观察匹配、缓存与时效。一个话题没数据可能是发现、QoS 或调度问题，不能只靠换线程数碰运气。

### 板书

```text
通信匹配 → 消息就绪 → 执行器调度 → 用户回调
```

### 课堂提问

> 多线程能修复 QoS 不兼容吗？

参考答案：不能，它们是不同环节。

这页最后可以总结：

> **先定位通信还是调度，再改变配置。**

---

## 第 2 页｜课程目录

这页你可以把它讲成：

> **前半做回调调度实验，后半做通信策略实验，两部分都用正常和异常对照。**

### 讲解与例子

依次讲执行器职责、单/多线程、回调组、组合与排查；再讲 QoS 目的、历史、可靠性、持久性、死线、寿命、兼容与选型。每个结论都指向一条可观察日志或查询结果。

### 板书

```text
执行器 → 组 → 并发
QoS → 兼容 → 数据行为
```

### 课堂提问

> 为什么要保持同一份源码比较？

参考答案：减少程序差异，只改变执行器和分组，才能解释因果。

这页最后可以总结：

> **实验控制变量与知识顺序一致。**

---

## 第 3 页｜执行器模型与回调并发控制

这页你可以把它讲成：

> **把执行器看成调度员，把定时器和订阅看成等待处理的事项。声明任务并不等于执行任务。**

### 讲解与例子

调度员等待就绪对象，再根据线程与分组许可执行回调。我们先用日志观察，不提前把操作系统的线程优先级和实时调度混进来。

### 板书

```text
创建对象 → 等待就绪 → 许可检查 → 执行回调
```

### 课堂提问

> 创建 Timer 后不 spin 能自动执行 Python 回调吗？

参考答案：本课 rclpy 示例不会，需执行器处理就绪事件。

这页最后可以总结：

> **实体和执行机制需要分别准备。**

---

## 第 4 页｜为什么需要执行器

这页你可以把它讲成：

> **先启动一个创建了 Timer 却不 spin 的节点，观察“对象存在但业务不执行”。**

### 讲解与例子

脚本 --executor none 创建两个 Timer 和 Publisher，随后等 5 秒退出；没有 FAST 和 SLOW 回调日志。DDS 图仍可能发现节点，但定时器就绪并不自动执行。再运行 single 模式，事件处理开始，形成对照。

### 板书

```text
无 spin：节点可存在，回调不处理
有 spin：等待与执行闭环
```

### 命令行实验

A 运行不 spin 的模式，B 在其 5 秒存活窗口查询：
```bash
# A
python3 第五课/课堂实验/executor_demo.py --executor none
# B
ros2 node list
ros2 topic info /executor/fast
```
预期 A 只打印“No spin”，没有 FAST/SLOW；对象可能已发现。A 自动退出后再以 --executor single 启动，观察回调出现。查询慢于 5 秒导致节点消失时重新运行；不要据此否定原理。

### 课堂提问

> node list 能看到对象就证明回调在工作吗？

参考答案：不能，必须观察业务日志或输出。

这页最后可以总结：

> **运行对象的存在与回调执行不是同一个证据。**

---

## 第 5 页｜执行器四大核心职责

这页你可以把它讲成：

> **监听、调度、线程管理是理解入口，但默认执行器不能承诺任意优先级、公平性或严格 FIFO。**

### 讲解与例子

沿课件四项讲事件就绪检测、任务执行、工作线程与调度机会。实际默认执行器的行为由客户端实现、回调组与等待机制决定；不是操作系统时间片轮转，也不能宣称高频任务永不饿死。我们观察输出间隔，只说明当前程序的阻塞现象。

### 板书

```text
就绪检测 + 回调执行 + 线程与组许可
不等于：硬实时 / 任意优先级 / 严格 FIFO
```

### 课堂提问

> 哪个命令能直接给所有回调设置优先级？

参考答案：本课没有这样的通用 CLI，若要优先级需设计执行器、线程与系统调度。

### 教师说明

PPT 的 FIFO、权重和公平性表述过于绝对。参考 [Executor 说明](https://docs.ros.org/en/humble/Concepts/Intermediate/About-Executors.html)。

这页最后可以总结：

> **区分执行器能力与课件中的理想目标。**

---

## 第 6 页｜执行器与节点的关系

这页你可以把它讲成：

> **一个执行器可以调度多个节点，但同一节点不能随意交给多个执行器同时 spin。**

### 讲解与例子

程序中的 add_node 只是建立管理关系，spin 才开始处理。rclpy.spin 的便利入口可使用全局执行器，不必把它理解成每次都新建一个执行器对象。需要隔离调度时可有多个执行器，C++ 还可按回调组分配；不是一进程一执行器的硬限制。

### 板书

```text
Node 提供实体 → add_node → Executor.spin
进程可包含多个节点与执行器
```

### 课堂提问

> 想隔离两个慢模块，是否只能新开一台机器？

参考答案：可以考虑独立执行器或进程，但要结合语言、分组与资源设计。

这页最后可以总结：

> **明确管理关系，再决定调度边界。**

---

## 第 7 页｜执行器工作原理：事件循环与 wait-set

这页你可以把它讲成：

> **没有就绪事项时等待，有事项时处理；等待不会把慢业务回调自动变成异步任务。**

### 讲解与例子

按 PPT 五步解释收集实体、等待、唤醒、执行和循环。ROS wait-set 包含定时器与 guard condition 等，并不只是 DDS 订阅集合。类比 select/epoll 是为了理解多路等待，不是实现完全一致。等待会阻塞线程以避免空转，不能同时说它“完全无阻塞”。

### 板书

```text
收集 → wait → 唤醒 → 取就绪实体 → 执行 → 循环
```

### 课堂提问

> 回调里 sleep 会不会占用执行线程？

参考答案：会，等待 I/O 的业务回调仍占该线程，其他线程与组条件决定能否绕过。

这页最后可以总结：

> **事件等待高效不代表业务处理没有阻塞。**

---

## 第 8 页｜单线程执行器（SingleThreadedExecutor）

这页你可以把它讲成：

> **先做基线：快回调本应每 0.1 秒执行，但慢回调睡 0.6 秒，会看到间隔被拉长。**

### 讲解与例子

单线程同一时刻执行一个回调，慢回调占用线程时快 Timer 即使就绪也无法执行。串行减少同一执行器回调间的数据竞争，但程序仍可能有其他线程、外部资源或同步等待造成死锁；不是所有程序都天然无锁安全。

### 板书

```text
SLOW BEGIN → 等待 0.6s → SLOW END
期间 FAST 可能被延后
```

### 命令行实验

停止前一模式，A 启动；B 测输出，C 可看日志或 echo：
```bash
# A
python3 第五课/课堂实验/executor_demo.py --executor single --groups default
# B
ros2 topic hz /executor/fast
# C
ros2 topic echo /executor/fast
```
等待至少 5 个慢周期。预期 SLOW BEGIN/END 之间 FAST 被延后，gap 出现大于 0.1s 的样本。hz 是观察端统计，不直接等于回调延迟。结束 B/C 后停止 A。

### 课堂提问

> 把 Timer 周期写成 0.1 就保证 10Hz 吗？

参考答案：不保证，执行能力、回调耗时与系统负载影响实际间隔。

这页最后可以总结：

> **周期是需求，实测间隔是结果。**

---

## 第 9 页｜多线程执行器（MultiThreadedExecutor）

这页你可以把它讲成：

> **增加工作线程提供执行机会，但必须先确认组允许并发。**

### 讲解与例子

本示例显式用两个工作线程，避免不同实现的默认数量影响对照。sleep 模拟 I/O 等待，会释放 Python GIL，因此能观察分组后的改善；纯 Python CPU 密集循环通常不能凭两个线程获得双核计算加速。C++ 或释放 GIL 的扩展库另看实测。

### 板书

```text
线程池能力 × 回调组许可 × 业务线程安全
```

### 命令行实验

依次在 A 比较，每次 Ctrl+C 后再运行下一条；B 可持续 echo：
```bash
python3 第五课/课堂实验/executor_demo.py --executor multi --groups default
python3 第五课/课堂实验/executor_demo.py --executor multi --groups split
```
默认组时仍有长 gap；split 时 SLOW BEGIN 与 END 之间预期出现 FAST，且线程 ID 可不同。sleep 是 I/O 等待模拟，不能推出纯 Python CPU 循环加速比例。结束所有观察进程。

### 课堂提问

> 默认组不变，两个线程一定能并行吗？

参考答案：同一默认互斥组内仍不能重叠执行。

这页最后可以总结：

> **线程数只是并发条件之一。**

---

## 第 10 页｜两类执行器性能对比与选型

这页你可以把它讲成：

> **先用单线程和多线程默认组比较，再用多线程分组；不要只比较一个平均 Hz。**

### 讲解与例子

比较 FAST 最大间隔、日志是否穿插、线程 ID 和资源开销。多线程能让独立 I/O 等待和轻任务并行，却可能引入锁竞争与调度抖动。算法吞吐、尾部延迟和故障需求不同，不能由本实验推出所有机器人都应多线程。

### 板书

```text
同源码：single/default → multi/default → multi/split
```

### 命令行实验

验收表由学生填写：single/default、multi/default、multi/split 各运行 10 秒，记录最大 FAST gap、是否在慢回调期间出现 FAST、线程 ID。只比较三种配置，不改 --slow 或 Timer 周期。若差异不明显，确认慢周期被观察到以及没有多个 /executor_probe 同时运行。

### 课堂提问

> 默认组案例里性能没有明显改善，是否证明多线程无用？

参考答案：只说明当前分组限制并发，需分开组后再观察。

这页最后可以总结：

> **选择应建立在负载和分组条件上。**

---

## 第 11 页｜回调组：并发控制的核心单元

这页你可以把它讲成：

> **回调组表达“哪些任务不能重叠”，它不会自动替所有共享变量加锁。**

### 讲解与例子

互斥组约束组内回调不能同时执行；可重入组允许重叠。不同组之间也可并发，所以跨组共享变量依旧要保护。执行器调度回调时检查组的许可，不是必须把整组作为一个不可拆分任务执行。

### 板书

```text
同互斥组：禁止重叠
不同组 / 可重入：允许候选并发
共享状态：仍需明确保护
```

### 课堂提问

> 把写回调放互斥组，读回调放另一组就安全了吗？

参考答案：不一定，读写仍可能并发，要同步或使用快照等方案。

这页最后可以总结：

> **组管执行关系，锁管具体共享状态。**

---

## 第 12 页｜两类回调组详解

这页你可以把它讲成：

> **互斥适合需要串行访问的任务，可重入适合能够承受重叠执行的任务。**

### 讲解与例子

同组互斥不能保护其他线程的访问；可重入也不代表回调一定同时运行，还需工作线程与就绪事件。将 --slow 调到 1.4，让周期为 1 秒的慢回调可能尚未完成又开始，观察同一回调重入的风险。脚本只打印观察数据，不能照搬到未保护的硬件调用。

### 板书

```text
互斥：组内一次一个
可重入：可重复进入
线程数与就绪事件决定实际执行
```

### 命令行实验

A 运行可重入实验：
```bash
python3 第五课/课堂实验/executor_demo.py --executor multi --groups reentrant --slow 1.4
```
观察某次 SLOW END 前是否已有另一个 SLOW BEGIN，日志线程 ID 支持判断重叠。系统负载会影响重叠时机，不要求每轮日志完全相同。再停止 A，改 --groups split 对照：每个慢回调自身不重入，但快回调可能获得另一个线程。最后恢复 --slow 0.6。

### 课堂提问

> 共享变量只能用互斥组保护吗？

参考答案：还可用锁、原子操作、消息传递或不可变快照。

这页最后可以总结：

> **安全方案需覆盖所有访问者。**

---

## 第 13 页｜回调组工作机制与默认组

这页你可以把它讲成：

> **没有显式分组时，许多对象进入同一默认互斥组；这就是多线程仍像串行的常见原因。**

### 讲解与例子

沿四步讲创建组、实体绑定、许可检查、执行。保留组对象的引用，避免其生命周期不明确。本实验的 split 模式使用两个不同互斥组，也能并发，不必把所有任务放可重入组。

### 板书

```text
默认互斥组 → 同组串行
两个互斥组 + 多线程 → 可跨组并行
```

### 命令行实验

重复第 9 页的 default/split 对照，并打开完整源文件确认 split 创建的是两个 MutuallyExclusiveCallbackGroup。验收：无需可重入组也可以观察跨组并行；保留 self.groups 引用。不要把示例的日志保护锁误解成两个业务共享状态都已自动安全。

### 课堂提问

> 实现并发必须用 ReentrantCallbackGroup 吗？

参考答案：不是，两个不同互斥组也可跨组并发。

### 教师说明

PPT 的“必须可重入，三项缺一不可”应修正为：足够的线程、允许并发的分组和线程安全业务。

这页最后可以总结：

> **先分清组边界，再选择是否允许重入。**

---

## 第 14 页｜回调组进阶：四种组合模式

这页你可以把它讲成：

> **感知控制分组、读写分离、资源隔离和流水线，都是设计候选，不是配完组就获得实时性。**

### 讲解与例子

按 PPT 四种组合逐项分析：慢感知与快控制不同组；读写若共享数据仍需同步；不同执行器帮助隔离但系统线程优先级另设；算法阶段并发要注意缓冲、顺序和时间戳。用本课 split 模式模拟慢 I/O 和快控制，不把它称为真正硬实时控制实验。

### 板书

```text
业务依赖图 → 共享状态 → 组 → 线程/进程 → 测量
```

### 课堂提问

> 不同组的写任务和读任务会不会相互冲突？

参考答案：会，组间允许并发，必须保护共享状态。

这页最后可以总结：

> **并发设计从数据依赖开始。**

---

## 第 15 页｜常见并发问题与排查

这页你可以把它讲成：

> **先看程序是没 CPU、忙 CPU、没线程机会还是数据被覆盖，再选择诊断工具。**

### 讲解与例子

数据竞争需检查所有共享访问；死锁检查循环等待和同组同步服务调用；优先级反转需要系统线程优先级上下文；回调延迟检查慢任务占用与队列。线程 ID 和日志时间支持初步诊断，TSan 主要用于相应 C++ 构建，不能直接拿来分析这段 Python 业务代码。

### 板书

```text
现象 → 时间/线程/队列 → 瓶颈或等待关系 → 修复
```

### 命令行实验

用日志记录而非一眼看平均值。A：
```bash
python3 第五课/课堂实验/executor_demo.py --executor single --groups default 2>&1 | tee tf_debug/executor_single.log
```
独立 C 在观察窗口后查询：
```bash
grep -E 'SLOW|FAST' tf_debug/executor_single.log | tail -n 20
ps -eLo pid,tid,comm,args | grep executor_demo.py | grep -v grep
```
预期单线程业务回调串行，但进程可能另有 DDS 线程；“OS 线程数多”不证明回调正在并发。Ctrl+C 停止 A。

### 课堂提问

> topic hz 低能直接认定网络丢包吗？

参考答案：不能，执行器处理不及时、发布频率低或观察端负载也可能影响。

这页最后可以总结：

> **频率异常是线索，不能单独判定根因。**

---

## 第 16 页｜QoS 服务质量策略与选型

这页你可以把它讲成：

> **接下来保持业务简单，改变通信规则，观察“同名话题却收不到”与“晚加入仍能收到”。**

### 讲解与例子

把 QoS 理解为每对通信端点的提供与请求条件。回调组影响处理时机，QoS 影响匹配、缓存和传输行为。接下来使用独立 /qos/probe，避免旧演示话题混入。

### 板书

```text
提供能力 → 请求条件 → 兼容 → 样本行为
```

### 课堂提问

> QoS 是一个节点全局统一配置吗？

参考答案：通常按 Publisher/Subscription 等端点配置，同一节点可以有不同设置。

这页最后可以总结：

> **端点规则需要逐条检查。**

---

## 第 17 页｜为什么需要 QoS

这页你可以把它讲成：

> **传感器关注新鲜数据，地图关注晚加入同步，任务关注明确结果，这些需求不能靠一种固定规则兼顾。**

### 讲解与例子

讲课件可靠性、延迟、内存、带宽和持久性权衡，但不要用“可靠就必然慢”做绝对排序。TRANSIENT_LOCAL 的本地缓存通常在发布端存活期间存在，并非磁盘归档。ROS 1 也有队列与 latch 等机制，课件对旧框架的比较需要限定。

### 板书

```text
业务需求 → 可靠性/历史/持久性/时效 → 配置与实测
```

### 课堂提问

> 静态地图只发一次，普通新订阅者是否一定收到？

参考答案：不一定，要设计持久性与发布端存活条件。

这页最后可以总结：

> **QoS 选型始于数据用途。**

---

## 第 18 页｜历史策略 + 队列深度

这页你可以把它讲成：

> **我们让生产速度大于消费速度，用编号跳跃和样本年龄观察队列取舍。**

### 讲解与例子

KEEP_LAST 保留有限的最近样本；KEEP_ALL 也受中间件资源限制，不是无限磁盘日志或零丢失保证。发送与接收端都有历史管理，不能简单等同“发不出去”与“未确认”两种缓存。深度影响保留行为，不要求两端数值相等。

### 板书

```text
发布 10Hz → 处理每条 0.4s
小队列：更容易跳编号
大队列：可能积累样本年龄
```

### 命令行实验

先停止 executor 实验。A 10Hz 发布，B 每条处理 0.4 秒：
```bash
# A
python3 第五课/课堂实验/qos_probe.py pub --reliability best_effort --period 0.1
# B
python3 第五课/课堂实验/qos_probe.py sub --reliability best_effort --depth 1 --slow 0.4
```
观察 RECV seq 与 age；可能出现编号跳跃。B 停止后改 --depth 20 重启，观察队列样本年龄是否上升。发布/消费速率差固定。具体跳号受实现影响；若不明显延长窗口。age 使用同机 monotonic，只用于本机演示，不适合作跨机时钟比较。结束 A/B。

### 课堂提问

> 队列设得越大，数据一定越实时吗？

参考答案：不是，处理能力不足时更可能积压旧样本。

这页最后可以总结：

> **用编号和年龄一起判断缓存效果。**

---

## 第 19 页｜可靠性策略（Reliability）

这页你可以把它讲成：

> **先故意让接收端提出发布端提供不了的可靠性，再用一条配置变化修复它。**

### 讲解与例子

RELIABLE 提供确认重传，BEST_EFFORT 不提供同等送达保障。兼容规则是提供能力满足请求：可靠发布者可以服务尽力订阅者，尽力发布者不能满足可靠订阅者。可靠传输不等于业务执行成功，也不保证资源溢出或进程死亡时所有消息必达。

### 板书

```text
Pub BEST_EFFORT → Sub RELIABLE：不兼容
Pub RELIABLE → Sub BEST_EFFORT：此项兼容
```

### 命令行实验

A 发布尽力消息，B 请求可靠接收；C 对照：
```bash
# A
python3 第五课/课堂实验/qos_probe.py pub --reliability best_effort
# B
python3 第五课/课堂实验/qos_probe.py sub --reliability reliable
# C
ros2 topic info /qos/probe --verbose
```
预期无 RECV 或出现不兼容日志。B 停止后改 --reliability best_effort，预期开始收到。再可用 A reliable/B best_effort 验证反向兼容；每次先停旧 A/B。

### 课堂提问

> 发送端没有失败日志就能证明接收端拿到了吗？

参考答案：不能，需看端点配置和实际数据，任务还需结果确认。

这页最后可以总结：

> **可靠性检查先看请求与提供方向。**

---

## 第 20 页｜持久性策略（Durability）

这页你可以把它讲成：

> **只发送一条并保留发布进程，晚加入订阅者才能验证缓存；发布者已退出时实验条件已经改变。**

### 讲解与例子

TRANSIENT_LOCAL 让匹配的晚加入订阅者接收缓存，depth 决定样本数量。VOLATILE 不请求补发连接前历史，但仍有运行中队列。一个 transient 发布者配 volatile 订阅者可匹配实时流，却不应依赖它收到旧缓存。

### 板书

```text
先发一次 → 发布者存活 → 晚加入匹配订阅者 → 历史样本
```

### 命令行实验

使用脚本只发一条并保持 spin，避免 CLI 等订阅者条件改变实验。A：
```bash
python3 第五课/课堂实验/qos_probe.py pub --durability transient_local --depth 1 --count 1
```
等待 A 打印 SEND 和 Publishing paused，再在 B 启动：
```bash
python3 第五课/课堂实验/qos_probe.py sub --durability transient_local --depth 1
```
预期晚加入仍 RECV seq=0。B 结束，改 --durability volatile 对照，不应依赖收到旧样本；A 不再发新数据。C info --verbose 核对持久性。最后停止 A/B。

### 课堂提问

> TRANSIENT_LOCAL 能把节点重启前数据永久保存吗？

参考答案：不能，本实验缓存属于存活发布端。

这页最后可以总结：

> **缓存实验需要固定发布端生命周期。**

---

## 第 21 页｜死线策略与寿命策略

这页你可以把它讲成：

> **Deadline 检测更新间隔，Lifespan 限制样本有效期，两者不会自动替我们完成故障恢复。**

### 讲解与例子

Deadline 事件表示未满足约定，不是强制让 Timer 按时运行；有限 deadline 的端点还要兼容。Lifespan 控制样本在中间件中的有效性，不会中断已开始的业务回调，也不能替代控制器里的时间戳和安全超时检查。本课通过有事件回调的完整脚本观察，普通 topic echo 不负责打印 deadline 事件。

### 板书

```text
Deadline：数据更新是否超时
Lifespan：单个样本是否过期
业务：收到事件后怎样处理
```

### 命令行实验

实验一 Deadline：A 与 B 都声明 0.1s，但 A 每 0.3s 才发：
```bash
# A
python3 第五课/课堂实验/qos_probe.py pub --period 0.3 --deadline 0.1
# B
python3 第五课/课堂实验/qos_probe.py sub --deadline 0.1
```
预期收到数据并出现 DEADLINE 日志，频率不会被自动加快。事件支持依赖 RMW；若 UnsupportedEventType，核对本机实现，本实验不能由无事件 echo 代替。

实验二 Lifespan：先停止 A/B。A 发布单样本、缓存有效 1 秒：
```bash
# A
python3 第五课/课堂实验/qos_probe.py pub --durability transient_local --count 1 --lifespan 1
# B：明确在 SEND 后超过 3 秒再启动
timeout --signal=INT 5s python3 第五课/课堂实验/qos_probe.py sub --durability transient_local
```
预期 B 不收到过期样本，5 秒退出，timeout 返回 124 属于预期等待结束。恢复基线时重启 A 把 lifespan 改 10，并在 SEND 后 1 秒左右启动 B，预期收到。保持发布进程存活，不把节点退出误当过期。

### 课堂提问

> Lifespan 过期就保证所有已收到指令停止执行吗？

参考答案：不保证，需要业务层超时与停止策略。

这页最后可以总结：

> **通信时效策略与业务安全机制分别设计。**

---

## 第 22 页｜其他 QoS 策略简介

这页你可以把它讲成：

> **存活检查判断端点声明是否还有效，不等于设备采集和业务处理全部正常。**

### 讲解与例子

AUTOMATIC 与 MANUAL_BY_TOPIC 的断言方式不同，有限 lease 也需要兼容。课件提到的 Destination Type 和 BEST_EFFORT_SINGLE_READER 不是 Humble rclpy QoSProfile 的标准公共策略，不能写成任意 ROS 话题自动分派给一个订阅者。工作队列应另设计分发器或任务协议。

### 板书

```text
Liveliness：端点存活声明
业务健康：数据更新 + 状态 + 内容
任务分发：应用协议
```

### 命令行实验

查本机公开字段：
```bash
python3 -c "from rclpy.qos import QoSProfile; print(QoSProfile.__slots__)"
ros2 topic pub --help
```
预期 QoSProfile 提供 reliability/durability/liveliness 等字段，不提供课件说的通用单读者负载均衡开关。可开两个 sub（用 --ros-args -r __node:=qos_sub_two 重命名第二个）观察两者都可收到一条消息；别把任意厂商文档当此 API 的 CLI 参数。

### 课堂提问

> 一个话题有两个订阅者，会自动负载均衡只给其中一个吗？

参考答案：本课标准话题模型不提供这样的默认语义，两者可各自接收。

### 教师说明

用下方 introspection 检查本机公开字段；参考 [Humble qos.py](https://github.com/ros2/rclpy/blob/humble/rclpy/rclpy/qos.py)。

这页最后可以总结：

> **不要把未暴露的 DDS 厂商特性当通用 ROS API。**

---

## 第 23 页｜QoS 不匹配的后果与排查

这页你可以把它讲成：

> **先查名称和类型，再对照端点配置；一个策略的修复只说明那项问题解决了。**

### 讲解与例子

课件三个例子要分开：可靠性不兼容可能拒绝建立数据关系；transient 发布配 volatile 订阅并不必然拒绝匹配，只是晚加入缓存行为不同；depth 不同一般不是匹配失败条件。日志可能提示不兼容，不把所有故障都称为无声黑洞。

### 板书

```text
list/type → info --verbose → 指定兼容 echo → 看编号
```

### 命令行实验

停止其他 /qos/probe 生产者，A reliable depth=1，B reliable depth=5：
```bash
# A
python3 第五课/课堂实验/qos_probe.py pub --depth 1
# B
python3 第五课/课堂实验/qos_probe.py sub --depth 5
# C
ros2 topic info /qos/probe --verbose
ros2 topic echo /qos/probe --qos-reliability reliable --once
```
预期深度不同仍接收。再用第 19 页可靠性不兼容对照。验收：能区分“缓存不同”与“拒绝匹配”。

### 课堂提问

> depth=1 发布、depth=5 订阅一定不兼容吗？

参考答案：不是，深度主要影响缓存行为。

这页最后可以总结：

> **区分拒绝匹配、历史缺失与处理丢样本。**

---

## 第 24 页｜传感器数据场景 QoS 选型

这页你可以把它讲成：

> **先确认算法容忍什么数据缺失，再选尽力与队列，不把传感器三个字当万能答案。**

### 讲解与例子

课件雷达、相机、IMU 表中的采样率和 deadline 是示例。deadine 应留调度与网络余量，按实际频率设；某些算法要求连续采样，不可随意丢帧。本课用字符串模拟高频流，只验证匹配与处理积压，不证明某硬件选型最优。

### 板书

```text
采样频率 + 数据价值 + 处理能力 + 抖动余量 → QoS
```

### 命令行实验

模拟高频传感器，A best_effort period=0.02；B best_effort --once echo：
```bash
# A
python3 第五课/课堂实验/qos_probe.py pub --reliability best_effort --period 0.02
# B
ros2 topic echo /qos/probe --qos-reliability best_effort --once
ros2 topic hz /qos/probe
```
如果 hz 工具的订阅配置影响可见性，以 --help 核对参数和 verbose 对照，不凭无统计断定发布停止。测完退出 B/A。

### 课堂提问

> IMU 总是可以丢任意数量的样本吗？

参考答案：不能，算法可能依赖连续积分，需要按系统需求判断。

这页最后可以总结：

> **预设是起点，算法需求决定最终配置。**

---

## 第 25 页｜控制指令与静态数据场景 QoS 选型

这页你可以把它讲成：

> **控制命令通常关注新鲜与可确认，静态配置关注晚加入获取；同是重要数据，策略也可能不同。**

### 讲解与例子

控制流避免旧指令积压，重要任务用应答或动作确认执行。课件 1–5 秒寿命不是所有控制系统的安全阈值；应按系统响应和风险设计。静态配置适合存活发布端缓存最新样本，若需跨重启保存，应配置文件、数据库或重发布。

### 板书

```text
控制：新鲜 + 超时 + 应答
静态：缓存 + 晚加入 + 重启恢复策略
```

### 课堂提问

> cmd_vel 用 RELIABLE 就能保证停机吗？

参考答案：不能，仍需失联停止、超时和系统安全设计。

这页最后可以总结：

> **重要性不等于只选可靠传输就够了。**

---

## 第 26 页｜官方预设 Profile 详解

这页你可以把它讲成：

> **通过 Python 打印官方 Profile，确认当前版本的配置，而不是只看课件表。**

### 讲解与例子

Default 通常深度 10、可靠、volatile；SensorData 深度 5、尽力、volatile；ServicesDefault 可靠、volatile；Parameters 深度 1000、可靠、volatile。参数服务与参数事件有独立命名的 Profile，但 Humble 官方 RMW 预设中二者都是可靠、volatile、深度 1000；不能因为叫事件就推断必然锁存。预设不自动满足所有业务。

### 板书

```text
官方 Profile → 检查值 → 按需覆盖 → 配套端点
```

### 命令行实验

用当前 Humble 库打印预设：
```bash
python3 - <<'PY'
from rclpy.qos import (
    QoSProfile, qos_profile_sensor_data,
    qos_profile_services_default, qos_profile_parameters,
    qos_profile_parameter_events,
)
for name, profile in [
    ('default', QoSProfile(depth=10)), ('sensor', qos_profile_sensor_data),
    ('services', qos_profile_services_default), ('parameters', qos_profile_parameters),
    ('parameter_events', qos_profile_parameter_events),
]:
    print(name, profile.reliability.name, profile.durability.name, profile.depth)
PY
```
观察 parameters 与 parameter_events：Humble 官方预设两者通常都是 RELIABLE、VOLATILE、深度 1000。它们各有用途，但名字不同不表示当前数值必然不同。以实际库和官方源码为准。

### 课堂提问

> 参数服务 Profile 会自动持久化参数值吗？

参考答案：不会，参数保存和重载另由文件或业务实现。

这页最后可以总结：

> **读当前版本配置，不靠 Profile 名字猜行为。**

---

## 第 27 页｜QoS 工程最佳实践与常见踩坑

这页你可以把它讲成：

> **把今天的实验整理成一个排查顺序，让每个调整都有观察证据。**

### 讲解与例子

课件实践归纳为默认基线、数据用途、集中配置、端点对照与时效余量。修改一个变量后重启对应端点，确认配置确实变化；不要认为改 Python 全局对象会改掉所有已创建端点。对深度、死线和寿命分别记录编号、年龄与事件，明确统计窗口和观察端影响。

### 板书

```text
记录基线 → 改一个变量 → 查询配置 → 观察现象 → 恢复
```

### 命令行实验

最终任务：每组提交一个对照。选项：第 9 页默认组/分组、第 18 页队列、第 19 页可靠性、第 20 页晚加入、第 21 页 deadline。验收包含启动命令、verbose 配置、编号/年龄/事件记录、解释与清理；不只交“运行成功”截图。命令可用 tee 保存，持续进程逐个 Ctrl+C，避免残留生产者影响下一课。

### 课堂提问

> 为什么不能同时改线程、QoS、发布频率再宣称找到根因？

参考答案：多个条件变化使因果无法区分。

这页最后可以总结：

> **能复现和解释的调整才是有效调优。**

---

## 教师核对资料

使用 rosdep 前，先初始化并更新规则；已初始化时跳过 init。


- [Humble 执行器概念](https://docs.ros.org/en/humble/Concepts/Intermediate/About-Executors.html)：等待与调度限制。
- [Humble 回调组](https://docs.ros.org/en/humble/How-To-Guides/Using-callback-groups.html)：默认组、跨组并发与死锁。
- [Humble rclpy executors 源码](https://github.com/ros2/rclpy/blob/humble/rclpy/rclpy/executors.py)：线程与回调许可。
- [Humble QoS 概念](https://docs.ros.org/en/humble/Concepts/Intermediate/About-Quality-of-Service-Settings.html)：兼容、持久性与事件。
- [Humble qos_event.py](https://github.com/ros2/rclpy/blob/humble/rclpy/rclpy/qos_event.py)：本版本事件回调导入与参数。
- [RMW Profile 源码](https://github.com/ros2/rmw/blob/humble/rmw/include/rmw/qos_profiles.h)：参数服务和参数事件区别。
- 随附 [执行器完整脚本](课堂实验/executor_demo.py)、[QoS 完整脚本](课堂实验/qos_probe.py)。

验证记录：已读取对应 PPT 的全部页面文字并核对页序；完整 Python 源码和 launch 做语法检查，XML 模型和包描述做解析检查，模型参数与树结构做静态检查。关键命令与 API 对照 Humble 官方资料及源码。当前 Windows 环境不能访问 WSL 运行会话，因此未完成 ROS 节点运行、C++ ROS 构建、TF/QoS 网络实测及 Gazebo/RViz 显示预演。文中输出均为预期现象，需教师在目标环境预演；不把静态通过写成实际实验通过。
