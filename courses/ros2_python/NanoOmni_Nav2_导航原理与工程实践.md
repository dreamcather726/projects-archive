# NanoOmni ROS 2 Nav2 导航：从原理到工程实践

> 面向已经完成 gmapping 学习的读者，以 NanoOmni 麦轮机器人为主线，串联定位、TF、代价地图、全局规划、MPPI、行为树、恢复行为、生命周期与工程调试。

> **版本说明：** 源码位置与行为以文中所用 Navigation2 1.1.16 和项目配置为准。不同 ROS 2 / Nav2 版本可能存在插件名称、参数和默认值差异，实际部署请以当前工作区配置为最终依据。原稿从飞书复制时缺失的表格已依据全文上下文重建。

## 阅读路线

- 第 0～2 章：建立全局地图与坐标系基础。
- 第 3～6 章：理解代价地图、NavFn 与 MPPI，这是导航算法主线。
- 第 7～12 章：理解行为树、恢复、定位、地图服务和完整数据流。
- 第 13～15 章：进入参数调优与现场排障。
- 附录：用于源码对照、术语速查和自测。

## 符号约定

| 符号 | 含义 | 常用单位 |
| --- | --- | --- |
| $x, y, \theta$ | 机器人平面位置与偏航角 | m、rad |
| $v_x, v_y, \omega_z$ | 前后速度、横移速度与偏航角速度 | m/s、rad/s |
| $\Delta t$ | 离散时间步长 | s |
| $d$ | 某点到最近障碍的距离 | m |
| $r_{ins}, r_{inf}$ | 内切圆半径与膨胀半径 | m |
| $\kappa$ | 膨胀层代价衰减系数 | 1/m |
| $C_i, w_i$ | 第 $i$ 条候选轨迹的代价与权重 | — |
| $\lambda$ | MPPI softmax 温度 | — |

---

## 第 0 章　先修知识与全局地图

### 0.1 你需要已经会的东西（都来自 gmapping 教程）

| 你已掌握的 | 在导航里的角色 |
| --- | --- |
| 贝叶斯滤波 / 粒子滤波（gmapping 教程第 2、3 章） | AMCL 定位就靠它；MPPI 的"采样+加权"也是同款思想 |
| 栅格地图 OccupancyGrid（第 7 章） | 导航的输入：/map 变成代价地图 |
| TF 树与坐标系（第 0 章） | 导航的"地基"：路径、代价地图、位姿全在坐标系里表达 |
| 里程计漂移 vs SLAM 校正（第 12 章） | 理解 map→odom 为什么独立存在（第 2 章会深化） |
| /cmd_vel 速度指令 | 导航的"最终出口"：MPPI 算出来的就是它 |

💡 一句话衔接：gmapping 教程的终点是“我画出了一张地图”；本文的起点是“拿着地图（或边走边画），让机器人从 A 到 B”。

### 0.2 全教程只需要你带 6 个问题

1. 导航和 SLAM 各管什么？为什么是两个独立问题？（第 1 章）
2. 机器人走的路是怎么算出来的？为什么"最短的路"往往不是"能走的路"？（第 3、4 章）
3. 全局路径只是一串点，机器人怎么把它变成每一瞬间的轮子速度？（第 5、6 章）
4. 导航过程中遇到意外（撞到新障碍、被卡住）谁来处理？（第 7、8 章）
5. 没有 SLAM 的时候，机器人靠什么知道自己在哪？（第 9 章）
6. Nav2 这么大一坨，我自己这台的配置到底启用了哪些、调哪个参数改什么？（第 13 章）

学完目标：1-5 能不看资料讲出来；第 6 题能打开 `nav2_params.yaml` 逐段说出每段在干嘛。

### 0.3 一个贯穿全文的机器人小剧场

还是那台 NanoOmni（和 gmapping 教程同一台，这样故事能接上）：

- 麦轮底盘，能前后走（$v_x$）、横移（$v_y$）、旋转（$\omega_z$）；
- 50Hz 里程计、约 9Hz 激光、最大速度实测被配置限制在 0.3 m/s；
- 场地上有墙、桌子、偶尔有人路过（动态障碍）；
- 它现在有两个"身份"要扮演：
  - 无图导航（`slam_nav`）：gmapping 边走边画图，Nav2 拿着最新地图导航；
  - 有图导航（`nav_with_map`）：加载一张旧地图，AMCL 负责"对号入座"。

小剧场的核心冲突：机器人对世界的认识永远是过时且带噪声的——地图可能是 5 秒前的，自己的位置可能偏了几厘米，所以导航是"带着不确定性地往前走，并且随时准备修正"。

### 0.4 全教程的"认知金字塔"（画给你）

```text
        Nav2 导航栈（你现在的目标）
              ▲
   ┌──────────────────────────────┐
   │  行为树 BT / 恢复行为          │  ← 第 7、8 章（"剧本"与"兜底"）
   └──────────────────────────────┘
              ▲
   ┌──────────────────────────────┐
   │ 局部控制 MPPI  + 全局规划 A*   │  ← 第 4、5、6 章（"方向盘"与"地图导航"）
   └──────────────────────────────┘
              ▲
   ┌──────────────────────────────┐
   │  代价地图 Costmap 2D           │  ← 第 3 章（"路感"：哪里能走）
   └──────────────────────────────┘
              ▲
   ┌──────────────────────────────┐
   │  TF 树 / 坐标系 / 机器人模型    │  ← 第 2 章（"地基"）
   └──────────────────────────────┘
              ▲
   定位（gmapping / AMCL）与地图（第 9、10 章）—— 你已经学的部分
```

先给你全场一句话：导航 = 用代价地图把"世界"翻译成"哪里能走、代价多少"→ 用全局规划找一条大方向的路 → 用局部控制把它变成每一刻的速度 → 用行为树把控整个流程和意外。

### 0.5 全文符号约定

见本文开头「符号约定」表。补充两个贯穿全文的词：

- 全局 (global)：相对于整张地图、变化慢的那份东西（全局路径、全局代价地图）。
- 局部 (local)：相对于机器人身边几米、变化快的那份东西（局部代价地图、MPPI 的 2.8 秒预测）。

记住这句：全局负责"往哪走"，局部负责"怎么走"。全文反复回响。

### 0.6 三个学习里程碑

- 里程碑 A：能说清"代价地图上的 254/253/252/0/255 各是什么、膨胀层在干什么"。（第 3 章）
- 里程碑 B：能手算一次 MPPI 更新：给 3 条采样轨迹打分、softmax 加权、得到新控制量。（第 6.9 节）
- 里程碑 C：能对着 `nav2_params.yaml` 和 RViz，指出"这次导航为什么画龙 / 为什么不敢走"，并说出要调哪个参数。（第 13、15 章）

### 0.7 本文讨论范围

只讲你这台机器人实际启用的 Nav2 组件：bt_navigator、planner_server(NavFn)、controller_server(MPPI)、costmap_2d、behavior_server、velocity_smoother、amcl、map_server、lifecycle_manager。不展开：Smac 规划器、TEB/RPP 控制器、collision_monitor、smoother_server、waypoint_follower（你配置里启了但基本没用）等——需要时按本文的阅读方法自己延伸。

---

## 第 1 章　什么是"导航"？Nav2 在解决什么问题

### 1.1 导航的四个子问题

**理论层：** 移动机器人"从 A 到 B"要同时回答四个问题：

| 子问题 | 问题描述 | 谁来解决（本项目） |
| --- | --- | --- |
| 定位 Localization | 我在哪？ | gmapping（顺带）或 AMCL |
| 建图 Mapping | 周围什么样？ | gmapping 或离线地图 |
| 规划 Planning | 往哪走？ | Nav2 planner_server（NavFn） |
| 控制 Control | 怎么走？ | Nav2 controller_server（MPPI） |

你已经完整学过前两个（gmapping 教程）。本教程的主角是后两个，外加一个"总导演"（行为树）和一个"兜底团队"（恢复行为）。

### 1.2 为什么"最短的路"不等于"能走的路"

直觉：从 A 到 B 的直线最短，但中间可能有：

- 静态障碍（墙、桌子）——已知的；
- 动态障碍（行人、临时纸箱）——刚刚才出现的；
- 机器人自己的身体——麦轮车体宽 0.18m，贴墙太近会剐蹭；
- 运动学约束——速度不可能瞬间改变（加速度有限）。

所以导航不是"画一条线"，而是在两个层次上同时答题：
全局层（慢，1Hz 左右）：在整张地图上找一条避开支路的路径    ——负责"大方向对"
局部层（快，20Hz）：结合身边几米的实时障碍和自身运动能力，
                    每 50ms 修正一次走法                 ——负责"眼下不撞"
这就是"全局规划 + 局部控制"两层的由来。本项目中：全局 = NavFn（第 4 章），局部 = MPPI（第 5、6 章）。

### 1.3 两种工作模式：同一套 Nav2，两套定位方案

你这台机器人配了两条完整链路，这是理解本项目配置的第一把钥匙：

| 对比项 | 无图导航 `slam_nav` | 有图导航 `nav_with_map` |
| --- | --- | --- |
| `/map` 的提供者 | gmapping（实时构建） | map_server（加载旧地图） |
| `map→odom` TF 的提供者 | gmapping | AMCL |
| 地图是否变化 | 会：随探索扩展，也可能因校正发生变化 | 不会：使用静态地图 |
| 定位表现 | 随建图质量波动 | 通常更稳定，但依赖地图质量 |
| 适用场景 | 探索新场地、临时任务 | 固定路线反复运行 |
| 一句话 | 边走、边画、边导航 | 拿着地图找路 |

两种模式互斥：都发 `/map` 和 `map→odom`，同时开会打架（配置文件的注释里也写了这一点）。

### 1.4 Nav2 的架构哲学（四件套）

**Nav2 层：** 理解这四件套，整个 Nav2 就没有黑箱了：

1. Server（服务节点）：planner_server、controller_server、behavior_server…… 每个 server 是独立的生命周期节点（第 11 章），负责一类计算。
2. Plugin（插件）：每个 server 里的算法都是可替换插件——planner_server 里可以插 NavFn 或 Smac，controller_server 里可以插 MPPI、DWB、TEB。换算法不改代码，只改配置。你的配置：

controller_plugins: ["FollowPath"]        # 一个控制器，叫 FollowPath
FollowPath:
  plugin: "nav2_mppi_controller::MPPIController"   # 它用的是 MPPI

3. Action（动作接口）：外界（RViz、你的程序）通过 ROS2 action 下任务，如 `NavigateToPose`（去某个点）。action = 目标 + 周期反馈 + 最终结果，天然适合"走一路报一路"的任务。
4. BT（行为树）：bt_navigator 用一棵"行为树"编排流程——先规划、再跟随、失败重试、不行就转圈……（第 7 章）。流程本身也是可替换的"剧本"。

```text
        RViz "2D Goal Pose" / 你的程序
                    │  action: NavigateToPose
                    ▼
        ┌─── bt_navigator（导演：按行为树调度）───┐
        │  ComputePathToPose        FollowPath     │
        └───────┬──────────────────────┬───────────┘
                ▼                      ▼
        planner_server            controller_server
        （NavFn 插件）             （MPPI 插件）      ← 都是 lifecycle 节点
                │                      │
                ▼                      ▼
           全局代价地图 /plan      局部代价地图 /cmd_vel_nav
                                       │
                                velocity_smoother
                                       │ /cmd_vel
                                       ▼
                              base_control → 串口 → 底盘 MCU
```

### 1.5 本项目实际启用了什么（先给全景表，后面逐章展开）

| 组件 | 本项目采用的实现 | 核心职责 |
| --- | --- | --- |
| bt_navigator | 行为树导航器 | 接收导航 action，并按行为树组织规划、控制和恢复 |
| planner_server | NavFn（A*） | 在全局代价地图上生成全局路径 |
| controller_server | MPPI | 把全局路径转成实时速度指令 |
| costmap_2d | 全局/局部代价地图 | 融合静态地图与实时障碍，表达通行代价 |
| behavior_server | Spin、BackUp、Wait 等 | 执行恢复动作 |
| velocity_smoother | 速度平滑器 | 做速度、加速度约束和失联停车 |
| AMCL / map_server | 有图模式定位与供图 | 加载地图并提供 `map→odom` |
| lifecycle_manager | 生命周期管理器 | 统一配置、激活并监测 Nav2 节点 |

### 1.6 本章小结

- 导航 = 定位 + 建图（已学） + 规划（往哪走） + 控制（怎么走）。
- 全局/局部两层分工：全局 1Hz 找大方向，局部 20Hz 管眼下。
- Nav2 四件套：Server + Plugin + Action + BT——换算法查配置，换流程改行为树。
- 本项目两条链路：slam_nav（gmapping 供图）与 nav_with_map（AMCL 定位），互斥不共存。

---

## 第 2 章　坐标系地基：TF 树与机器人模型

本章是纯"地基"章，看起来不如后面热闹，但 90% 的"导航不工作"问题都出在这层。

### 2.1 你机器人当前的 TF 树（实测）

```text
map ──(gmapping 或 amcl)──► odom ──(base_control, 50Hz)──► base_footprint ──(URDF)──► base_link
                                                                                        ├──► base_laser_link（激光）
                                                                                        ├──► imu
                                                                                        └──► 车轮/相机...（URDF 静态）
```

对照 gmapping 教程：那张 TF 树是"建图视角"的，这张是"导航视角"的——只多了一条 **`map→odom` 的语义解释**（下面 2.2）。

### 2.2 为什么要有`map→odom` 这一层？（导航世界最重要的 5 厘米）

直觉类比：

- `odom` 系像"计步器的世界"：从开机起走了多少步，它记得清清楚楚，但它从不知道自己踩过的地方是哪儿——走久了会累计漂移（麦轮横移时漂得更多）。
- `map` 系像"真实世界的坐标系"：地图和真实场地对齐，不动。但机器人没法直接知道自己在 map 里的坐标——它只有计步器。
- 于是两者之间需要一个"校正插头"：`map→odom`。

```text
真实位置（map 系，机器人永远测不准）
      │
      │  map→odom：由 gmapping/AMCL 估出来（低频、可能"跳"）
      ▼
推算位置（odom 系，连续、平滑、但会漂）
      │
      │  odom→base_footprint：由 base_control 里程计发出（50Hz、连续）
      ▼
机器人自身（base_footprint）
```

理论层的关键词：里程计漂移是缓变、连续的；SLAM/定位的校正量是突变、跳变的。把它们分开放在两条 TF 边上，下游节点（代价地图、控制器）就能各取所需：

- 代价地图的局部层用 `odom` 系——因为障碍物相对车身的运动必须连续平滑，不能被定位跳变"抖到"；
- 代价地图的全局层和路径用 `map` 系——因为要跟地图对齐。

这解释了一个高频疑问："为什么 gmapping 定位跳一下，局部代价地图里看不出抖？"——因为局部层压根不听 map 系的，它只听 odom 系。

### 2.3 一条 TF 边的"新鲜度"：时间戳与外推

TF2 的查询是带时间的：`tf2_echo map base_footprint` 背后的查找是"查询时刻 $t$ 的变换"。TF 缓冲有保留时限，太老的数据会被丢掉；两条边时间戳对不齐时，tf2 会做外推或报 `ExtrapolationException`。
课程回顾：gmapping 教程里我们修过 `use_sim_time` 导致时间戳为 0 的 bug——那个 bug 的"症状"就是这类坐标查询失败。记住结论：TF 一切问题，先看时间戳。

### 2.4 机器人模型：半径、内切圆与"行走的安全壳"

**本仓库层：** `nav2_params.yaml` 里与机器人形状相关的三个概念（第 3 章讲膨胀时会再见面）：

| 参数 | 你的值 | 含义 |
| --- | --- | --- |
| robot_radius | 0.18 m | 把车近似成圆的半径（本项目车体约 0.18×0.22m，取保守圆）；用于代价地图"这个格子车能不能进" |
| footprint（未启用） | — | 更精确的多边形轮廓；CostCritic.consider_footprint: true 在 MPPI 里按轮廓判碰撞 |
| base_link 高度 | URDF 里 base_footprint 之上 0.0325m | 车轮轴心离地高，导航层面基本用不到 |

为什么半径取保守值 0.18？——代价地图判"能不能过"时用圆，宁松勿紧的工程习惯：圆大了过不了窄门（保守但不撞），圆小了能钻但可能剐蹭（激进但危险）。你的配置选择保守。
**易错点：** 改大 `robot_radius` 或 `inflation_radius` 之后窄道过不去，不是你调错了——是"物理上真的过不去（按当前配置的定义）"。窄道场景要把它们调小，并在真机上验证剐蹭余量。

### 2.5 里程计是怎么算出来的：base_control 的航位推算全解

导航全程用的"当前速度"和 `odom→base_footprint` 这条 TF，都是你底盘节点自己算的。这个算法虽小，但它是导航的第一块砖——砖歪了，后面全歪。
先把分工说清（这条链上谁负责什么——对应截图里"底盘侧"的完整描述）：

| 角色 | 负责 |
| --- | --- |
| 底盘 MCU（下位机） | 电机闭环（PID 等）、IMU 姿态融合、按串口协议执行速度设定值、按协议上报速度/IMU/电池 |
| base_control（上位机） | 协议打包/解析（帧/CRC/环形缓冲）、航位推算积分、发布 odom/imu/battery + TF |
| Nav2（planner/controller） | 只消费位姿与速度做规划/控制，从不直接碰电机——最终速度设定值由 base_control 打包发往 MCU |

算法名：航位推算（Dead Reckoning）——速度 × 时间积分。和 gmapping 教程里的"腿猜"是同一个思想，区别只在于这里是"上位机拿串口数据积分"，噪声来自轮子而非采样游戏。
输入：下位机报的三样东西（50Hz）

| 数据 | 原始形式 | 换算 |
| --- | --- | --- |
| 前后速度 Vx | int16，单位 mm/s | float(ctypes.c_int16(Vx).value / 1000.0) → m/s |
| 横移速度 Vy | int16，单位 mm/s | 同上（麦轮独有） |
| 偏航角 Yaw | int16，单位 0.01° | /100 → 度，×π/180 → 弧度 |

两个工程细节（都在你的代码里）：`ctypes.c_int16(...)` 是把两字节拼出来的无符号整数还原成有符号（负速度是补码）；偏航角是下位机融合好的——陀螺仪+磁力计的姿态解算在底盘 MCU 里完成，上位机只是翻译，不自己积分陀螺（这是本车里程计比"裸编码器积分"准的重要原因）。
**核心：** 两条积分公式（对应代码"第四步"注释块）
机器人上报的速度在车身系，而 odom 位姿是世界系，所以要先把速度矢量旋转 $\theta$（当前偏航角）：
$$\begin{cases}
x_{t+1} = x_t + \big(V_x\cos\theta_t - V_y\sin\theta_t\big)\,\Delta t \\
y_{t+1} = y_t + \big(V_x\sin\theta_t + V_y\cos\theta_t\big)\,\Delta t \\
\theta_{t+1} = \text{下位机上报的偏航角（直接取用，不积分）}
\end{cases}$$
**人话版：** 每 20ms 问一次"现在什么速度"，把它按"车头朝哪"转成世界的方向，乘上这 20ms 得到位移，累加。这正好是 MPPI 前向仿真用的同一个公式（第 6.4 节）——一边是"真实执行后的记账"，一边是"预演可能性的推演"，公式同源。
数字例子：车头朝 30°（$\theta=\pi/6$），$V_x=0.5$m/s，$\Delta t=40$ms（两拍）：
$\Delta x = 0.5\times\cos30°\times0.04 = 0.0173$m，$\Delta y = 0.5\times\sin30°\times0.04 = 0.0100$m，
总位移 $\sqrt{0.0173^2+0.0100^2}=0.020$m = $0.5\times0.04$ ✓（旋转不改变速度大小）。
三个"防翻车"保险（都值得学）

1. dt 上限保护（`MAX_ODOM_DT=0.2`s）：系统卡顿或串口阻塞会让某一拍延迟到 0.3s+，用错误的大 dt 积分会让位姿"一步跳出去"——代码选择跳过这一拍的积分只更新时间基准（日志里会有一条 warn）。宁可少积一步，不可跳一步。
2. 首拍只建时间基准：构造完成到 spin 开始之间隔着串口握手（~0.2s），首拍 dt 必然超限——代码把 `previous_time` 初始化为 None，首拍不积分也不告警（这是我们修过的一个"每次启动必刷一条误报警告"的问题）。
3. TF 与 odom 用同一时间戳：`odom` 消息头和 `odom→base_footprint` 的 TF 都用同一个 `self.current_time`——下游（gmapping/Nav2）拿这两个数据做插值时，时间戳错位会算出错误位姿。

它为什么会漂？（物理来源清单）

| 误差源 | 表现 | 对导航的影响 |
| --- | --- | --- |
| 轮径/减速比假设（底盘报 64mm/20:1） | 系统性比例误差（走 10m 偏 1%→10cm） | 全局层由 SLAM/AMCL 校正 |
| 麦轮打滑（尤其横移、原地快转） | 突发性误差，累积最快 | 这就是 vy_max: 0.05 和 PreferForward 权重 15 的由来（第 6.5 节） |
| 地面不平/负载变化 | 随机漂移 | 被 map→odom 的定位校正吸收 |
| 串口 20ms 的"一问一答"延迟 | 速度对应的是"上上拍" | 50Hz 下可接受；急加减速时有轻微滞后 |

gmapping 教程第 9 章的四个噪声参数（srr/srt/str/stt）就是给这些物理误差建的概率模型——现在你能把"模型参数"和"物理来源"一一对上了。
对照代码
base_control_ros2.py：odom_timerCB()（主积分）、cmd_velCB()（反向：把速度指令打包下发）、_parse_frame() 中 CMD_RX_VEL 分支（原始值解析）。导航侧消费点：controller_server 读 /odom 的 twist 字段当速度（第 6.11 节）、TF 被所有节点查询（第 2.2 节）。

### 2.6 静态 TF 从哪来：robot_state_publisher 与 URDF

**本仓库层：** 除了 `odom→base_footprint`（动态、底盘节点发）和 `map→odom`（动态、gmapping/AMCL 发），TF 树上其余所有边（`base_footprint→base_link→激光/IMU/车轮/相机`）都是静态的，来源有两种（二选一，由 `use_urdf` 决定）：

| 对比项 | `use_urdf:=true`（推荐/默认） | `use_urdf:=false` |
| --- | --- | --- |
| TF 提供者 | robot_state_publisher（读取 URDF） | base_control / lidar 各自的 static_transform_publisher |
| 帧的完整度 | 完整，包含车轮、相机等 | 仅包含导航所需子集 |
| RViz 整车模型 | 可用 RobotModel 显示 | 只能查看 TF 坐标轴 |

URDF → TF 的机制：`nanoomni_description.urdf` 里每个 `<link>` 是一个坐标系，每个 `<joint>` 是父子关系 + 一个 `origin (xyz, rpy)`；`robot_state_publisher` 读 `robot_description` 参数后，把所有 fixed joint 的 origin 原样变成静态 TF（发到 `/tf_static`，Transient Local——晚启动的 RViz 也能立刻拿到）。URDF 里所有关节都是 fixed，所以不需要 joint_state_publisher。
你的 URDF 关键数值（与上节坐标系对账用）：
base_footprint → base_link        z=0.0325（轮轴中心离地高）
base_link      → imu              (0, 0, 0)          # 与下位机共位
base_link      → base_laser_link  (0.0537, -0.0018, 0.129), rpy=(0,0,π)   # ★ yaw=π
base_link      → 四只轮子          (±0.0746, ±0.089266, 0), rpy=(-π/2,0,0)
base_link      → 相机             (0.099, -0.0001, 0.1077)
两套模式的数值已经对齐过（静态版激光 TF 的 z 改成了 URDF 的 0.129、yaw π 同款）——但两套不要同时启动（同名父子帧双发布者会让 tf2 混乱，也会抢串口，见第 12 章注意事项）。
为什么激光帧要 yaw=π？ S4.yaml 里雷达参数 `inverted: true` 已经让扫描方向翻转一次，TF 再翻一次，两次抵消（配置注释里写明了这个组合关系）——单独改其中任何一个都会导致扫描方向反过来。这是"参数与 TF 必须成对理解"的最佳例子。

### 2.7 本章小结

- TF 树三层：map（真实世界）→ odom（计步器世界）→ base_link（车身），两段交接分别由"定位"和"里程计"负责。
- map→odom 的独立性是为了隔离"漂移（连续）"和"校正（跳变）"。
- 局部用 odom、全局用 map 是 Nav2 的默认分工。
- 机器人形状用 robot_radius 保守化；它和膨胀半径共同决定"能走多窄"。

---

## 第 3 章　代价地图 Costmap 2D：导航的"路感"

本章是全文第一个硬核章。学完它，"哪里能走、代价多少"这个问题就永远属于你了。

### 3.1 从"有墙/没墙"到"路感"：为什么需要代价地图

gmapping 给你的 `/map` 是一张三值图：0（空闲）/ 100（障碍）/ -1（未知）。规划器如果只用它，路只有两种状态——"能走"或"不能走"。问题：

- 贴着墙 1cm 的路"能走"，但真开过去会剐蹭；
- 离墙 2cm、5cm、10cm……"危险程度"其实是连续的；
- 动态障碍物（刚走过去的人）需要临时记在图上，且要能被擦掉。

所以导航不用原图，而用代价地图（Costmap）：每个格子是 0~255 的连续代价，"有墙"只是最高档。

### 3.2 代价的数值语义（必须背下来的 5 个数）

**Nav2 层：** `nav2_costmap_2d/include/nav2_costmap_2d/cost_values.hpp`：

| 数值 | Nav2 常量 | 含义 | 规划语义 |
| ---: | --- | --- | --- |
| 255 | `NO_INFORMATION` | 未知区域 | 是否可通行取决于规划器配置 |
| 254 | `LETHAL_OBSTACLE` | 致命障碍 | 不可通行 |
| 253 | `INSCRIBED_INFLATED_OBSTACLE` | 机器人内切圆会碰撞的区域 | 不可通行 |
| 1~252 | 膨胀代价 | 越靠近障碍代价越高 | 通常可通行，但会被惩罚 |
| 0 | `FREE_SPACE` | 自由空间 | 可通行 |

记忆法：255 最"虚"（不知道）、254 最"硬"（肯定撞）、253 是"车身贴着墙"、往下递减到 0。

### 3.3 分层代价地图：像叠加的透明胶片

**Nav2 层：** costmap_2d 的核心设计：一张代价地图 = 若干图层（layer）叠加而成，像几张透明胶片叠在一起：

```text
master 层（合成结果，最终给规划/控制看）
   ▲ 逐格取"合并规则"（默认取最大值，可配置）
   │
   ├── inflation_layer    ← 把障碍"发泡"，制造缓冲带（第 3.5 节）
   ├── obstacle_layer     ← 实时障碍：雷达打到的地方标记、扫过的地方擦除（第 3.4 节）
   └── static_layer       ← 静态地图：/map 投影过来的障碍（第 3.3 节）
```

**本仓库层：** 你的配置正是这个"三层结构"：

```yaml
# 全局代价地图（跟地图对齐，不随车移动）
plugins: ["static_layer", "obstacle_layer", "inflation_layer"]
update_frequency: 1.0     # 每秒重算 1 次
# 局部代价地图（以车为中心 3×3m 滚动窗口）
plugins: ["obstacle_layer", "inflation_layer"]   # 注意：没有 static_layer！
rolling_window: true
update_frequency: 5.0
```

为什么局部没有 static_layer？ 局部窗口是"车身周围 3×3 米"，它要回答的是"眼前这几米怎么走"——用实时的雷达障碍层就够了；静态地图的墙已经包含在雷达观测里。而且局部窗口挂在 odom 系，跟地图对齐没有意义。

### 3.4 obstacle_layer：把雷达变成"即时路况"

两个核心动作：

1. Marking（标记障碍）：雷达一束打到 3.2m 处有回波 → 那个格子写 LETHAL_OBSTACLE。
2. Clearing（清除空闲）：从机器人位置到回波点之间的射线扫过的格子写 FREE_SPACE——这是最关键的一步：如果一个障碍物被搬走了，它的"障碍格"会在雷达再次扫过该方向时被光线擦掉。这就是"动态障碍物会被自动清理"的机制。

**本仓库层：** 你的参数：
scan:
  topic: /scan
  marking: true
  clearing: true
  obstacle_max_range: 6.0    # 超过 6m 的回波，当障碍物用
  raytrace_max_range: 8.0    # 超过 8m 不再做"清空射线"
为什么两个 range 不一样？——"清空比标记更保守"：远处的回波可能不准（激光打到玻璃反射、远处噪声），但"我确实清过这条路"只在近距离才可信。近距离（<6m）标记；中等距离（6~8m）允许清空但不标记；>8m 全不管。
代码对照：nav2_costmap_2d/plugins/obstacle_layer.cpp 的 raytraceFreespace()（616 行）就是"沿着射线擦格子"。

### 3.5 inflation_layer：给障碍"发泡"，制造安全壳（本章核心）

直觉：在障碍周围画两个圈——

- **内圈（内切圆半径 $r_{ins}$）**：车身转一圈能碰到障碍的半径。圈内 = 253（不可通行）；
- **外圈（膨胀半径 $r_{inf}$）**：从内圈向外，"危险度"按距离指数衰减，一路降到 0。

理论层/公式（来自源码 `inflation_layer.hpp` 的 `computeCost`，已对照 1.1.16 源码逐行核实）：
$$c(d) =
\begin{cases}
254, & d = 0 \text{（障碍格本身）} \\
253, & 0 < d \le r_{ins} \\
252 \cdot e^{-\,\kappa \cdot (d - r_{ins})}, & r_{ins} < d < r_{inf} \\
0, & d \ge r_{inf}
\end{cases}$$
**人话版：** 离障碍越远越安全，代价按指数往下掉；参数 $\kappa$（配置名 `cost_scaling_factor`）越大掉得越快（"胆小的车"只用离墙远的路线；越小越"敢贴墙"）。
**本仓库层：** 你的值：

```yaml
inflation_layer:
  cost_scaling_factor: 3.0     # κ
  inflation_radius: 0.40       # r_inf = 0.4m
# 注意 r_ins 不是直接配置的：Nav2 用 footprint/robot_radius 推出内切圆半径
# 你配了 robot_radius: 0.18 → r_ins ≈ 0.18
```

数字例子（你自己动手验算一遍，里程碑 A）：取 $\kappa=3$，$r_{ins}=0.18$，计算几圈外的代价：

| 到最近障碍栅格的距离 $d$ | 公式/规则 | 代价 | 直观含义 |
| ---: | --- | ---: | --- |
| $d \le 0.18\,\text{m}$ | 内切半径以内 | 253 | 车身会与障碍发生碰撞 |
| 0.20 m | $252e^{-3\times0.02}$ | ≈ 237 | 非常危险 |
| 0.25 m | $252e^{-3\times0.07}$ | ≈ 204 | 危险 |
| 0.30 m | $252e^{-3\times0.12}$ | ≈ 176 | 仍有明显风险 |
| 0.40 m | $252e^{-3\times0.22}$ | ≈ 130 | 膨胀半径边界 |
| $d > 0.40\,\text{m}$ | 膨胀范围以外 | 0 | 不再产生膨胀代价 |

**易错点：** $r_{inf}=0.40$ 意味着"离任何障碍 0.4m 内都要付代价"。你的车道如果只有 0.8m 宽（两边墙各留 0.4m），中央的代价 ≈ 130，规划器会尽量走中间，但不会拒绝走——因为 130 不是"不可通行"。想让规划器彻底远离某区域，那是 253/254 的职责。
再补一句设计动机：为什么要制造"梯度"而不是干脆一刀切？ 因为规划器（第 4 章）是基于"总代价最小"找路的——梯度让它可以权衡"走远一点点但更安全"，也让 MPPI（第 6 章）有一个连续的信号可以优化。一刀切的图会让规划器只知道"能走/不能走"，没有任何"舒适区"概念。

### 3.6 滚动窗口：局部代价地图为什么是"跟着车走的一块布"

局部代价地图 rolling_window: true，尺寸 3×3m，以车身为中心随车移动。它的坐标系是 odom。好处：

- 计算量恒定（永远只有 3600 个格子：$3/0.05 \times 3/0.05 = 60\times60$）；
- 只有"眼前"的信息，实时障碍更新快。

局部窗口里的障碍来源：只有 obstacle_layer——时刻反应"现在周围哪儿有东西"，服务于几十毫秒级的局部决策。

### 3.7 代价地图的更新机制：滚动窗口怎么"滚"、图层怎么"合"

前面讲了各图层"算什么"，这一节讲 master 层"怎么组织"——这些机制决定了很多现象"为什么是这样"。
① 更新循环：`update_frequency` 触发 `updateMap()` → 按 `plugins` 列表顺序，每层把自己的结果写进 master 网格；`publish_frequency` 是另一套节奏，只控制"对外发 /costmap 的频率"。所以你可以"算得勤、发得慢"（局部图 5Hz 算 2Hz 发）。
**② 合并规则 `combination_method`**（每个栅格层自己的参数，默认 `1`）：
值
名字
含义
1（默认）
Maximum
新值与旧值取最大——只能加代价，不能减
0
Overwrite
有效值直接覆盖
人肉版：Maximum 意味着"每个人都在往上叠黑"，谁也不能把别人画的障碍擦淡——所以"清除旧的标记"必须由绘制者自己用更低的值（自由空间=0）去覆盖……等等，Maximum 下 0 覆盖不了 254？——这就是为什么清除逻辑在图层内部完成：obstacle_layer 自己维护"这一层本轮的值"，先清后标，最后整层参与合并。理解了这层，就不会有"为什么 0 擦不掉障碍"的困惑。
③ 滚动窗口怎么"滚"：局部图不是"每帧清空重画"，而是 `Costmap2D::updateOrigin()`（源码 `costmap_2d.cpp:330`）——窗口在 odom 系里跟着机器人平移，缓冲区按整格平移（把旧内容挪到新位置），只有新露出来的边缘区域才置默认值。所以：平移本身几乎零成本；真正花钱的是 obstacle 层在新边缘上的连线清障。
**④ `track_unknown_space`**：开启（你的全局图）→ 原始地图的 -1 在代价地图里是 255（NO_INFORMATION），规划器由 `allow_unknown` 决定怎么对待；关闭 → 未知直接被当空闲（对 SLAM 场景很危险，你没用）。局部图天然全是观测过的区域，无所谓。
**⑤ 车底清障 `footprint_clearing_enabled`**（障碍层参数，默认 `true`，你未改）：把机器人 footprint 覆盖的格子强制清成自由。为什么必须有它？雷达装在车头，斜向的束经常打到自己的车体——没有这项，机器人会把自己标成障碍，然后"原地卡死"（自己的障碍圈住自己）。装了自制遮挡物/加了新支架后导航突然死锁，先查这里。

### 3.8 障碍层细节全表：一个"幽灵障碍"的完整生命周期

**理论层：** obstacle_layer 的完整处理链（每个 `observation_sources` 条目独立执行）：

| 步骤 | 行为 | 相关参数（你的值） |
| --- | --- | --- |
| 1. 取帧 | 订阅 /scan（SensorData QoS） | — |
| 2. 转坐标 | 用 TF 把每束激光转到代价地图坐标系 | — |
| 3. 过滤 | 高度过滤（max_obstacle_height）、距离过滤 | 2.0m |
| 4. 标记 Marking | 每个回波点写 254 | obstacle_min/max_range = 0 / 6.0 |
| 5. 清空 Clearing | 沿"车→回波"直线把格写 0（raytrace） | raytrace_min/max_range = 0 / 8.0 |
| 6. 合并 | 按 combination_method（Maximum）上叠 | 第 3.7 节 |

"幽灵障碍"的完整故事（第 12.3 节提过现象，这里是机制）：

1. 行人站在 (2, 0) → 雷达回波 → 该格被标记 254；
2. 行人走了 → 但没有任何机制"自动知道"障碍没了——直到雷达再次扫过该方向；
3. 下一次射线经过 (2, 0) 并打到更远处（比如 5m 的墙）→ raytrace 把 (2,0) 沿线清成 0 → 幽灵消失；
4. 什么时候幽灵擦不掉？——雷达看不到该方向时（机器人再没朝那边看）、或回波点超过 `raytrace_max_range`（8m）时。这时幽灵会长期驻留，规划器永远绕它走。

兜底操作：`ros2 service call /local_costmap/clear_entirely_local_costmap nav2_msgs/srv/ClearEntireCostmap`（全局图同理）。行为树的恢复分支里第一步就是这个服务（第 7.3 节）——现在你知道它为什么排第一了。
两个你没配置但有意义的参数：observation_persistence（>0 时标记保留 N 次更新再衰减——动态环境可给 1~2 次做迟滞）、inf_is_valid（/scan 里 inf 是否当"很远的自由"——你的雷达 invalid_range_is_inf: false 不产生 inf，此参数与你无关）。

### 3.9 代价地图的三种发布形态（RViz 里该看哪个）

**本仓库层：** nav2_costmap_2d 同时对外发三种"地图"（源码 `costmap_2d_publisher.cpp`）：

| 话题 | 类型 | 内容 | 用途 |
| --- | --- | --- | --- |
| /costmap | nav_msgs/OccupancyGrid | 合成后的最终代价（0~255 灰阶图） | RViz 显示；兼容旧式消费者 |
| /costmap_raw | nav2_msgs/Costmap | 最终代价与分层元数据 | 调试工具或上层分析 |
| /costmap_updates | map_msgs/OccupancyGridUpdate | 自上次发布后的变化块 | 增量更新，节省带宽 |

RViz 实践：

- 想看"导航眼中的世界" → Map 显示接 /local_costmap/costmap 和 /global_costmap/costmap，并把 Color Scheme 设为 costmap（黑=254 障碍，灰阶=膨胀梯度，白=自由，蓝灰=未知）；
- 局部图建议叠在全局图上对比看：全局图里的"幽灵"和局部图里的"实时障碍"分工不同（第 12.3 节）；
- 调试 MPPI 时同时开 `/optimal_trajectory`（第 6 章）——"代价图 + 轨迹"两张图一起看，90% 的行为异常都能肉眼定位。

### 3.10 本章小结

- 代价地图 = 0~255 连续代价；记住 255/254/253/0 四个锚点。
- 分层设计：static（地图投影）+ obstacle（实时雷达 mark/clear）+ inflation（发泡），逐格合成。
- 膨胀公式 $252e^{-\kappa(d-r_{ins})}$：指数衰减，$\kappa$ 越大越"远离墙"。
- 局部滚动窗口挂 odom 系、只管眼前；全局挂 map 系、俯瞰全场。
- 代码入口：nav2_costmap_2d/src/layered_costmap.cpp（合成调度）、plugins/ 下三个插件文件。

---

## 第 4 章　全局规划：从 Dijkstra 到 A* 到 NavFn

### 4.1 问题定义：在"代价棋盘"上找一条最便宜的路

输入：一张带代价的栅格图（第 3 章的产物）+ 起点 + 终点。
输出：一串格子坐标 / 路点，构成一条从起点到终点、总代价尽可能小的路径。
注意两个和"常识"不同之处：

- 代价不只看长度：绕远但安全的路可能比抄近但贴着墙的路"更便宜"；
- 不要求"数学最短"，要求"足够好 + 算得快"（1Hz 要能重算）。

### 4.2 家谱：Dijkstra → A*

**理论层：** 两个经典图搜索算法：
Dijkstra（1956）：从起点开始"水波纹扩散"，每次挑当前离起点代价最小的格子往外扩，直到扩到终点。

- 设 $g(n)$ = 从起点走到格子 $n$ 的真实最小代价；
- 保证全局最优，但盲目扩散——终点在东边，它也往西边扩一大圈。

**A\*（1968）**：给 Dijkstra 装上"方向感"，引入启发函数 $h(n)$：
$$f(n) = g(n) + h(n)$$

- $g(n)$：已走代价（Dijkstra 的老本行）；
- $h(n)$：从 $n$ 到终点的估计代价（"还有多远"的直觉）；
- 每次优先扩展 $f$ 最小的格子。

**人话版：** Dijkstra 是"地毯式搜索"，A* 是"地毯式搜索 + 往目标方向多用力"。$h(n)$ 只要不高估真实距离（"admissible"），A* 的结果就和 Dijkstra 一样最优，但通常快得多。
数字直觉：8m×8m、5cm 栅格的 2D 规划：

- Dijkstra 平均扩展 ~几万格；
- 加上良好的 $h$ 后，A* 通常只扩展几千格。

### 4.3 NavFn：把"导航函数"当成势场

**Nav2 层：** 本项目用 `nav2_navfn_planner`（配置里的 `plugin: "nav2_navfn_planner/NavfnPlanner"`，`use_astar: true`）。NavFn 来自 90 年代的 Stanford 工作，"Nav Fn" = Navigation Function（导航函数）——本质是在地图上造一个"势场"：

- 目标点是势场的“谷底”；
- 每个格子的势表示从该格到达目标的累计代价；
- 最优路径从起点沿势能下降方向到达目标。

关键实现细节（源码 `nav2_navfn_planner/src/navfn.cpp` 核实）：

1. 代价翻译：代价地图的 0~252 被映射成 NavFn 内部势：

$$\text{cost}_{navfn} = 50 + 0.8 \times \text{cost}_{costmap}$$
   （源码宏：COST_NEUTRAL = 50，COST_FACTOR = 0.8）

- 空闲格（0）→ 势 50；
- 贴着障碍的格子（250）→ 势 250；
  - 254/255（障碍/未知）在 NavFn 里是 `COST_OBS`——默认不可通行（但见 4.5）。

2. 传播：以目标点为势能零点向外扩散，直到覆盖起点；这一步会得到整张势场；
3. **A\* 部分**：`use_astar: true` 时，用"二次近似启发 + 欧氏距离"作为 $h$（源码注释 `use quadratic approximation`，以及 `650` 行附近 `uses Euclidean distance heuristic`）——提前"冲向终点"，扩展的格子数大幅减少；
4. 提取路径：从起点沿势能下降方向前进，直到抵达目标，形成路点序列。

二次近似启发的人话版：欧氏直线距离是最"乐观"的估计；NavFn 在其上乘一个略小于 1 的修正系数（来自经验），让 $h$ 略微低估真实代价，从而既快又不破坏最优性。

### 4.4 本仓库层：你的 NavFn 配置逐行读

```yaml
GridBased:
  plugin: "nav2_navfn_planner/NavfnPlanner"
  tolerance: 0.5            # 起点/终点没落在栅格正中心时，允许 0.5m 内的"就近格"
  use_astar: true           # 用 A* 而不是纯 Dijkstra —— 更快
  allow_unknown: true       # 未知区域允许通行（★见 4.5）
```

`tolerance` 用于目标点不可直接到达时的兜底：规划器可以在目标周围 0.5 m 内寻找可行终点。它不是普通的“格心取整”误差。

### 4.5 `allow_unknown`：SLAM 模式的生命线

无图导航（slam_nav）时地图是边逛边长的——前方很可能是一片"未知"（255）。如果规划器把未知当障碍，那么机器人永远只能在自己走过的区域里打转。
allow_unknown: true 的含义：未知区域按"可通行、但要付代价"处理（源码 `navfn_planner.cpp:241` 把 `allow_unknown_` 直传给 NavFn 的处理逻辑）。
什么时候该关掉它？ 有图导航模式下，地图是完整的，未知区域 = "地图外/没建过的地方"，让它当障碍更安全。本项目两个模式共用同一份配置，`allow_unknown: true` 对两种模式都"不死板"——代价是可能规划出一条"冲进未知区"的路线（这正是探索任务想要的）。

### 4.6 为什么选 NavFn 而不是 Smac？（本仓库层决策）

Nav2 里还有更"高级"的规划器（Smac：支持 Hybrid-A*、State Lattice，能规划出满足车辆运动学、带倒车段的光滑路径）。本项目用最朴素的 NavFn，理由在配置注释里也写了：

| 维度 | NavFn（A*） | Smac（Hybrid-A* 等） |
| --- | --- | --- |
| 速度 | 快 | 慢（尤其大图） |
| 输出质量 | 格子台阶状路径 | 光滑、可倒车 |
| 对"地图在变"的鲁棒性 | 高（每 1s 重算无压力） | 低（重算贵） |
| 适配底盘 | 全向/差速都行 | 参数与运动学耦合深 |

slam_nav 模式下地图每秒都在长，NavFn 的"快 + 简单"比"优 + 贵"更合适；且路径的"台阶感"会由下游 MPPI 磨平（第 6 章：MPPI 只用路径做参考，不精确贴线）。

### 4.7 NavFn 实现细节：从宏常量到路径提取（源码行级）

把第 4.3 节的"原理图"落到 1.1.16 的实际代码上：
① 三个宏，定住整张图的"物理常数"（`navfn.hpp` 52-79 行）：
#define COST_NEUTRAL  50    // 空闲格的基准势
#define COST_FACTOR   0.8   // 代价地图值 → 势的放大系数
#define COST_OBS      254   // NavFn 内部的"禁区"
#define COST_OBS_ROS  253   // ROS 侧 253(=膨胀内圈)也被当障碍!
#define PRIORITYBUFSIZE 10000
注意 `COST_OBS_ROS = 253` 这条：膨胀层的内切圆区域（253）在 NavFn 眼里等同于障碍——这就是第 3.5 节"253 是死线"在规划器侧的直接证据。"车能贴多近墙"其实由膨胀层而不是 NavFn 决定。
② 传播用"优先级桶"而不是二叉堆（`navfn.cpp` 的 `propNavFnAstar` 与 `push_cur` 宏）：代价是离散小整数，于是 NavFn 把待扩展格按势值分进 `curP/nextP` 两个桶数组（`curPe/nextPe` 两个指针顺序扫）——同桶内无需排序，均匀代价网格上比堆更快。这是"用数据分布特性换速度"的经典工程手法。
**③ A\* 的启发函数**：源码注释明确写了 `use quadratic approximation`（452、527 行附近）与 `uses Euclidean distance heuristic`（650 行附近）——即"欧氏距离 + 二次修正"，让 $h$ 略微低估（保守），在快与最优之间取平衡（第 4.2 节的落地版）。
④ 起点格强制清障（`navfn_planner.cpp` 231 行）：注释原文 "clear the starting cell within the costmap because we know it can't be an obstacle"——机器人贴墙/在膨胀圈里时，它脚下的格子可能是 253，NavFn 会强行清掉起点格再规划。这也是为什么"机器人偶尔在'被围住'的图里还能挪出来"。
⑤ 路径提取（`navfn_planner.cpp` 411 行起）：从 `planner_->getPathX()/getPathY()` 拿回格子坐标序列，组装成 `nav_msgs/Path`（map 系）发布到 `/plan`。目标点在障碍/未知里时的处理：在 `tolerance`（0.5m）半径内吸附到最近的可行格（第 4.4 节）。
**⑥ `use_final_approach_orientation`**（1.1.16 参数，你未配置=默认 `false`）：false 时路径终点朝向 = 你下发的目标朝向；true 时用"接近方向"作为终点朝向。倒车入库式目标（目标朝向与接近方向相反）要用 true 才走得优雅——你目前的两个 goal 场景（去点、停）用 false 足够。

### 4.8 本章小结

- 全局规划 = 带代价的图搜索；Dijkstra 保最优但盲目，A* 加启发函数又快又最优。
- NavFn 把代价地图翻译成势场（$50 + 0.8\times cost$），用二次近似启发 + 欧氏距离做 A*。
- allow_unknown: true 是 SLAM 模式的生命线；tolerance 是起终点吸附范围。
- 本项目选 NavFn 的核心动机：地图持续变化时需要"快而稳"；路径台阶交给 MPPI 磨平。

---

## 第 5 章　局部控制入门：模型预测控制（MPC）

全局规划给了你"往哪走"（一串路点）；本章回答"怎么把路点变成每个瞬间的速度"。

### 5.1 为什么全局路径不能直接开

把全局路径直接当速度指令，会立刻遇到三个问题：

1. 路径没有时间：路径只说"经过这些格"，没说"用多快经过"。以 0.3m/s 还是 0.05m/s 过弯，路径都不知道。
2. 路径是"重规划前的旧世界"：1 秒前规划的路，面前可能新站了个人。局部必须能偏离全局路径逃命，之后再回来。
3. 底盘有极限：加速度有限（起步不能瞬间到 0.3m/s）、转弯半径/角速度有上限（$\omega_z \le 1.5$rad/s）、麦轮横移不能超过 0.05m/s。速度指令必须时时合法。

所以"控制"的本质是：在底盘能力约束下，跟踪全局路径，同时躲开眼前的障碍，并把这一切做成一个 50ms 一个的决策流。

### 5.2 MPC：先"预演未来"，再"只迈一步"

模型预测控制（Model Predictive Control），控制理论里最"直觉"的一族方法，三件套：

1. 模型：知道"如果我从现在开始一直发这些速度，未来会走到哪"（一个简化的运动学模型）；
2. 目标函数：给每条"未来"打分——离路径多远、离障碍多近、离目标多远、头朝哪……
3. 滚动优化（receding horizon）：每次只信"预演"的第一小段，执行一小步（本项目 50ms），下一周期带着更新的世界重新预演——所以模型不准也没关系，错了下一拍就纠。

每个控制周期（50ms）重复：
  ① 从当前状态出发，预演一批"未来 2.8 秒的可能走法"
  ② 给每条打分（critics）
  ③ 组合出一个"参考速度"，只取它的第一小步执行
  ④ 世界变了（新障碍/新位姿）？没关系，下一周期①②③重来
**人话版：** MPC 像开车时的"脑内预演"——"如果我这 3 秒继续这么打方向会撞上吧？那轻轻往左一点"。你并不精确规划 3 秒，你只是用预演来指导当下的一小步。

### 5.3 一个一维 MPC 手算例子

机器人沿一条直线走，位置 $x$，速度 $v$，每 0.5s 决策一次，加速度最多 $\pm 1$。目标：停到 $x=10$。当前 $x=0$，$v=6$。
预演两条候选：

- 候选 A：立刻以 $-1$ 减速 → 0.5s 后 $x = 0 + 6\times0.5 - 0.5\times1\times0.25 = 2.875$，$v=5.5$；
- 候选 B：以 $-0.5$ 轻减速 → $x = 3.06$，$v=5.75$；

加上"过冲成本"：A 离"减速目标线"更近 → A 略优。执行 A 的第一步（发 0.5 秒 $-1$），然后世界刷新重来。这就是 MPC 的全部灵魂——"预演很长、执行很短、滚动反复"。

### 5.4 从"求导最优"到"采样最优"：为什么用撒点

经典 MPC 用数学优化（梯度下降等）解"哪条控制序列最优"。但导航系统里：

- 目标函数里全是"代价地图查表、footprint 碰撞检测"这类不可导的东西；
- 约束（速度上限、加速度上限）形状复杂；
- 必须每 50ms 出结果，不能慢慢收敛。

采样式 MPC 换了个思路：不求解，直接撒一大把候选，谁分低就偏向谁。这正是第 6 章 MPPI 的做法——它也是你学过的粒子滤波的"亲兄弟"：粒子滤波是"撒粒子估状态"，MPPI 是"撒轨迹选控制"，两者共享"采样 + 打分 + 加权"的骨架。

### 5.5 本章小结

- 控制 = 约束下的路径跟踪 + 实时避障 + 每 50ms 的连续决策。
- MPC 三件套：模型、目标函数、滚动优化；精髓是"预演长、执行短、反复纠"。
- 导航场景让经典优化求解不可行（不可导、要快），于是采用采样式 MPC——引出第 6 章。

---

## 第 6 章　MPPI：本项目的心脏

本章是全文分量最重的一章。读完后你将对"机器人每一刻的速度是怎么冒出来的"有完整的、可动手验算的理解。

### 6.1 MPPI 是什么：名字里的信息论

MPPI = Model Predictive Path Integral control（模型预测路径积分控制，Williams et al. 2017）。"路径积分"来自它的数学出身：用随机采样 + 指数加权去近似一个"最优控制的概率分布"（自由能/KL 散度视角）。你不需要那个推导，只需要记住它的工程形象：
MPPI = "撒 2000 条未来轨迹 → 打分 → 按'好多少'指数加权平均 → 得到一个速度 → 只执行 50ms → 重来"。
它是采样式 MPC 家族里由于"软加权"而特别平滑的一种：不取最优那一条（那会抖），而是"偏向好的、兼顾次好的"。

### 6.2 一条"未来"怎么表达：控制序列

**本仓库层：** 你的配置决定了一条"预演"的形状：

```yaml
time_steps: 56        # 未来分成 56 个时间格
model_dt: 0.05        # 每格 0.05 秒 → 总时长 56×0.05 = 2.8 秒
motion_model: "Omni"  # 每个时间格控制量 = (vx, vy, wz) 三维（麦轮）
batch_size: 2000      # 每次撒 2000 条
```

所以一个"控制序列"是一张 $56 \times 3$ 的表：

```text
        t=1     t=2     t=3   ...  t=56
vx     0.30    0.30    0.28   ...  0.0    ← 每格一个前后速度
vy     0.00    0.01    0.00   ...  0.0    ← 横移（本车限幅 0.05）
wz     0.10    0.12    0.15   ...  0.0    ← 旋转
```

只有第一行（更准确地说：序列滚动后的第一格）会变成发出去的速度指令——其余 55 格都是"陪跑的未来"。

### 6.3 采样：把 2000 条"未来"撒出来

**Nav2 层：** 每条候选 = 当前序列 + 随机噪声：
$$u_i = u_{base} + \varepsilon_i,\quad \varepsilon_i \sim \mathcal{N}(0, \sigma^2)$$
每个维度的噪声强度由配置控制（本仓库层）：

| 参数 | 你的值 | 含义 |
| --- | --- | --- |
| vx_std | 0.20 | 前后速度的采样噪声（"敢探索多大的速度变化"） |
| vy_std | 0.05 | 横移噪声（调小 → 少撒横移，呼应你"少用横移"的整体策略） |
| wz_std | 0.40 | 旋转噪声（转向的主要探索通道） |

**人话版：** MPPI 是"在当前想法的周围撒一圈变体"：

- 噪声调大 → 探索更野（可能发现更好的走法，但也更抖、算得更费）；
- 噪声调小 → 保守（贴着上一步走，遇突发事件反应慢）。
- 你的 `regenerate_noises: false`：同一批噪声在一段时间内复用（省 CPU、也让行为更平滑，代价是探索的随机性降低）。

代码对照：nav2_mppi_controller/src/noise_generator.cpp（撒噪声）、src/optimizer.cpp 的 sampleControlSequence/bounded 处理。

### 6.4 前向仿真：把每条轨迹"走"出来

对每条 $u_i$，用全向轮运动模型一步步积分（源码 `optimizer.cpp` 的 `integrateStateVelocities`，333-336 行）：
$$\begin{aligned}
\Delta x &= (v_x\cos\theta - v_y\sin\theta)\,\Delta t \\
\Delta y &= (v_x\sin\theta + v_y\cos\theta)\,\Delta t \\
\theta_{t+1} &= \theta_t + \omega_z \Delta t
\end{aligned}$$
**人话版：** 把"车身系速度"旋转到世界系，×dt 累加位置；朝向按角速度积分。这就是你 base_control 里程计公式的"MPPI 内版本"——同一个公式，两次登场。
数字例子：$v_x=0.30$，$\theta=0$，$dt=0.05$ → 一步前进 $0.015$m；56 步全速前进 ≈ $0.84$m（2.8 秒内最多走这么远——这就是"2.8 秒预演"的空间尺度）。

### 6.5 打分：8 个 critics 就是 8 位"考官"

每条轨迹的最终代价 = 各 critic 打分 × 权重求和（同一条轨迹里，越靠后的时刻用 $\gamma^t$ 轻微衰减，体现"眼前比远处重要"）。你的 8 位考官（本仓库层，全部来自 `nav2_params.yaml`）：

| Critic | 主要考察什么 | 本项目中的关键点 |
| --- | --- | --- |
| ConstraintCritic | 速度与运动约束是否合法 | 先排除不满足底盘约束的轨迹 |
| GoalCritic | 是否接近目标位置 | 进入目标附近后退出，避免干扰精细收尾 |
| GoalAngleCritic | 最终朝向是否正确 | 负责目标附近的姿态收敛 |
| PreferForwardCritic | 是否出现倒车 | 权重 15；最后 0.5 m 内退出 |
| CostCritic | 轨迹是否靠近或碰撞障碍 | footprint 碰撞代价为 1,000,000，近似一票否决 |
| PathAlignCritic | 轨迹方向是否与路径一致 | 权重 14，抑制左右摆动 |
| PathFollowCritic | 是否沿路径向前推进 | 把候选轨迹拉回全局路径 |
| PathAngleCritic | 车头方向是否匹配路径 | 影响拐弯与转向积极性 |

三个必须讲清楚的设计点：
**① `threshold_to_consider` 的语义（容易理解反！）**：它是"退出距离"——距离目标（路径终点）小于该值时，这个 critic 不再生效。源码里是 `withinPositionGoalTolerance(...)` 为真就 `return`（不贡献代价）。所以：

- GoalCritic 在最后 1.4m 内"闭麦"——因为接近目标时，靠 PathFollow/GoalAngle 做精细收尾，粗粒度的"离目标距离"反而会抢戏；
- PreferForward 在最后 0.5m 内"闭麦"——这就是允许最后半米倒车/侧移收尾的设计（你配置注释里的"靠近目标 0.5m 内才允许侧移停靠"说的正是这件事）。

**② PreferForwardCritic 权重 15 的动机（你的 [HEADFIRST] 调参）**：它惩罚的是 `max(-vx, 0)`——只罚向后，不罚横移。源码一行讲完：`backward_motion = max(-vx, 0)`。为什么你把它调得比 PathAlign(14) 还高？——因为麦轮的里程计横移/侧向时漂移最大，而它又开在 gmapping 的 map→odom 上；让车头始终朝前、少做非纵向机动能显著压低累积 drift，让建图与定位都更稳。代价是机动性略降（近似变成"差速车"），这是本项目明确接受的权衡。
③ CostCritic 的"软硬结合"：distance 到障碍还很远时只按代价地图（第 3 章那张指数衰减图）扣分；一旦轨迹的 footprint 真的压上 254（或越过 `critical_cost`）就一口气加 `collision_cost: 1000000`——近似"一票否决"（100 万对比其它 critic 的个位数~几十，等于宣判）。

### 6.6 组合：softmax 加权（全章最核心的三个公式）

**理论层：** MPPI 对不同候选的控制序列做"指数加权平均"：
$$w_i = \frac{\exp(-C_i/\lambda)}{\sum_j \exp(-C_j/\lambda)}
\qquad
u_{new} = \sum_i w_i \, u_i$$

- $C_i$：第 $i$ 条轨迹的总代价；$\lambda$：温度（配置 temperature）。
- 人话版：代价每低一点，权重按 指数 放大；$\lambda$ 越小 → 越"唯分数论"（近似只取最优）；$\lambda$  越大 → 越"佛系平均"。

**Nav2 层：** `optimizer.cpp` 的实现（1.1.16 源码 356-388 行）比教科书多了两步工程处理：
// ① 重要性采样的修正项（gamma 出现的地方）：
//    采样以"当前序列 u"为中心，数学推导会额外带一项 uᵀΣ⁻¹ε 的修正
costs_i += gamma / σ_vx² * Σ_t (u_t · (cvx_i,t - u_t))  (+ wz、vy 同理)

// ② 数值稳定：整体减去最小值（exp 的公共因子会约掉，不影响结果）
costs -= min(costs)

// ③ softmax 加权平均（逐维）
w_i = exp(-costs_i / temperature);  w_i /= Σw
u_new.vx = Σ_i w_i * cvx_i;  u_new.wz = Σ_i w_i * cwz_i;  (holonomic 再加 vy)
其中 `cvx/cwz/cvy` 就是经过速度上下限截断（bounded）后的采样序列——所以超限的采样不是被扔掉，而是"压回上限参与竞争"。
**本仓库层：** `temperature: 0.3`、`gamma: 0.015`。

- $\lambda=0.3$ 很小，含义：**代价值每差 1，权重就差 $e^{1/0.3}\approx 28$ 倍**——所以你的 MPPI 实际相当"果断"，接近取最优（但仍保留平滑性）。critics 权重（3~15）和温度是一起起作用的："权重比"和"温度"共同决定谁说了算。
- $\gamma=0.015$ 很小，含义：重要性采样那层修正很轻，几乎可以忽略——你可以把 6.6 的前两个公式当作主要矛盾。

### 6.7 为什么是"加权平均"而不是"直接取最优"

取最优（argmin）的三个坏处：

1. 抖动：最优轨迹在两条之间随机跳，速度指令跟着跳；
2. 没有"将功补过"：几条都不完美时，average 能把它们的"优点"拼起来（这条躲得远、那条方向好）；
3. 探索退化：全押一条会让下次采样失去多样性。

软加权还有个漂亮的数学性质：$\lambda \to 0$ 时它收敛到取最优，$\lambda \to \infty$ 时收敛到简单平均——温度是一个"从死板到佛系"的连续旋钮，不用改结构就能调"性格"。

### 6.8 其它关键旋钮

| 参数 | 本项目值 | 作用 |
| --- | ---: | --- |
| `time_steps` | 56 | 预测离散步数 |
| `model_dt` | 0.05 s | 每个预测步的时间长度；总视野为 2.8 s |
| `batch_size` | 2000 | 每个控制周期采样的候选轨迹数 |
| `temperature` | 0.3 | softmax 温度；越小越偏向低代价轨迹 |
| `gamma` | 0.015 | 控制扰动的重要性采样修正系数 |
| `prune_distance` | 1.7 m | 只保留机器人前方一段路径供控制器计算 |
| `transform_tolerance` | 0.2 s | TF 查询的时间容差 |
| `visualize` | true | 发布最优轨迹等调试信息 |

### 6.9 手算一个小型 MPPI 更新（里程碑 B）

设只有 3 条采样（真实是 2000），只看 vx 维，当前序列 $u=[0.30]$（单步简化）。三条采样与打分结果：

| 轨迹 $i$ | 采样 $u_i$ | 代价 $C_i$ | 减最小值后 | $w_i=\exp(-C_i'/0.3)$ | softmax |
| --- | --- | --- | --- | --- | --- |
| 1 | 0.32 | 12.0 | 0.0 | $e^{0}=1$ | 0.9986 |
| 2 | 0.28 | 14.0 | 2.0 | $e^{-6.67}=0.0013$ | 0.0013 |
| 3 | 0.10 | 22.0 | 10.0 | $e^{-33.3}\approx 3\times10^{-15}$ | ≈0 |

$$u_{new} = 0.9986\times0.32 + 0.0013\times0.28 + 0\times0.10 \approx 0.3199$$
体会三件事：

1. 结果非常接近最优那条（0.32），但不是直接抄——被其它轨迹轻轻拉了一下（这就是"既果断又平滑"）；
2. 代价差 2 个单位（12 vs 14），权重差约 770 倍——温度 0.3 下"微小劣势"被指数放大；
3. 那条 0.10 的轨迹其实"完全被无视了"——但它的代价仍然参与计算（万一某一周期所有轨迹都糟，softmax 也能给出一条"最不糟的加权"）。

### 6.10 和别的局部控制器对比（知道 Nav2 里还有什么）

| 控制器 | 原理一句话 | 特点 | 本项目为何不用 |
| --- | --- | --- | --- |
| MPPI（本项目） | 采样+软加权 | 全向/差速通吃、避障强、平滑；算力需求高 | — |
| DWB | 采样+打分取优（DWA 的 ROS2 版） | 简单稳、算力低 | 实现简单、资源占用低，但本项目更看重 MPPI 的平滑性与复杂代价处理能力 |
| TEB | 图优化（时间弹性带） | 轨迹漂亮 | 优化求解慢、参数玄学 |
| RPP | 纯跟踪几何曲线 | 极轻量 | 避障弱，适合简单跟踪 |

### 6.11 controller_server 的每一拍：外圈流水线（源码 computeAndPublishVelocity 全解）

MPPI 是心脏，但心脏外面还有一圈"管路"——每 50ms 的完整流水线（源码 controller_server.cpp 460-530 行，已逐行核对）：
每一拍（20Hz）：
 ① getRobotPose(pose)                       # 查 TF(map→base_link) 拿当前位姿
                                            #   失败 → 抛异常 "Failed to obtain robot pose"
 ② progress_checker_->check(pose)           # SimpleProgressChecker: 10 秒内没动够 0.5m →
                                            #   抛 "Failed to make progress"（防卡死）
 ③ getThresholdedTwist(odom_sub_->getTwist())   # 从 /odom 取速度, 并用三个
                                            #   min_{x,y,theta}_velocity_threshold(0.001) 做死区:
                                            #   小于阈值的速度直接当 0 (吸掉静止噪声)
 ④ 调 MPPI 插件 computeVelocityCommands(pose, twist, goal_checker)
                                            # ★ 第 5、6 章的全部内容发生在这里
 ⑤ 异常处理（failure_tolerance=0.3 的"宽容窗口"）:
      控制器抛异常 → 先发零速继续试, 若"上次成功出速度"距今 > 0.3s
      → 才抛 "Controller patience exceeded" 给行为树（触发恢复）
      （对比: failure_tolerance=0 → 立即上报; = -1 → 永不放弃, 一直试）
 ⑥ 组 feedback: speed + distance_to_goal     # 沿路径找"离车最近的点"(遍历比较),
                                            #   再累加剩余段长度 → 发给 action 反馈
 ⑦ publishVelocity → /cmd_vel_nav            # 交给 velocity_smoother
三个值得记住的设计：

- "失败也有宽容期"：单拍算不出速度（比如瞬时 TF 抖动）不算失败——0.3 秒内继续发零速观察，超时才惊动行为树。这避免了"恢复行为滥触发"。
- 速度死区：0.001 阈值把里程计静止噪声归零——MPPI 里的 `PreferForward`（罚负速度）等 critics 会看速度符号，死区防止"噪声速度被当成倒车"。
- goal_checker 的双重身份：MPPI 内部每拍都会调它判断"是否已到终点"（用于收尾减速逻辑）；`SimpleGoalChecker` 的 `stateful: true` 表示"一旦判到就锁存"——即便机器人之后又被扰动漂出容差，也维持"已到达"状态（防止终点处的判定抖动）。

### 6.12 路径在控制器里长什么样：PathHandler 与"最近点"

全局路径是 map 系的长路径；MPPI 在局部系里以 50ms 的节奏工作——中间的翻译器就是 `PathHandler`（源码 `nav2_mppi_controller/src/path_handler.cpp`）：

1. 坐标变换：每拍把路径从 `global_frame`（map）变换到机器人当前所在的局部坐标系——所以"定位跳变"会立即反映到路径的相对位置上；
2. 找最近点：在路径上找离机器人最近的点（第 6.11 节第⑥步的同一个最近点概念）；
3. **剪枝（`prune_distance: 1.7`）**：只保留"最近点起、向前 1.7m"的一段给 MPPI。为什么？——MPPI 的预演窗口是 2.8 秒 × 最高 0.3m/s ≈ 最远 0.84m，1.7m 的路径视野绰绰有余；留整条路径只会拖慢打分（每拍要对路径逐点算距离代价）。
4. **`transform_tolerance: 0.2`**：查 TF 时容忍 0.2s 的"未来外推"——网络/计算延迟下 TF 最新帧可能比"现在"早一两拍，允许一点外推避免频繁查询失败。

n由此理解一个现象：机器人的实际轨迹可以明显偏离 `/plan` 的"表面形状"却不报错——因为 MPPI 眼里只有"前方 1.7m 的一段 + 障碍物"，它有权在局部自由发挥（这也是为什么全局路径"台阶状"完全不影响行驶品质，第 4.6 节）。

### 6.13 一次完整 MPPI 周期：12 步流水账（数字示意版）

把本章所有概念串成"一拍的完整故事"（数字为示意值，帮助建立数量感）：
设定: 车在 (0.50, 0.20), 朝向 12°, 当前速度 (0.25, 0, 0.30)
路径: 全局路径变换到局部系并剪枝 → 前方 1.7m 内的 34 个路点

① 取上一拍的优化序列 u* (56 步 × [vx,vy,wz]) 作为"当前名义控制"
② 撒噪声: 每条轨迹 u_i = u* + ε,  ε_vx~N(0,0.20²), ε_wz~N(0,0.40²) → 2000 条
③ 逐条前向积分 56 步 (dt=0.05): Δx=(vx·cosθ−vy·sinθ)·dt ... → 2000 × 56 个位姿
④ 打分 (8 位 critic, 权重见 6.5 表):
     - 某条轨迹擦过膨胀区(代价 180) → CostCritic 记 ~3.81×180 量级
     - 某条轨迹车头偏出路径切线 20° → PathAngleCritic 记 penalty
     - 全部轨迹: 累积 cost 大致落在 10 ~ 150 的量级区间

⑤ cost 加上 gamma 修正项(6.6 节 ①), 再整体减最小值(②)
⑥ softmax: w_i = exp(−C_i′/0.3) 归一化 → 代价值每差 1, 权重差 ~28 倍
     → 实际效果: 前 ~50 条好轨迹占了 >90% 权重
⑦ 加权平均: u_new[每步][每维] = Σ w_i · u_i → 得到"兼顾最优与平滑"的新序列
⑧ 限幅: vx∈[−0.2, 0.3], vy∈[−0.05, 0.05], wz∈[−1.5, 1.5] (applyControlSequenceConstraints)
⑨ 序列左移一格 (shift_control_sequence), 整条序列平移向前 50ms
⑩ 取第 0 格 → 组装 TwistStamped: (vx=0.27, vy=0.00, wz=0.33)
⑪ 发布 /cmd_vel_nav → velocity_smoother 做加速度合规 → /cmd_vel → 底盘
⑫ 一拍结束; 50ms 后带着(可能已变的)新位姿从头再来
数量级感受：一拍里"算"的量 ≈ 2000 条 × 56 步 × (积分 + 8 项打分) ≈ 千万次浮点量级——全部向量化后落在 RK3588 单核的 50ms 预算内（实测 Nav2 全栈约占 1 个核）。

### 6.14 本章小结

- MPPI = 撒 2000 条 2.8 秒的候选（56×0.05s），按 8 个 critics 打分，再按 $e^{-C/\lambda}$ 软加权平均，取第一格执行，滚动重来。
- 两个"性格参数"：$\lambda$（温度，果断↔佛系）与 γ（重要性修正强度）；噪声三兄弟（vx/vy/wz_std）决定"探索胆量"。
- 你的配置是一台明确偏保守的车：速度上限 0.3m/s、横移 0.05m/s、PreferForward=15 强制车头朝前（压麦轮漂移）。
- 高频调试抓手：RViz 打开 /optimal_trajectory（visualize: true 已开）+ /local_costmap/costmap。
- 代码对照：controller.cpp（主循环）、optimizer.cpp（6.6 的公式就在 356-388 行）、critics/*.cpp（8 位考官各自文件）、noise_generator.cpp（采样）。

---

## 第 7 章　行为树：导航任务的"剧本"

### 7.1 为什么"导航流程"本身也需要一个算法

你已经有了组件：规划器会规划、控制器会跟随。但"先规划后跟随、跟随失败就清代价地图重试、再失败就转圈再退一步"这套流程谁来管？

- 用代码写死 if-else？流程一变就要改编译，且难以组合；
- 用状态机？状态一多就爆炸（状态 × 事件矩阵）。

**Nav2 层：** Nav2 用行为树（Behavior Tree）——流程本身变成一份数据（一棵随时可换的树），由 bt_navigator 解释执行。

### 7.2 行为树 60 秒入门

行为树由节点组成，每个节点执行后返回三种状态之一：SUCCESS / FAILURE / RUNNING。基本元件（BT.CPP 库的术语）：

| 节点类型 | 规则 | 直觉 |
| --- | --- | --- |
| Action | 执行具体任务，可返回 SUCCESS / FAILURE / RUNNING | 干活的人 |
| Condition | 检查条件，通常立即返回成功或失败 | 做判断的人 |
| Sequence | 子节点从左到右执行，任一失败则整体失败 | 全部都要成功 |
| Fallback | 子节点从左到右尝试，任一成功则整体成功 | 这个不行就换下一个 |
| Decorator / Control | 改变子节点执行频率、重试或组合逻辑 | 调度规则 |

直觉示例：

```text
Fallback（"要么好好走，要么兜底"）
 ├── Sequence（正常导航：先规划再跟随，两步都成功才算成功）
 │     ├── ComputePathToPose
 │     └── FollowPath
 └── Recovery（兜底：转圈/后退）
```

正常路径失败了，就滑到右边兜底——这就是"失败了怎么办"的表达方式。

### 7.3 你的机器人实际跑的树（逐节点解析）

**本仓库层：** 你的配置没有指定自定义树 → 用 Nav2 默认的 `navigate_to_pose_w_replanning_and_recovery.xml`（源码 `nav2_bt_navigator/behavior_trees/`，下文 XML 为 1.1.16 原文摘录）：
<RecoveryNode number_of_retries="6" name="NavigateRecovery">          ← 总兜底：最多重试 6 次
  <PipelineSequence name="NavigateWithReplanning">                     ← "边规划边跟随"流水线
    <RateController hz="1.0">                                          ← 每秒最多重规划 1 次
      <RecoveryNode number_of_retries="1" name="ComputePathToPose">    ← 规划 + 失败清全局代价地图
        <ComputePathToPose goal="{goal}" path="{path}" planner_id="GridBased"/>
        <ClearEntireCostmap service_name="global_costmap/clear_entirely_global_costmap"/>
      </RecoveryNode>
    </RateController>
    <RecoveryNode number_of_retries="1" name="FollowPath">             ← 跟随 + 失败清局部代价地图
      <FollowPath path="{path}" controller_id="FollowPath"/>
      <ClearEntireCostmap service_name="local_costmap/clear_entirely_local_costmap"/>
    </RecoveryNode>
  </PipelineSequence>
  <ReactiveFallback name="RecoveryFallback">                           ← 上面整体失败后的“兜底菜单”
    <GoalUpdated/>                                                     ← 目标换了？直接重来
    <RoundRobin name="RecoveryActions">                                ← 依次轮流尝试：
      <Sequence name="ClearingActions">
        <ClearEntireCostmap service_name="local_costmap/clear_entirely_local_costmap"/>
        <ClearEntireCostmap service_name="global_costmap/clear_entirely_global_costmap"/>
      </Sequence>                                                      ← ① 清两层代价地图
      <Spin spin_dist="1.57"/>                                         ← ② 原地转 90°
      <Wait wait_duration="5"/>                                        ← ③ 等 5 秒
      <BackUp backup_dist="0.30" backup_speed="0.05"/>                 ← ④ 后退 0.3m
    </RoundRobin>
  </ReactiveFallback>
</RecoveryNode>
几处值得琢磨的设计：

1. **`PipelineSequence` + `RateController`**：跟随路径的同时，每秒重规划一次（世界在变，路径要跟上）。这就是"边跑边修路"。
2. **`planner_id="GridBased"` 与 `controller_id="FollowPath"`**：树里写死的这两个名字，正是你 `nav2_params.yaml` 里 `planner_plugins: ["GridBased"]` 和 `controller_plugins: ["FollowPath"]` 的对应项——树通过名字引用插件。
3. 两级恢复：局部的（清代价地图重试 1 次）→ 全局的（转圈/等待/后退，最多 6 轮）。你之前遇到的"车原地转 90°"就是 ② 在执行。
4. 为什么先清代价地图：动态障碍或定位抖动可能在代价地图里留下"幽灵障碍"，清了再试是最便宜的修复。

### 7.4 任务怎么进来：NavigateToPose action（以及一个真实修过的坑）

**Nav2 层：** 导航任务从外面进 bt_navigator 只有一条正路：**ROS2 action `/navigate_to_pose`**（类型 `nav2_msgs/action/NavigateToPose`）。它的三件套：
部分
内容
用途
goal
一个 PoseStamped（目标位姿）
下单
feedback
current_pose、distance_remaining、speed、number_of_recoveries…
每秒回播"还剩多远、恢复了几次"
result
导航总耗时、剩余距离、恢复次数
交单（成功/失败）
两种下单方式：

- RViz 工具：Nav2 官方 rviz 插件里的 "Nav2 Goal" 工具（`nav2_rviz_plugins/GoalTool`）——它直接调用 action，点一下地图就下单；
- 命令行/程序：

ros2 action send_goal /navigate_to_pose nav2_msgs/action/NavigateToPose \
  "{pose: {header: {frame_id: map}, pose: {position: {x: 1.0, y: 0.0}, orientation: {w: 1.0}}}}" --feedback
真实坑（本项目已修）：普通 RViz 工具 `rviz_default_plugins/SetGoal` 只发布 `/goal_pose` 话题——这是 ROS1 时代的遗留，Humble 的 Nav2 里没有任何节点订阅它。你原来的 `nav.rviz`/`slam.rviz` 里装的正是这个工具：在 RViz 点目标根本没反应（目标从未到达 bt_navigator）。已把两个 rviz 配置里的目标工具换成 `nav2_rviz_plugins/GoalTool`（直连 action），验证通过。
这个坑值得记住两点：① "发了话题"不等于"有人处理"——用 ros2 topic info /goal_pose 看有没有订阅者；② 排查导航"点了没反应"时，第一步是 ros2 action list | grep navigate，确认 action 在，再看你用的工具是不是真会去调它。

### 7.5 行为树运行机理：tick、黑板与三个关键控制节点的精确语义

**Nav2 层：** 把第 7.2 节的"元件"落到运行时的三个机制上：
① tick（心跳）：bt_navigator 以 `bt_loop_duration: 10`（ms）的节奏从根节点往下走一遍——每个节点被执行一次叫"tick"。RUNNING 状态的节点靠"每拍被重新 tick"来持续推进任务。一棵树不是"跑一遍就完"，而是"10ms 拍一次的视频"，而不是"一张照片"。
② 黑板（blackboard）与端口（port）：树上节点之间靠黑板键传数据——XML 里的 `{goal}`、`{path}` 就是黑板键：

- `ComputePathToPose` 的 output port `path` 把规划结果写进 `{path}` 键；
- `FollowPath` 的 input port `path` 从 `{path}` 键读出来用。

所以"规划器的输出接到控制器的输入"这件事，在树里就是两个节点共享一个黑板书签。
③ 每个树节点 = 一个 C++ 插件 = 一层 action 封装：你在 `plugin_lib_names` 列表里注册的行（第 13.2 节），对应的就是 `nav2_behavior_tree/plugins/` 里的类。例如 `ComputePathToPose` 节点内部其实是一个 action client，去调 planner_server 的 `/compute_path_to_pose`；action 返回的 error code 被翻译成树的 SUCCESS/FAILURE。行为树本质上是"action 调用的编排层"。
④ 三个关键控制节点的精确语义（前两个是 BT.CPP 标准节点，第三个是 Nav2 自创——理解它才理解"边规划边跟随"）：

| 节点 | 子节点 RUNNING 时 | 子节点 FAILURE 时 | 记忆法 |
| --- | --- | --- | --- |
| Sequence（顺序） | 返回 RUNNING，下拍从该子节点继续（不回头） | 整体失败 | "一次性流水线" |
| ReactiveSequence（反应式） | 返回 RUNNING，下拍从头重 tick（前面成功的节点也重新判） | 整体失败 | "警惕的流水线" |
| PipelineSequence（流水线） | 重 tick 之前所有成功过的节点 | 整体失败并停掉全部 | "多任务并行的流水线" |

PipelineSequence 的官方注释举例（源码 pipeline_sequence.hpp，直接翻译）：
三个子节点 A, B, C:
  A RUNNING                    → 等 A
  A SUCCESS                    → 开始 tick B
  A RUNNING(被重tick), B SUCCESS → 开始 tick C
  A RUNNING, B SUCCESS, C SUCCESS → 整体 SUCCESS, 停掉 A
套到你的树上：`PipelineSequence[ RateController→ComputePathToPose , FollowPath ]` 的含义就是——FollowPath 正在跑（RUNNING）时，左边的规划子树每拍都被重新 tick，但被 `RateController(hz=1.0)` 限住"真正重规划每秒才一次"。"边跑边修路"就是这么实现的。
另外两个装配套件：RecoveryNode（第 7.3 节）把"主逻辑+恢复分支"打包并计数重试；RoundRobin（轮流）让恢复动作按顺序轮换（清图→转圈→等待→后退，每触发一次换下一个）。

### 7.6 换一份"剧本"：自定义行为树（入门）

你的配置没有设置 default_nav_to_pose_bt_xml → 用 Nav2 默认树。想改流程（比如去掉恢复、改后退距离、加自定义动作节点）：

1. 把 nav2_bt_navigator/behavior_trees/navigate_to_pose_w_replanning_and_recovery.xml 拷一份改；
2. 把 bt_navigator 参数 default_nav_to_pose_bt_xml 指向你的文件（绝对路径可用）；
3. 重启 `slam_nav`，看 bt_navigator 日志确认加载成功（XML 写错会让 bt_navigator configure 失败，症状是 lifecycle 卡 unconfigured，第 11.2 节）。

最好的学树方法：用 `Groot`（BehaviorTree.CPP 的图形编辑器，`sudo apt install ros-humble-groot`）打开默认树，一边点节点一边对照第 7.3 节——树的可视化就是它设计的初衷。

### 7.7 本章小结

- 行为树把"导航流程"变成可替换的数据；三态（SUCCESS/FAILURE/RUNNING）+ 四种元件（Action/Condition/Sequence/Fallback）+ 装饰器。
- 默认树的核心结构：每秒重规划 + 跟随，失败走两级恢复（清代价地图 → 转圈/等待/后退）。
- 树通过 planner_id/controller_id 名字与你的插件配置对应。
- 代码对照：nav2_bt_navigator/src/bt_navigator.cpp（执行）、behavior_trees/*.xml（剧本）。

---

## 第 8 章　恢复行为与速度平滑

### 8.1 五个恢复行为（behavior_server）

**本仓库层：** 你的 `behavior_plugins` 启了五个（源码 `nav2_behaviors/plugins/`）：

| 行为 | 干什么 | 关键参数（默认/你的值） | 什么时候被树调用 |
| --- | --- | --- | --- |
| Spin | 原地旋转 | XML 里 spin_dist=1.57（90°） | 全局恢复第 2 步 |
| BackUp | 按给定速度后退一段 | XML 里 backup_dist=0.30, speed=0.05 | 第 4 步 |
| Wait | 原地等待 | XML 里 wait_duration=5 秒 | 第 3 步（让行人先过） |
| DriveOnHeading | 沿当前朝向前冲一小段 | 用于被卡住时"蹭出来" | 默认树不用，可自定义 |
| AssistedTeleop | 有人遥控时帮忙避障 | 遥控兜底 | 默认树不用 |

行为服务器本身也有安全限速：max_rotational_vel: 1.0、min 0.4、角加速度 3.2 rad/s²——所以"转 90°"不是瞬间完成的。
注意这些行为也要经过 velocity_smoother（launch 里把 behavior_server 的 `cmd_vel` 也 remap 到了 `cmd_vel_nav`）——恢复动作也不许急起急停。

### 8.2 速度平滑器：最后一道"物理合规"闸门

MPPI 输出的速度理论上已满足上限，但周期之间的变化率（加速度）没人管——上一拍 0.3m/s、下一拍 -0.2m/s 在数学上合法，在真实电机上会打滑/顿挫。velocity_smoother 补上这一课。
**理论层：** 开环加速度限制算法（源码 `nav2_velocity_smoother/src/velocity_smoother.cpp`）：

1. 取"当前速度"：开环模式 = 上一条已发出的指令（`feedback: "OPEN_LOOP"`）；闭环模式则用 `/odom` 实测速度；
2. 绝对限幅：$v \leftarrow \text{clamp}(v, v_{min}, v_{max})$，你的值 $[0.30, 0.05, 1.5]$ / $[-0.20, -0.05, -1.5]$；
3. 加速度限幅：每拍允许的最大变化量 = `max_accel/max_decel` × Δt（你的 $a_{max}=[2.5, 2.5, 3.2]$，20Hz → 每拍最多变 0.125 m/s、0.16 rad/s）；限幅前先判断这拍是"加速"还是"减速"（同向且速度幅值在变大 → accel；否则 → decel）再套用对应上限。注意：**"按同一比例缩小所有轴、保持方向不变"的比例缩放（源码里的 `eta`）只在 `scale_velocities: true` 时启用**——你的配置是 `false`，即三个轴各自独立限幅（详见 8.5 节）；
4. 平滑输出 cmd_vel_smoothed → remap 到 /cmd_vel → base_control。

**安全细节：** `velocity_timeout: 1.0`——1 秒内没收到新指令就自动把速度归零。这是"层级之间断线"的最后保护：BT 挂了、controller 卡了，车也会在一秒内停住。

### 8.3 指令链路全景（把它和 launch 文件对齐）

```text
# nav2.launch.py 里三处 remap 的完整含义：
controller_server:  cmd_vel        → cmd_vel_nav     # MPPI 的输出改名
behavior_server:    cmd_vel        → cmd_vel_nav     # 恢复行为输出改名
velocity_smoother:  cmd_vel        → cmd_vel_nav     # 平滑器的输入
                    cmd_vel_smoothed → cmd_vel       # 平滑器的输出接到底盘
```

为什么要绕这一圈？因为 **controller 和 behavior 都想用 `/cmd_vel` 这个名字**，谁都不该直接连到底盘——于是约定：内部都用 `cmd_vel_nav`，**唯有平滑器有资格"下发"到真正的 `/cmd_vel`**。这是 ROS2 里很典型的"命名空间约定优于配置"。

### 8.4 恢复行为的实现结构（以 Spin 为例）

**Nav2 层：** 五个行为共享同一个基类结构（`timed_behavior`）：周期以 `cycle_frequency: 10Hz` 运行，每隔拍产生一次速度，直到"目标达成 / 超时 / 判定无法完成"。
Spin 的完整生命（源码 `nav2_behaviors/plugins/spin.cpp`，已核对）：

1. onRun：收到 spin_dist（树的 XML 里传 1.57 rad）→ 计算转速（受 max_rotational_vel: 1.0、min_rotational_vel: 0.4、rotational_acc_lim: 3.2 约束的梯形速度规划）；
2. `onCycleUpdate`（每 100ms）：用局部代价地图做"旋转扫掠碰撞预测"（`isCollisionFree`：把机器人将要转过的扇区模拟出来，查是否会碰到障碍）——安全则发 `angular.z`，会撞就停；
3. 累计转过的角度（用里程计反馈），达到目标 → SUCCESS；超时 → FAILURE。

**关键认知：** 恢复行为不是"闭眼挣扎"——它们每个都在安全框架内：会做碰撞预测、速度受限、走过平滑器。BackUp（直线后退）与 DriveOnHeading（沿当前朝向前进一段）结构同构，都有碰撞预测；Wait 是纯计时；AssistedTeleop 是"人工遥控指令的避障过滤转发"（有人开的时候副驾帮踩刹车）。

### 8.5 velocity smoother 的完整逻辑（applyConstraints 逐行）

把第 8.2 节的四步展开到源码级（velocity_smoother.cpp，已核对）：

```text
① 取"当前速度"current_:
     open_loop  → last_cmd_   (上一条已平滑的输出指令)
     closed_loop→ /odom 实测速度
② 绝对限幅: clamp(command, min_velocities, max_velocities)   ← 三轴各自 clamp
③ 加速度限幅 applyConstraints(current, command, accel, decel, eta)
     核心三行 (源码 225 行起):
       dv = command - current
       判定: 同向且 |command| ≥ |current| → 走"加速"分支用 max_accel
             否则(减速/过零/反向)       → 走"减速"分支用 max_decel
       command = current + clamp(dv, ±限制×Δt)
④ 比例缩放(eta): 只有 scale_velocities=true 才启用
     findEtaConstraint 算出三轴中"需要缩得最狠"的系数 eta∈(0,1],
     三轴同乘 eta, 保证运动方向不变(但整体变慢)
     ★ 你的配置 scale_velocities: false → eta 恒为 1 → 三轴各自独立限幅
⑤ 死区: |v| < deadband_velocity → 归零 (你=0, 未启用; 设 0.02~0.05 可吸静止微抖)
⑥ 发布 cmd_vel_smoothed (→ remap 到 /cmd_vel);
   若无新指令超过 velocity_timeout=1.0s → 持续发全零(失联刹车)
```

两种限幅模式的取舍（你选 false 的原因分析）：

- 独立限幅（false，你的选择）：每个轴各管各的，响应"直给"——但"大转向 + 大前速"同时超出时，实际执行的方向和你请求的方向会有一点点偏差；
- 比例缩放（true）：严格保持"请求速度矢量的方向"，但整体按最狠的那个轴缩——更"稳"也更"肉"。

对速度上限只有 0.3m/s、且行动以车头朝前为主的你这台车，独立限幅的"偏差"几乎不可感知，响应直接更重要。
附带一个没用到的能力：smoother 支持外部 `speed_limit` 话题做动态限速（比如"接近人群自动限 0.1m/s"）——本项目未用，留作扩展。

### 8.6 本章小结

- 恢复行为 = 优雅的"自救动作库"（转圈/后退/等待/前蹭/辅助遥控），由行为树按剧本调用。
- 速度平滑器 = 最后一层物理合规：限幅 + 加速度约束 + 1 秒失联停车。
- 内部速度话题统一走 cmd_vel_nav，只有平滑器输出连到 /cmd_vel。

---

## 第 9 章　定位：AMCL（有图导航模式）

无图模式下定位由 gmapping 顺带给出（你已经全会了）。本章讲"有图模式"的专职定位员。

### 9.1 AMCL 和 gmapping 的血缘关系

AMCL = Adaptive Monte Carlo Localization（自适应蒙特卡洛定位）。看到 Monte Carlo 就知道——又是粒子滤波（gmapping 教程第 3 章）。两者的本质区别：

| 对比项 | gmapping | AMCL |
| --- | --- | --- |
| 地图 | 边定位边构建 | 已知且固定 |
| 粒子表示 | 轨迹假设，并关联各自的地图 | 机器人位姿假设 |
| 主要任务 | SLAM：定位 + 建图 | Localization：仅定位 |
| 计算量 | 较高 | 较低 |
| 本项目模式 | `slam_nav` | `nav_with_map` |

**一句话：** AMCL 是"地图已知版"的粒子滤波——省掉了地图维度，粒子可以少很多、算得很快。

### 9.2 运动模型：OmniMotionModel（5 个 alpha）

粒子怎么"随机器人动"？——运动模型（gmapping 教程第 9 章的同款概念）：

- 差速车使用 DifferentialMotionModel；本项目使用全向模型；
- 你的是麦轮，所以配置 `robot_model_type: "nav2_amcl::OmniMotionModel"`（源码 `nav2_amcl/src/motion_model/omni_motion_model.cpp`）——5 个噪声参数：

```yaml
alpha1: 0.2   # 旋转中的旋转噪声
alpha2: 0.2   # 平移中的旋转噪声
alpha3: 0.2   # 平移中的平移噪声
alpha4: 0.2   # 旋转中的平移噪声
alpha5: 0.2   # ★ Omni 独有：横向平移噪声
```

**人话版：** AMCL 每个周期把每个粒子按"里程计的位移 + 噪声"往前撒一把。alpha 越大粒子撒得越散（越不信任里程计）。记住 5 号——只有全向模型才有横向噪声概念，这是你这个配置不能换成差速模型的硬原因。

### 9.3 传感器模型：似然场（likelihood field）——AMCL 的"眼睛"

**理论层：** 粒子撒出去后怎么知道"撒的位置像不像"？拿这帧激光和地图比对。AMCL 提供三种模型，你配置的是 `laser_model_type: "likelihood_field"`（源码 `nav2_amcl/src/sensors/laser/likelihood_field_model.cpp`）：
似然场的两阶段思想：

1. 离线预计算（`nav2_amcl/src/map/map_cspace.cpp`）：给地图每个格子算"到最近障碍的距离" $d$，存成一张距离场；
2. 在线打分：对每个粒子的每个激光束，取"激光命中点在地图上的位置"，查距离场得 $d$，则这一束的似然：

$$p(z \mid x, m) \propto z_{hit}\cdot e^{-d^2/(2\sigma_{hit}^2)} + \frac{z_{rand}}{z_{max}} + z_{max}\cdot[\text{测距=最大}]$$
   人话版：激光点离地图上的墙越近，似然越高（高斯峰）；再混一点"随机噪声概率"兜底。全部束的似然相乘 = 这个粒子的"像不像"。
你的参数：max_beams: 60（每帧只抽 60 束算——省算力的常见工程手段）、laser_max_range: 8.0、likelihood_max_dist: 2.0（距离场最大计算距离）、权重份额 z_hit: 0.5、z_rand: 0.5。
和 gmapping 的关系：gmapping 教程第 8 章的"似然场打分"讲的是同一套思想（打偏的束按离墙距离惩罚）——恭喜，你已经学过 AMCL 的眼睛了，只不过 AMCL 是"打在静态地图上"，gmapping 是"打在粒子自己的地图上"。

### 9.4 自适应粒子数：KLD 采样

为什么"自适应"：走廊里直着走，位置很确定——几百个粒子就够了；在开阔处做 360° 转向，不确定性大——需要几千。固定粒子数会很浪费。
算法：KLD（KL 散度）采样——根据粒子分布"展开的程度"动态决定粒子数：
$$n \approx \frac{k-1}{2\varepsilon}\left(1-\frac{2}{9(k-1)}+\sqrt{\frac{2}{9(k-1)}}z_{1-\delta}\right)^3$$
**人话版：** 分布越散（占据的不同网格越多），算出来的 $n$ 越大；公式里的 $\varepsilon$（`pf_err: 0.05`）是"允许的近似误差"，$z$（`pf_z: 0.99`）是置信度。你的上下限：

```yaml
min_particles: 500      # 至少 500 个（再确定也不少于）
max_particles: 2000     # 至多 2000 个（再迷糊也不超过）
```

### 9.5 什么时候更新、什么时候重采样

- 更新触发：不是每帧激光都算——只有机器人动得够多才算：`update_min_d: 0.25m` / `update_min_a: 0.2rad`。人话版：站着不动的帧拿去算定位是浪费。
- 重采样：`resample_interval: 1`（每次更新都可能重采样，但受"有效粒子数"保护，参考 gmapping 教程第 6 章 neff 的思想）。
- 恢复机制你关掉了：`recovery_alpha_slow: 0.0`、`recovery_alpha_fast: 0.0`——这是 AMCL 检测"绑架"（被人搬走）并全局重定位的机制；你关掉它换取"走廊里定位更稳"（配置注释的原话）。代价：真被搬走了要手动在 RViz 用 2D Pose Estimate 重定位。

### 9.6 初始位姿：AMCL 的"开机仪式"

AMCL 必须知道一个大概的初始位置才能开始。你的配置选择了自动化：

```yaml
set_initial_pose: true
initial_pose: {x: 0.0, y: 0.0, z: 0.0, yaw: 0.0}   # 假设开机位置 ≈ 地图原点
```

流程分两种：

- 开机位置 ≈ 地图原点：什么都不用做，AMCL 自动收敛；
- 否则：RViz 工具栏点 "2D Pose Estimate"，在地图上点机器人真实位置并拖出朝向 → 发 `/initialpose` 话题 → 粒子云瞬间扑向那里。

**工程技巧：** 地图原点 = 建图开始时机器人的位置（gmapping 的初始位姿）。所以**每次`有图导航`时，把机器人摆在"当初开始建图时站的位置"附近再开机**，就能白嫖自动初始化。

### 9.7 AMCL 的输出

- `map→odom` TF（第 2 章的那条"校正插头"）——这是它最重要的输出；
- /amcl_pose：当前位姿估计（带协方差）；
- `/particlecloud`：粒子分布（RViz 里看起来像"一片点云"，散得越开=越不确定，这是你肉眼诊断定位健康度的最佳窗口）。

### 9.8 AMCL 的更新循环精确结构（源码 laserReceived 主线）

**Nav2 层：** AMCL 的节拍由 `/scan` 驱动（不是 odom 回调！），完整流程（`nav2_amcl/src/amcl_node.cpp:618` 的 `laserReceived`，已逐段核对）：

```text
每来一帧 /scan:
 ① TF 查"这帧扫描时刻"机器人相对 odom 的位姿 (getOdomPose, 用 scan 时间戳!)
 ② 首帧? → 初始化滤波: pf_odom_pose_=pose, 强制发布
 ③ shouldUpdateFilter(pose, delta): 位移 ≥ 0.25m 或转角 ≥ 0.2rad 才置"需要更新"
     (节流: 站着不动的扫描不算, 省算力)
 ④ 需要更新 → motion_model_->odometryUpdate(pf_, pose, delta)
     ★ 运动更新: 每个粒子按"里程计增量 + 噪声"采样 (OmniMotionModel, 5 个 alpha)
 ⑤ updateFilter(...)
     ★ 观测更新: 对抽样出的 60 束激光逐束算似然场得分, 累加到粒子权重
 ⑥ 每 resample_interval(=1) 次更新 → pf_update_resample(pf_, map)
     ★ 重采样: 按权重比例重抽粒子; 粒子数在重采样时由 KLD 采样决定(500~2000)
```

 ⑦ 满足条件(刚重采样/首帧) → 发布 /amcl_pose、/particlecloud、map→odom TF
三个"所以"：

- 所以 AMCL 的算力消耗由"移动量"决定——推着车慢慢走比原地转圈更省；
- 所以 关闭的绑架恢复（第 9.5 节）一旦真被抬走，粒子只能靠"慢慢挪 + 似然自然拉回来"（通常拉不回来）——必须手动 2D Pose Estimate；
- 所以 你看到的"粒子云松紧"就是"当前位姿的不确定度"——第 ⑤ 步权重越平均（各粒子得分差不多），粒子云越难收敛，重采样后越散。

### 9.9 似然场的完整公式与各参数的确切含义

**理论层：** likelihood_field 模型对单束激光的似然（源码 `sensors/laser/likelihood_field_model.cpp` 的公式结构）：
$$p(z \mid x, m) \;\propto\; z_{hit}\cdot \frac{1}{\sqrt{2\pi}\,\sigma_{hit}} e^{-\frac{d^2}{2\sigma_{hit}^2}} \;+\; \frac{z_{rand}}{z_{max}} \;+\; z_{max}\cdot \mathbb{1}[\text{测距}= 最大量程]$$
其中 $d$ = 该束命中点在地图距离场里查到的"到最近障碍的距离"。三项的含义：

| 项 | 物理含义 | 你的参数 |
| --- | --- | --- |
| 高斯项 | "打在墙附近"——离墙越近越可信 | z_hit: 0.5，$\sigma_{hit}$ = sigma_hit: 0.2 |
| 随机项 | "任何地方都可能出杂点"的平坦兜底 | z_rand: 0.5 |
| 最大量程项 | "这束没打到东西"（打空）也有其合理性 | 雷达不出 inf，此项几乎不参与 |

为什么似然场比 beam 模型"宽容"：它只问"命中点离最近的墙多远"（查预计算的距离场，距离场上限 `laser_likelihood_max_dist: 2.0`m），不再逐点沿射线求交——所以对"地图过时"（墙的位置差了十几厘米）容忍度高，代价是会"误信"能穿透薄墙的读数。gmapping 教程第 8 章的"扫描匹配打分"用的是同族思想（打偏的束按离墙距离惩罚）——你现在已经把两个粒子滤波器的"眼睛"都学全了。
max_beams: 60：把 720 束均匀抽 60 束来打分——估计量的方差大一点，换 12 倍的算力，这个近似对粒子权重排序几乎无影响（工程上人尽皆知的"够用就好"）。

### 9.10 本章小结

- AMCL = "地图已知的粒子滤波"；运动用 OmniMotionModel（5 个 alpha，含横向噪声），观测用似然场（预计算距离场 + 高斯打分的经典组合）。
- 自适应粒子数（500~2000）与位移触发更新（0.25m/0.2rad）是它省算力的手段。
- 你关闭了绑架恢复（求稳），换来需要手动重定位的代价。
- 诊断工具：RViz 看粒子云松紧；ros2 topic echo /amcl_pose 看协方差。
- 代码对照：nav2_amcl/src/amcl_node.cpp（主逻辑）、src/motion_model/omni_motion_model.cpp、src/sensors/laser/likelihood_field_model.cpp、src/map/map_cspace.cpp（距离场）、src/pf/pf.c（滤波核心）。

---

## 第 10 章　地图服务：map_server 与地图文件

### 10.1 pgm + yaml：地图的"两张纸"

本仓库层/工程常识，`~/map/my_map.yaml` 是导航地图的"说明书"，`.pgm` 是像素图：
image: my_map.pgm        # 像素文件（灰度图：白=空闲 黑=障碍 灰=未知）
mode: trinary            # 三值模式
resolution: 0.05         # 每像素 0.05m
origin: [-10, -10, 0]    # 左下角像素在 map 系的坐标（x, y, yaw）
negate: 0                # 0=白是空闲；1=反过来
occupied_thresh: 0.65    # 像素值换算成占据概率的阈值
free_thresh: 0.25
从像素到概率的换算（`mode: trinary`）：
$$\text{occ} = \frac{255 - \text{pixel}}{255},\qquad
\begin{cases}
\text{occ} > 0.65 & \to 100 \text{（占据）}\\
\text{occ} < 0.25 & \to 0 \text{（空闲）}\\
\text{否则} & \to -1 \text{（未知）}
\end{cases}$$
**人话版：** 黑像素（0）→ occ=1 → 墙；白像素（255）→ occ=0 → 空地；浮点在中间的被压成未知。
有趣事实：gmapping 发的 `/map` 本来就是 0/100/-1 三值图（gmapping 教程第 7 章：`updateMap` 里直接二值化），所以存下来再加载几乎无损——地图的"信息密度"在建图那一刻就定死了。

### 10.2 map_server 与那个被修过的 QoS bug（真实案例教学）

map_server 把 yaml/pgm 读进内存后，以 Transient Local（latched） QoS 发布 `/map`：晚来的订阅者立刻能拿到最后一帧——这是"地图"这个数据天生该有的语义（地图不需要"等下一帧"）。
这就串起你项目里两个真实修过的坑（都发生在今天，可以当活教材）：

1. **gmapping 的 `/map` 原本是 volatile**——nav2 的 static_layer 和 map_saver_cli 都按 Transient Local 订阅，QoS 不匹配直接收不到任何地图。症状：`map_saver_cli` 报 `Failed to spin map subscription`；修法：gmapping 的 `/map` 发布器改用 transient_local（现在的新代码里已修）。
2. **`save_map.sh` 传 `save_map_timeout:=10000` 报参数类型错**——该参数是 double，脚本传整数直接退出。修法：`10000.0`。

两个坑的共同教训：ROS2 的"收不到"未必是没发，而可能是 QoS / 参数类型的"暗礁"。排查时先 `ros2 topic info /map -v` 看两端的 QoS。

### 10.3 保存地图的两条路

```bash
# ① 项目脚本（推荐；内部已处理好 QoS/参数细节）
bash ~/gmapping_ws/src/robot_navigation_ros2/script/save_map.sh            # → ~/map/my_map
bash ~/gmapping_ws/src/robot_navigation_ros2/script/save_map.sh office     # → ~/map/office

# ② 手动调用（等价）
ros2 run nav2_map_server map_saver_cli -f ~/map/my_map \
  --ros-args -p save_map_timeout:=10000.0 -p map_subscribe_transient_local:=true
```

前提都是建图节点正在运行（它订阅 `/map` 保存"当前这一瞬间"的快照）。

### 10.4 本章小结

- 地图 = pgm（像素）+ yaml（分辨率/origin/阈值），像素灰度↔占据概率有确定换算。
- map_server 用 Transient Local 发布；gmapping 修过的 QoS 与脚本修过的参数类型都是"地图能不能被用起来"的关键细节。

---

## 第 11 章　生命周期与启动编排（Nav2 的工程骨架）

### 11.1 为什么 Nav2 节点都是"生命周期节点"

nav2 的 server 都是 Lifecycle Node（生命周期节点）：比普通节点多一个状态机：
unconfigured ──configure──► inactive ──activate──► active
     ▲                          ▲                    │
     └──────cleanup─────────────┴─────deactivate─────┘
                              （shutdown → finalized）

- configure：读参数、建插件、订阅话题——"装弹上膛"；
- activate：开始真正干活（发布/行动）——"开火"；
- 好处：多节点系统可以"整齐划一地上电"——代价地图、规划器、控制器按顺序配好再一起激活，不会有半成品节点抢话题。参数只在 configure 时读——所以改参数要重启节点（或是让它重新 configure）。

### 11.2 lifecycle_manager：上电总指挥

nav2.launch.py 里最后那个 lifecycle_manager_navigation 拿着名单：
node_names: [controller_server, planner_server, behavior_server,
             bt_navigator, waypoint_follower, velocity_smoother]
它做的事：逐个 configure → 逐个 activate；之后起一个 bond 心跳（每秒"你还好吗"），任何节点挂了就尝试处理。启动这么"重"是有代价的——你看到 Nav2 启动要好几秒，主要花在这。
**排障第一课：** ros2 lifecycle get /controller_server 显示 active [3]才算就绪；卡在  unconfigured  = configure 阶段就失败了，去看该节点自己打印的 ERROR。

### 11.3 本项目三个启动文件的编排逻辑

**本仓库层：** 三个一键 launch 的"装配单"（都在 robot_navigation_ros2/launch/）：

| 层级 | `slam_nav.launch.py` | `nav_with_map.launch.py` | `slam_gmapping bringup.launch.py` |
| --- | --- | --- | --- |
| 硬件层 | robot_lidar（底盘、雷达、URDF） | 同左 | 底盘、雷达（无 URDF） |
| 定位层 | gmapping.launch.py | localization.launch.py（map_server + AMCL） | gmapping |
| 导航层 | nav2.launch.py | nav2.launch.py | 无 |
| 可视化 | rviz_nav（可选） | 同左 | 自带 RViz 配置（可选） |
| 用途 | 边走、边建图、边导航 | 有图导航 | 纯建图 |

关键设计：硬件层永远只有一份（robot_lidar.launch.py），定位层"二选一"插上——gmapping 与 AMCL 像两把钥匙，同一把锁只能插一把。

### 11.4 两个真实 bug 的复盘（本章最值钱的部分）

这两个坑都真实发生在你这台机器人上、已修复；它们的教训比任何书本知识都值钱。
案例 A：用 LifecycleNode 启动"普通节点"——静默挂起

- 现象：`base_control_ros2` 和 `ydlidar` 的 launch 用 `LifecycleNode(...)` 启动，但这两个可执行文件里是普通 Node。奇怪的是"能跑"，只是偶尔怪怪的。
- 原理：launch 的 `LifecycleNode` 动作启动后会去调用节点的 `change_state` 服务做 configure/activate；普通节点没有这个服务。launch_ros 的等待逻辑是 `wait_for_service` 无限等（只在 launch 退出时才放弃）——于是后台静默挂着一个永远等不到的线程，不报错、不影响主流程——但是个定时炸弹（例如 `ros2 lifecycle` 工具、或者将来加了严格依赖的编排都会出问题）。
- 教训：launch 文件的节点动作类型必须和节点实现匹配；"能跑"不等于"对"。
- 修法：改用普通 Node()（现在三个 launch 都已修正）。

案例 B：launch 参数同名冲突——Nav2 全体"裸奔"

- 现象：slam_nav 启动后 controller_server 报 Couldn't load critics! No critics defined for FollowPath，Nav2 全体卡在 unconfigured。
- 排查：`ps aux | grep controller_server` 看进程命令行——发现收到的竟然是 `--params-file .../S4.yaml`（雷达的参数文件）！再查 `/controller_server` 的参数：`FollowPath.vx_max` 是"not set"（yaml 没生效，加载了默认的 DWB 控制器，DWB 又因无 critics 报错）。
- 原理：launch 的"启动参数（LaunchConfiguration）"在整个 launch 上下文里是全局共享的。`slam_nav` 先包含雷达链（其中 `ydlidar.launch.py` 声明了 `params_file = S4.yaml`），后包含 `nav2.launch.py`（它声明 `params_file = nav2_params.yaml`）——后者的默认值被前者挡住了（同名参数已存在，新声明被忽略）。
- 教训：给 launch 参数起名要带"域"（`lidar_params_file` vs `nav2_params_file`）；大组合 launch 里，任何"通用名"都可能撞车。
- 修法：雷达侧改名为 lidar_params_file（现在已修正；cartographer_ws 里的旧拷贝还有这个雷）。

这两个案例合起来是一条通用工程观：集成系统的故障，大多不是算法错，而是"接线"错。学会看进程命令行、看 QoS、看生命周期状态，比背算法更能救命。

### 11.5 生命周期细节：每个阶段节点在做什么 + bond 心跳

① 状态转换对应节点里的代码（每个 Nav2 server 的"做与不做"清单）：

| 转换 | on_configure（装弹） | on_activate（开火） |
| --- | --- | --- |
| 干什么 | 读参数、创建插件（MPPI/NavFn/critic）、创建订阅发布对象（但不激活发布） | 激活 publishers、开启 action server、开定时器（控制循环跑起来） |
| 典型翻车 | 参数缺失/非法 → configure 失败（例如 No critics defined，第 11.4 节案例 B）；XML 树解析失败 → bt_navigator 卡住 | 激活时查 TF/代价地图，长时间等不到会超时（例如 map→odom 不存在，第 9.6 节） |

推论：参数只在 configure 读一次 → "改参数要重启节点"（gmapping 的参数同理，它启动时读一次）。Nav2 支持运行时 `ros2 param set` 改一部分参数，但大部分插件参数改了不生效，别依赖它。
② bond 心跳（`lifecycle_manager.cpp`，已核对）：manager 在 activate 每个节点后，与它建立一条 bond 连接——节点每秒发一次心跳，manager 用 `bond_timeout: 4.0`（秒，默认值）判断"它还活着吗"。超时 → manager 知道该节点崩了。和 velocity_smoother 的 1 秒失联刹车是同一族设计：Nav2 的每一层都假设"上面那层可能随时没了"，自己兜底。
③ 失败时的回滚：manager 的启动是"逐个 configure→逐个 activate"，任何一个失败 → 打印 `Failed to bring up all requested nodes` 并放弃激活（你见过的那个报错就是它）——已激活的节点保持 active，未激活的留在原地，等人工修复。这就是为什么"Nav2 起不来"时你要看第一个报 ERROR 的节点，而不是最后一个。

### 11.6 本章小结

- Nav2 的节点都是生命周期节点，靠 lifecycle_manager 统一 configure→activate；ros2 lifecycle get 是排障第一步。
- 三个一键 launch = 硬件层 + 定位层（二选一）+ 导航层 的装配单。
- 两个真实 bug（动作类型不匹配、launch 参数撞名）的教训：集成问题优先查"接线"。

---

## 第 12 章　完整数据流走查：一次导航任务的全生命周期

本章把前面 11 章全部串起来，用一次真实的"点目标→到达"走一遍每个环节。建议对照 RViz 和终端亲自跟一遍（这是附录 A 阶段 6 的"人肉单步"）。

### 12.1 时间线：从点下 "2D Goal Pose" 到停稳

时刻 T+0ms：你在 RViz 点目标 → `/goal_pose` 话题 → bt_navigator 转成 `NavigateToPose` action 目标，行为树开始跑。
T+10ms：树的 `ComputePathToPose` 节点 → 调 `planner_server` 的 action →

- 全局代价地图（1Hz 刷新，第 3 章）此刻的 master 网格；
- NavFn（A*）在其上搜索（第 4 章）→ 得到路点序列 /plan；
- 若失败，先清全局代价地图重试一次，再失败交给恢复分支。

T+40ms：树的 `FollowPath` 节点 → 调 `controller_server` 的 action，进入 20Hz 循环（每 50ms 一拍，第 6 章）：
每一拍（50ms）内 controller_server 做的事：
  ① 取机器人当前位姿（TF map→base_link）与当前速度
  ② 取全局路径 + 局部代价地图（5Hz 刷新，第 3 章）
  ③ MPPI: 在上一拍序列上撒 2000 条变体（noise_generator）
  ④ 每条 56 步前向积分（Omni 模型）→ 得到 2000 条 2.8 秒轨迹
  ⑤ 8 个 critics 打分（cost/避障/贴路/朝前...）→ 加权合成
  ⑥ softmax 加权平均 → 新控制序列（optimizer.cpp 356-388 行）
  ⑦ 限幅 → 取第一格 → 发 cmd_vel_nav
同时：`velocity_smoother`（20Hz）对 `cmd_vel_nav` 做加速度合规（第 8 章）→ 输出 `/cmd_vel` → base_control 打包成串口帧 → 底盘 MCU → 轮子转。
每 1 秒：树的 `RateController` 触发重规划——世界变了（新障碍、SLAM 地图长了、定位修正了），路径跟新（回到 T+10ms 那一步）。
异常时：FollowPath 失败 → 清局部代价地图重试 → 还不行 → 恢复分支（清图→转圈 90°→等 5s→后退 0.3m 轮着来，第 7 章）。
T+终点：goal checker 判"到位"（xy 误差 < 0.10m、朝向误差 < 0.10rad，第 6 章末）→ FollowPath SUCCESS → 树整体成功 → 速度归零（并因 `velocity_timeout` 兜底，即使链路断也不会跑飞）。

### 12.2 算力账（概念版）

- MPPI 一拍的分量：2000 轨迹 × 56 步 × (积分 + 8 个 critic 的向量化打分)。看着大，但都是 SIMD 友好的矩阵运算——RK3588 的一颗大核就能实时（你实测 gmapping+Nav2+驱动全开约 1.5~2 个核）。
- 真正的"贵"点在地图变化时的重建与控制周期内不可用的重规划——这也是为什么全局 1Hz、costmap 1~5Hz、MPPI 20Hz 的频率是"错峰设计"：慢的别拖快的。

### 12.3 slam_nav 模式才有的特殊剧情

无图导航时，地图和定位都在"生长"，会出现三种专属现象：

1. 新的地图区域触发重规划：机器人刚"看"到一条走廊，全局路径可能立刻改走捷径——正常现象；
2. 回环时的 map→odom 跳变：gmapping 发现"原来来过这里"，一次性把位姿修正几十厘米（第 2 章说过：校正本身就是跳变的）。表现：RViz 里车"啪"地跳到墙中间——局部代价地图不抖（它在 odom 系），全局路径会重算；
3. 历史轨迹上的"幽灵障碍"：以前建图时有人站的地方在地图上留了障碍，现在人走了但地图还记着——这是"地图过期"最典型的症状，有图导航模式要用新地图重存，无图模式等 gmapping 逐渐修正。

### 12.4 本章小结

- 一次导航 = 行为树指挥下的三种"心跳"协同：重规划 1Hz、代价地图 1~5Hz、MPPI 20Hz。
- 速度链路：MPPI → cmd_vel_nav → 平滑器 → /cmd_vel → 串口 → MCU，任何一环断了都有兜底。
- slam_nav 的特殊剧情（重规划、定位跳变、幽灵障碍）都是"两个估计器同时在线"的必然现象，不是 bug。

---

## 第 13 章　参数总表：逐段解读 nav2_params.yaml

本章是"字典"——不用背，但要知道每个旋钮在哪儿、管什么。文件位置：
~/gmapping_ws/src/robot_navigation_ros2/config/nav2_params.yaml

### 13.1 速度相关的"三处一致"（最容易调错的组）

你的速度上限写在三个地方，它们应保持一致（配置注释里也强调过）：

| 位置 | 参数 | 值 | 作用 |
| --- | --- | --- | --- |
| MPPI `FollowPath` | `vx_max / vx_min / vy_max / wz_max` | `0.30 / −0.20 / 0.05 / 1.5` | 采样与限幅时的速度边界 |
| velocity_smoother | `max_velocity / min_velocity` | `[0.30, 0.05, 1.5] / [−0.20, −0.05, −1.5]` | 最终下发前的硬限幅 |
| behavior_server | `max_rotational_vel` 等 | `1.0 / 0.4 / 3.2` | 恢复动作的限速 |

想开快：三处一起改大（并先在空旷处试验）。想单独让转向更猛只改 wz 组——但记得"三处一致"的原则。

### 13.2 bt_navigator 段

| 参数 | 值 | 含义 |
| --- | --- | --- |
| `global_frame / robot_base_frame` | `map / base_link` | 规划与控制的参考坐标系（第 2 章） |
| `odom_topic` | `/odom` | 速度反馈源 |
| `plugin_lib_names` | 多个插件库 | 注册行为树节点类型，例如 ComputePathToPose、Spin |
| `bt_loop_duration` | `10 ms` | 行为树 tick 周期 |
| `default_server_timeout` | `20 ms` | 等待 action server 的超时时间 |

### 13.3 controller_server（MPPI）——全文参数密度最高的段

controller_frequency: 20.0          # 控制循环 20Hz（每 50ms 一拍）
FollowPath:
  time_steps: 56 / model_dt: 0.05   # 预演 2.8 秒（第 6.2 节）
  batch_size: 2000                  # 每拍撒 2000 条
  vx_std/vy_std/wz_std: 0.20/0.05/0.40   # 噪声（第 6.3 节）
  temperature: 0.3                  # softmax 温度（第 6.6 节）
  motion_model: "Omni"              # 全向模型（第 6.4 节）
critics 8 个 + 各自权重（第 6.5 节表格）
progress_checker: 10 秒内必须动 0.5m   # 卡死检测
general_goal_checker: xy 0.10m / yaw 0.10rad   # 到点判据（第 12 章）
调参入口的分工：想改"走法性格"→ critics 权重 / 温度 / 噪声；想改"走到多准才算到"→ goal_checker；想改"算多细"→ batch_size / time_steps（越大越准越慢）。

### 13.4 planner_server 段

GridBased（NavFn）：tolerance: 0.5、use_astar: true、allow_unknown: true——第 4.4 节逐行讲过。

### 13.5 两张代价地图

| 对比项 | 全局代价地图 | 局部代价地图 |
| --- | --- | --- |
| 参考坐标系 | `map` | `odom` |
| 范围 | 整张地图 | 机器人周围 3 m × 3 m |
| 是否滚动 | 否 | 是，随机器人移动 |
| 图层 | static + obstacle + inflation | obstacle + inflation |
| 更新频率 | 1 Hz | 5 Hz |
| 主要消费者 | NavFn 全局规划器 | MPPI 局部控制器 |

障碍层参数（两张图相同）：obstacle_max_range: 6.0、raytrace_max_range: 8.0（第 3.4 节）；膨胀（第 3.5 节）：cost_scaling_factor: 3.0、inflation_radius: 0.40；形状：robot_radius: 0.18。

### 13.6 behavior_server / velocity_smoother

behavior：五个插件（第 8.1 节）＋ 限速；smoother：feedback: OPEN_LOOP、max_accel [2.5, 2.5, 3.2]、velocity_timeout: 1.0（第 8.2 节）。

### 13.7 AMCL / map_server（仅有图模式，第 9、10 章）

AMCL：OmniMotionModel、likelihood_field、粒子 500~2000、update 0.25m/0.2rad、恢复关闭、set_initial_pose: true。map_server：yaml_filename 由 launch 注入。

### 13.8 症状 → 证据 → 处理 矩阵（调参速查）

| 症状 | 先看什么证据 | 优先处理 |
| --- | --- | --- |
| 车走“画龙” | RViz 中 `/optimal_trajectory` 是否左右横跳 | 降低 `vx_std/wz_std`；提高 PathAlignCritic 权重；检查里程计漂移 |
| 不敢走窄道 | 局部代价地图中的高代价区 | 减小 `inflation_radius`；核对 `robot_radius` 是否偏大 |
| 贴墙剐蹭 | 局部代价地图边缘与 footprint | 增大 `inflation_radius`，并检查 CostCritic 配置 |
| 到点前突然刹车或绕圈 | goal checker 参数与行为树日志 | 适当放宽 `xy_goal_tolerance`；检查最后 0.5 m 的 critic 退出逻辑 |
| 原地抖动或嗡鸣 | `/cmd_vel` 的微小抖动 | 降低采样噪声；按需设置 `deadband_velocity` |
| 拐弯迟疑 | PathAngleCritic 相关日志与角速度限幅 | 降低 PathAngleCritic 权重，或适当放宽 `wz_max` |
| 出现“幽灵障碍” | 局部代价地图中存在现实里已经移走的障碍 | 等待 clearing 射线清除，或调用代价地图清除服务 |
| 恢复行为过于频繁 | 行为树日志中的 recovery 次数 | 先排查定位抖动和地图污染，不要只改恢复参数 |

### 13.9 新手调参流程（重要，别乱调）

1. 一次只改一个参数，写在本子上（改前值/改后值/现象）；
2. 先调物理直觉类的（速度、膨胀半径、到点容差），再动"性格类"（critics 权重、温度）；
3. 任何改动都要在 RViz 里全程盯着代价地图和轨迹可视化——盲调等于赌博；
4. 调完固化的配置，记得同步更新本教程 13.8 的笔记（或直接写在 yaml 注释里，你现有的注释风格就是好榜样）。

---

## 第 14 章　两种模式怎么选：与 gmapping 的协同

### 14.1 选型速查

| 场景 | 推荐 |
| --- | --- |
| 第一次进新场地 / 场地布局会变 | slam_nav（无图导航） |
| 固定路线反复跑（巡检、送物） | nav_with_map（有图导航） |
| 只想先看看建图效果 | slam_gmapping bringup.launch.py rviz:=true（纯建图） |
| 演示"自动寻路" | 任一；有图模式更稳、更少翻车 |

### 14.2 无图导航的三个"天生代价"（对应 gmapping 教程第 12 章）

1. 没有回环检测：gmapping 的"回环"是扫描匹配凑出来的局部修正，没有全局位姿图优化——大场地上拐几道弯回来，误差可能在几十厘米级；
2. 地图会"长胖"和过期：人走过的障碍留印（第 12.3 节）；地图窗口 `±10m` 之外自动扩图（gmapping 教程 11 章）；
3. 两个估计器同时在线：gmapping 的位姿估计 + AMCL 不在（slam 模式无 AMCL）——导航用的位姿就是 gmapping 的最佳粒子位姿，它的每一次校正跳变都会直接改变"我在哪"。

### 14.3 有图导航的前提与维护

- 地图质量决定上限：建图时越慢越细（0.2/0.2 的更新阈值），导航时越省心；
- 场地变化（家具挪动）后：轻则代价地图即时看到（动态层），重则要重存地图；
- AMCL 的粒子云（RViz `/particlecloud`）是你的"定位体温计"：长期又散又跳 → 地图和现实对不上了。

### 14.4 交接流程建议（工程习惯）

① 纯建图（bringup）绕场两周 → ② save_map.sh 存档 → ③ 关机
④ 把机器人摆回建图起点 → nav_with_map 启动 → ⑤ RViz 确认粒子云收敛
⑥ 先跑 1~2 个小目标验证行为 → ⑦ 再跑正式任务

---

## 第 15 章　调试工具箱与 FAQ

### 15.1 工具箱（按"怀疑链"排序：TF → 地图 → 代价 → 规划 → 控制 → 底盘）

```bash
# TF（一切的地基）
ros2 run tf2_ros tf2_echo map base_footprint     # 整链通不通
ros2 run tf2_ros tf2_echo map odom               # 定位在不在更新

# 地图与代价地图
ros2 topic hz /map                               # 有图模式应该稳定有帧（latched 也在）
ros2 topic echo /global_costmap/costmap --field info --once    # 尺寸/分辨率/原点

# 规划与控制
ros2 action send_goal /compute_path_to_pose nav2_msgs/action/ComputePathToPose \
  "{goal: {header: {frame_id: map}, pose: {position: {x: 1.0}, orientation: {w: 1.0}}}, use_start: false}"
                                                 # ★只规划不动的"空转测试"
ros2 topic echo /cmd_vel_nav --once              # MPPI 原始输出
ros2 topic hz /cmd_vel                           # 平滑后下发的频率

# 生命周期
ros2 lifecycle get /controller_server

# 可视化（RViz，Fixed Frame = map）
#   建议勾选显示：/local_costmap/costmap、/global_costmap/costmap、
#   /optimal_trajectory（MPPI visualize 已开）、/particlecloud（有图模式）、
#   /plan（Path，全局路径）
```

空转测试（`compute_path_to_pose`）是本文档推荐的第一诊断动作：只算路径、不发速度，机器人不动——能一次性验证 TF/代价地图/规划器整条链，且绝对安全。

### 15.2 FAQ

| 现象 | 排查顺序 |
| --- | --- |
| 车在原地不动，也没反应 | ① ros2 lifecycle get /controller_server 是否 active ② /cmd_vel 有没有数据 ③ 串口有没有被别的进程占用（pgrep -af base_control） |
| 启动时 Nav2 报 No critics defined | 参数文件没加载到（历史上是 launch 参数撞名的典型症状，第 11.4 节） |
| RViz 里车/地图消失、报 No transform | Fixed Frame 是否 map；TF 三件套（底盘/雷达/定位源）是否都在跑 |
| 规划一直失败（"无法规划"） | 目标点是否在障碍/未知区里；allow_unknown 的值；起点是否被障碍"包围"（代价地图脏了→清一下） |
| 定位粒子云突然散开 | 里程计异常（打滑/被抬）或环境变化；用 2D Pose Estimate 重定位 |
| 车"绕远路" | 看全局代价地图的膨胀是否把近路糊住了（第 13.8 节） |
| 恢复行为无限循环 | 看树日志 + 照片级复盘：定位抖动？地图幽灵障碍？把根因修掉，别只调恢复 |
| 一切正常但就是慢 | 速度三处配置（第 13.1 节）；MPPI batch_size 太大会拖慢一拍的时间预算 |
| 保存地图报 QoS/超时错 | 用项目脚本；先确认建图在跑、/map 有发布（第 10.2 节） |

### 15.3 录包复盘（进阶习惯）

```bash
# 边跑边录（只录关键话题，别贪多）
ros2 bag record /scan /odom /tf /tf_static /cmd_vel /map /amcl_pose -o ~/bags/run1
# 事后慢慢回放分析（不用真车、可以反复看）
ros2 bag play ~/bags/run1 --clock
```

技巧：改参数前后各录一段，用同样起点回放对比——这是把"玄学调参"变成"对照实验"的唯一办法。

---

## 附录 A　理论 → 源码对照阅读路线

源码根：/home/bingda/gmapping_ws/nav2_study/navigation2（1.1.16）。
建议按阶段读，每阶段末的"自问"能确认你真的读懂了。行号以 1.1.16 为准，改版后按函数名搜索。
阶段 0 · 先认识"入口与出口"（理论：第 12 章）

- nav2_bt_navigator/src/bt_navigator.cpp：action 目标进来 → 加载行为树 → 循环 tick。
- nav2_bt_navigator/behavior_trees/navigate_to_pose_w_replanning_and_recovery.xml：你正跑的这棵树（第 7.3 节原文）。
- 自问：树里 `planner_id`/`controller_id` 和 yaml 的哪个字段对上？

阶段 1 · 代价地图（理论：第 3 章）

- nav2_costmap_2d/src/layered_costmap.cpp：图层合成调度（master 层）。
- nav2_costmap_2d/plugins/obstacle_layer.cpp：raytraceFreespace()（616 行）——沿射线擦格子。
- `nav2_costmap_2d/plugins/inflation_layer.cpp`：`computeCost` 的调用处；**公式本尊在 `include/nav2_costmap_2d/inflation_layer.hpp` 154-161 行**（$252e^{-\kappa(d-r)}$）。
- nav2_costmap_2d/plugins/static_layer.cpp：/map → 代价，Transient Local 订阅。
- 自问：一个格子从"雷达回波"到"膨胀后的 237"，经过了哪几个文件？

阶段 2 · 全局规划（理论：第 4 章）

- nav2_navfn_planner/src/navfn_planner.cpp：插件入口、setCostmap(..., allow_unknown_)（241 行）。
- nav2_navfn_planner/src/navfn.cpp：calcNavFnAstar()（310 行）→ propNavFnAstar；代价翻译宏 COST_NEUTRAL 50 / COST_FACTOR 0.8（navfn.hpp 65-66 行）；启发函数相关注释在 452、527、650 行附近。
- 自问：为什么"减最小值"（cost normalization）在 NavFn 和 MPPI 里都出现？（数值稳定，同一个工程套路）

阶段 3 · MPPI 控制器（理论：第 5、6 章）——最推荐精读的目录

- nav2_mppi_controller/src/controller.cpp：控制循环（computeVelocityCommands 入口）、插件生命周期。
- `nav2_controller/src/controller_server.cpp` 460-530 行（`computeAndPublishVelocity`）：MPPI 外面的整条流水线（第 6.11 节逐行讲解）——位姿获取、进度检查、速度死区、failure_tolerance、feedback。
- nav2_mppi_controller/src/path_handler.cpp：路径变换 + 最近点 + prune_distance 剪枝（第 6.12 节）。
- nav2_mppi_controller/src/optimizer.cpp：
  - sampleControlSequence（撒噪声、bounded 截断）；
  - integrateStateVelocities（330-336 行，第 6.4 节公式）；
  - **`updateControlSequence`（356-388 行）——第 6.6 节三个公式的原文**（gamma 修正、减最小值、softmax 加权）；

- getControlFromSequenceAsTwist（390 行起）：取第 0/1 格发给外面。
- nav2_mppi_controller/src/critics/：8 位考官各自一个文件，先读 goal_critic.cpp、prefer_forward_critic.cpp（各 40 来行，最短最好懂），再读 cost_critic.cpp（最复杂）。
- nav2_mppi_controller/include/nav2_mppi_controller/motion_models.hpp：Omni/MotionModel 的维度定义。
- 自问：`threshold_to_consider` 在源码里是哪一行决定"闭麦"的？（`withinPositionGoalTolerance` 为真则 return）

阶段 4 · 行为树与行为（理论：第 7、8 章）

- nav2_behavior_tree/plugins/：树上每个 XML 节点背后的 C++ 动作（compute_path_to_pose_action 等）。
- nav2_behaviors/plugins/spin.cpp（最短的恢复行为，从它入手）。
- 自问：`RateController` 是怎么让"重规划"变成每秒一次的？

阶段 5 · 定位（理论：第 9 章）

- nav2_amcl/src/amcl_node.cpp：参数声明（106~430 行一大片）、激光更新节拍、TF 发布。
- nav2_amcl/src/motion_model/omni_motion_model.cpp：5 个 alpha 的用法（对照 gmapping 教程第 9 章）。
- nav2_amcl/src/sensors/laser/likelihood_field_model.cpp：似然场打分公式。
- nav2_amcl/src/map/map_cspace.cpp：距离场预计算。
- nav2_amcl/src/pf/pf.c：粒子滤波核心（KLD 采样在其中；对照 gmapping 教程第 3 章对照读）。
- 自问：AMCL 的 `/particlecloud` 里粒子多了/少了，分别可能是哪里在起作用？（KLD 上下限、更新触发）

阶段 6 · 全程连读（最推荐的"看懂全图"动作）
开 slam_nav，同时开三个终端窗口：bt_navigator.cpp ↔ navfn_planner.cpp ↔ optimizer.cpp。发一个 2 米远的目标，人肉单步：action 进树 → 规划出 /plan → 每 50ms optimizer 更新 → /cmd_vel 出去。再故意在车前放个箱子，看 CostCritic 如何在轨迹上"加钱"、车如何绕开。跟两轮，你会对"导航实时运行"产生肌肉记忆。

已知工程注意（与本项目相关的踩坑记录）

1. map→odom 的时间戳语义：gmapping 修过"tf_delay 被截断为 0"，Nav2 侧没有这个问题；但对照阅读时注意两边时间戳风格差异。
2. launch 参数撞名（第 11.4 节案例 B）：读别人的组合 launch 时，先 `grep LaunchConfiguration` 找出所有同名参数。
3. LifecycleNode vs Node（案例 A）：读 launch 文件时，先确认可执行文件里的节点类型。
4. /map 的 QoS：为什么必须是 transient_local——第 10.2 节。
5. **`threshold_to_consider` 是"退出距离"不是"进入距离"**——最容易理解反的参数，第 6.5 节。
6. 速度三处一致（第 13.1 节）：MPPI、smoother、behavior 各配一套，改速度要一起改。

---

## 附录 B　术语表

| 术语 | 英文 | 一句解释（章节） |
| --- | --- | --- |
| 代价地图 | Costmap | 0~255 的"路感"栅格（第 3 章） |
| 膨胀 | Inflation | 给障碍发泡做安全壳（第 3.5 节） |
| 滚动窗口 | Rolling Window | 跟着车走的局部地图（第 3.6 节） |
| A\* | A-star | 带启发函数的最优图搜索（第 4.2 节） |
| 导航函数 | NavFn | NavFn 规划器内部的"势场"（第 4.3 节） |
| 模型预测控制 | MPC | 预演未来、只走一步（第 5 章） |
| MPPI | Model Predictive Path Integral | 采样 + 软加权的 MPC（第 6 章） |
| 温度 | temperature | softmax 的"果断↔佛系"旋钮（第 6.6 节） |
| 采样噪声 | vx_std/vy_std/wz_std | 撒轨迹时的"探索胆量"（第 6.3 节） |
| 行为树 | Behavior Tree | 流程即数据的调度方式（第 7 章） |
| 恢复行为 | Recovery Behavior | 自救动作库（第 8.1 节） |
| 速度平滑 | Velocity Smoothing | 加速度合规的最后一道闸（第 8.2 节） |
| AMCL | Adaptive Monte Carlo Localization | 自适应粒子滤波定位（第 9 章） |
| 似然场 | Likelihood Field | AMCL 的观测模型：离墙越近越像（第 9.3 节） |
| KLD 采样 | KLD Sampling | 自适应粒子数的依据（第 9.4 节） |
| 瞬态本地 | Transient Local (latched) | 晚订阅者也能拿到最后一帧的 QoS（第 10.2 节） |

生命周期
Lifecycle
configure/activate 状态机（第 11 章）

## 附录 C　公式速查表与易混概念对照

### C.1 核心公式速查

| 公式 | 出处 | 一句话 |
| --- | --- | --- |
| $c(d)=252e^{-\kappa(d-r_{ins})}$ | 膨胀层 | 离障碍越远代价指数下降（第 3.5 节） |
| $f(n)=g(n)+h(n)$ | A\* | 已走代价 + 剩余估计（第 4.2 节） |
| $50+0.8\times cost$ | NavFn | 代价地图 → 势场的翻译（第 4.3 节） |
| $\Delta x=(v_x\cos\theta-v_y\sin\theta)\Delta t$ | 运动模型 | 车身速度→世界位移（第 6.4 节） |
| $w_i\propto e^{-C_i/\lambda}$ | MPPI | 指数加权的"偏爱程度"（第 6.6 节） |
| $u_{new}=\sum_i w_i u_i$ | MPPI | 软加权平均出新速度（第 6.6 节） |
| $p(z\mid x,m)\propto z_{hit}e^{-d^2/2\sigma^2}+\dots$ | AMCL | 似然场打分（第 9.3 节） |
| $\text{occ}=(255-pixel)/255$ | 地图文件 | 像素灰度→占据概率（第 10.1 节） |

### C.2 易混概念对照

| 容易混淆的概念 | 前者 | 后者 |
| --- | --- | --- |
| 地图 vs 代价地图 | 地图描述占据状态（0/100/−1） | 代价地图描述通行代价（0~255） |
| 全局规划 vs 局部控制 | 决定大方向与全局路径 | 决定当前时刻怎么走 |
| `map` vs `odom` | 全局一致但校正时可能跳变 | 局部连续但会累积漂移 |
| gmapping vs AMCL | 同时定位和建图 | 在已有地图中定位 |
| 254 vs 253 | 障碍本体 | 车身内切圆会与障碍相撞的区域 |
| Action vs Topic | 有目标、反馈和结果，适合任务 | 连续发布数据，适合传感器与控制流 |

### C.3 记忆版总流程

"图 → 路 → 速 → 命"四步走：
代价地图把世界变成"路感"（图）→ NavFn 在图上找路（路）→ MPPI 把路磨成每一刻的速度（速）→ 行为树管整个命运流程与意外（命）→ 最后平滑器合规放行。

## 附录 D　分层自测题与参考答案

先自己在纸上答，再对照参考答案（折叠在每题的"答案"里——电子版直接看即可）。

### D.1 为什么要分全局/局部两层，而不是一个算法一口气算完？

答：全局用低频率（1Hz）在整张大图上找可靠大方向；局部用高频率（20Hz）对"眼前几米 + 机器人运动能力"做实时反应。合在一起算，要么慢到无法实时，要么只能看局部而丢了全局最优。

### D.2 代价地图上 253 和 254 分别是什么？253 和 252 的区别在工程上意味着什么？

答：254 = 致命障碍（绝对不可通行）；253 = 内切圆膨胀区（车身核心会碰，也不可通行）。253 与 252 的区别：253 是"死线"，252 往下是"要付费但可通行"的梯度——这差异就是"绕远但安全"这类决策的空间。

### D.3 膨胀公式里 $\kappa$ 调大，机器人的行为会怎么变？代价是什么？

答：$\kappa$（cost_scaling_factor）越大，代价随距离下降越快——"只有离障碍很近才贵"，规划器变得更"敢贴墙"；调小则相反（更保守远离）。代价：太小会不敢进窄道/门；太大容易贴墙剐蹭。

### D.4 MPPI 为什么用软加权平均而不是直接取最优轨迹？

答：避免抖动（最优在相近轨迹间跳）、允许"优点互补"、保持探索多样性；且温度连续可调（$\lambda\to0$ 即退化为取最优），给调参留了旋钮。

### D.5 你的 `temperature: 0.3` 说明控制性格偏"果断"还是"佛系"？给出量的依据。

答：偏果断。代价值每差 1，权重差 $e^{1/0.3}\approx28$ 倍——差一点点就"没得商量"，接近取最优。

### D.6 `PreferForwardCritic` 罚的是什么运动？权重 15 的动机？最后 0.5m 为什么放开？

答：罚的是倒车（max(-vx,0)），不是横移。权重 15 是为了"车头朝前、少非纵向机动"——压住麦轮侧向漂移（它直接污染 gmapping 的 map→odom 校正量）。最后 0.5m 放开（threshold_to_consider 语义）是为了允许精细的倒车/侧移收尾。

### D.7 AMCL 和 gmapping 都用了粒子，为什么 AMCL 通常能少用很多粒子？

答：AMCL 地图已知——粒子只表示位姿（3 维），不背着各自的地图；gmapping 每个粒子 = 一条轨迹 + 一张地图（地图维度是"往粒子背后挂"的）。省掉了地图的采样维度，同样精度需要的粒子自然少。

### D.8 某天有图导航定位粒子云突然大面积散开，写出你的前三个排查动作。

答：① 看 /odom 是否异常（打滑/被抬起/串口丢帧）；② 看环境是否变大（家具挪动、大面积反光/遮挡）；③ tf2_echo map odom 看 AMCL 输出是否在跳——必要时用 2D Pose Estimate 重新初始化，仍不行则检查地图与现场是否已经不一致。

### D.9（综合）把第 12 章的时间线压缩成五句话。

答：① 目标进树；② 1Hz 规划出全局路径；③ 20Hz MPPI 采样 2000 条 2.8 秒轨迹、8 个 critic 打分、softmax 加权出速度；④ 平滑器做加速度合规后下发给底盘；⑤ 失败走两级恢复，1 秒失联自动停车。

---

## 附录 E　项目算法覆盖对照表（"全覆盖了吗"的答卷）

本附录把机器人上实际运行的每一处算法/机制逐一列出，对照到本教程章节与源码文件。你可以拿它当"检查清单"：任何一行看不懂，回到对应章节。

### E.1 实际启用的算法/机制（全部有对应章节 ✓）

| # | 算法 / 机制 | 所在节点或插件 | 教程章节 | 源码/配置位置（关键） |
| --- | --- | --- | --- | --- |
| 1 | 航位推算里程计（速度积分 + 保护） | base_control_ros2 | 2.5 | base_control_ros2.py 的 odom_timerCB |
| 2 | 串口协议（帧/CRC/环形缓冲）+ 速度设定值下发（与 MCU 的分工） | base_control_ros2 ↔ 底盘 MCU | 2.5 | cmd_velCB/_parse_frame/crc_byte（工程实现，非导航理论） |
| 3 | TF 树组织与时间戳纪律 | base_control / gmapping / amcl | 2.2、2.3、2.5 | 各节点 TF 广播代码 |
| 4 | URDF → 静态 TF（robot_state_publisher） | nanoomni_description | 2.6 | urdf/nanoomni_description.urdf |
| 5 | 雷达驱动（无算法：回波 → LaserScan/PointCloud 的转换与发布） | ydlidar_ros2_driver | 12.1（数据流入口）、附录 F | ydlidar_ros2_driver_node.cpp |
| 6 | 分层代价地图（master 合成 / 合并规则 / 滚动窗口 / 车底清障） | costmap_2d | 3.3、3.6、3.7 | layered_costmap.cpp、costmap_2d.cpp:330 |
| 7 | 障碍层：marking / clearing（raytrace）/ 幽灵障碍机制 | obstacle_layer | 3.4、3.8 | plugins/obstacle_layer.cpp:616 |
| 8 | 膨胀层：指数衰减代价 $252e^{-\kappa(d-r_{ins})}$ | inflation_layer | 3.5 | inflation_layer.hpp:154-161 |
| 9 | 静态层：地图投影 + Transient Local 订阅 | static_layer | 3.3、10.2 | plugins/static_layer.cpp |
| 10 | NavFn：代价→势场翻译（50+0.8c）/ 优先级桶 / A*（二次近似+欧氏启发）/ 起点清障 / 路径提取 | nav2_navfn_planner | 4.3、4.4、4.7 | navfn.cpp、navfn_planner.cpp:231,411 |
| 11 | allow_unknown 未知区通行策略 | nav2_navfn_planner | 4.5 | navfn_planner.cpp:241 |
| 12 | MPPI：采样（噪声）→ 前向积分 → critics 打分 → softmax 加权 → 限幅 → 滚动 | nav2_mppi_controller | 6.2–6.8、6.13 | optimizer.cpp:356-388（核心更新） |
| 13 | MPPI 运动模型（Omni，含 vy 维） | 同上 | 6.4 | include/.../motion_models.hpp |
| 14 | 噪声生成（vx/vy/wz_std，复用策略） | 同上 | 6.3 | src/noise_generator.cpp |
| 15 | 8 个 critics（各自代价项与权重） | 同上 | 6.5 | src/critics/*.cpp（每 critic 一个文件） |
| 16 | controller_server 流水线：位姿获取 / 进度检查 / 速度死区 / failure_tolerance / feedback | nav2_controller | 6.11 | controller_server.cpp:460-530 |
| 17 | PathHandler：路径变换 / 最近点 / 剪枝 | nav2_mppi_controller | 6.12 | src/path_handler.cpp |
| 18 | 目标判据：SimpleGoalChecker（stateful）+ 到点容差 | nav2_controller | 6.11 | plugins/simple_goal_checker.cpp |
| 19 | 进度卡死检测（0.5m / 10s） | nav2_controller | 6.11 | plugins/simple_progress_checker.hpp |
| 20 | 行为树调度（tick / 黑板 / 三种 Sequence / Recovery / RateController / RoundRobin） | bt_navigator | 7.2–7.6 | behavior_trees/*.xml、nav2_behavior_tree/plugins/ |
| 21 | NavigateToPose action 接口与 RViz GoalTool | bt_navigator + rviz 插件 | 7.4 | nav2_msgs/action/NavigateToPose |
| 22 | 恢复行为 ×5（含旋转扫掠碰撞预测） | nav2_behaviors | 8.1、8.4 | plugins/spin.cpp 等 |
| 23 | 速度平滑：限幅 / 加速度约束（accel-decel 分支）/ 超时刹车 / 死区 | velocity_smoother | 8.2、8.5 | velocity_smoother.cpp:225,286 |
| 24 | AMCL：粒子滤波 + Omni 运动模型（5 alpha） | nav2_amcl | 9.1、9.2、9.8 | motion_model/omni_motion_model.cpp、pf/pf.c |
| 25 | AMCL 似然场观测模型（距离场预计算 + 三项公式） | nav2_amcl | 9.3、9.9 | sensors/laser/likelihood_field_model.cpp、map/map_cspace.cpp |
| 26 | KLD 自适应粒子数（500~2000） | nav2_amcl | 9.4、9.8 | pf/pf.c（pf_update_resample 内） |
| 27 | 地图文件格式（pgm/yaml → 占据概率换算）与加载 | nav2_map_server | 10.1 | nav2_map_server |
| 28 | QoS 匹配（/map 的 Transient Local；传感器流 Best Effort） | 全线 | 10.2 | 各发布/订阅声明 |
| 29 | 生命周期状态机（configure/activate）与 bond 心跳（4s） | nav2_lifecycle_manager + 各 server | 11.1、11.2、11.5 | lifecycle_manager.cpp:44,182 |
| 30 | 两级恢复与代价地图清除服务 | 行为树 + costmap 服务 | 7.3、3.8 | 默认树 XML |
| 31 | gmapping RBPF（导航的上游供图/定位） | slam_gmapping | 姊妹篇 + 1.3、12.3、14 章 | 见《Gmapping_SLAM 理论学习教程》 |

### E.2 项目中存在但未启用（故本教程未展开）——防"以为漏了"

| 组件 | 状态 | 如需启用，应重点补充的内容 |
| --- | --- | --- |
| Smac Planner | 未启用 | 运动学约束、Hybrid-A* / State Lattice 与启发函数 |
| DWB / RPP / TEB | 未启用 | 各自的轨迹生成、打分和适用底盘 |
| collision_monitor | 未启用 | 传感器安全区、减速区和急停链路 |
| smoother_server | 未启用 | 全局路径平滑与可行性约束 |
| waypoint_follower | 已配置但当前基本未使用 | 多航点任务编排与失败策略 |

### E.3 一句话结论

项目实际启用的算法与机制（E.1 全部 31 项）在本文中均有对应章节和源码对照；唯一属于"理论上该懂但不在本项目里"的部分（Smac/TEB/RPP/DWB/碰撞监控等）列在 E.2 中并给了定位与升级入口。若机器人日后再引入新组件（例如换 Smac、加碰撞监控），请按同样格式给本附录补一行——这份对照表就是这门课的"教学大纲与进度条"。

## 附录 F　接口速查：话题 / action / 服务 / QoS

### F.1 各节点的核心接口（本项目启动后实际存在的）

| 节点 | 订阅 | 发布 | Action（服务端） | 关键服务 |
| --- | --- | --- | --- | --- |
| base_control_ros2 | /cmd_vel | /odom、/imu、/battery、TF odom→base_footprint | — | — |
| ydlidar 驱动 | — | /scan、/point_cloud、TF(静态) base_link→base_laser_link | — | start_scan/stop_scan |
| slam_gmapping | /scan、TF | /map、/map_metadata、/entropy、TF map→odom | — | — |
| amcl（有图模式） | /scan、/initialpose、/map | /amcl_pose、/particlecloud、TF map→odom | — | — |
| map_server（有图模式） | — | /map(latched) | — | load_map |
| bt_navigator | （TF；无目标话题*） | 树状态日志 | /navigate_to_pose、/navigate_through_poses | — |
| planner_server | /map、/global_costmap/* | /plan、/global_costmap/*(内部) | /compute_path_to_pose、/compute_path_through_poses | clear 服务见下 |
| controller_server | /odom、TF、/local_costmap/* | /cmd_vel_nav（remap）、/optimal_trajectory、/trajectories（visualize） | /follow_path | — |
| behavior_server | TF、代价地图 | /cmd_vel_nav（remap） | /spin、/backup、/drive_on_heading、/wait、/assisted_teleop | — |
| velocity_smoother | /cmd_vel_nav、/odom | /cmd_vel（remap） | — | — |
| lifecycle_manager_navigation | — | — | — | /lifecycle_manager_navigation/manage_nodes、/is_active |
| （代价地图清除） | — | — | — | /local_costmap/clear_entirely_local_costmap、/global_costmap/clear_entirely_global_costmap（类型 nav2_msgs/srv/ClearEntireCostmap） |

\* 第 7.4 节的坑：**没有节点订阅 `/goal_pose`**——目标必须走 action（或用已修好的 `nav2_rviz_plugins/GoalTool`）。

### F.2 关键 QoS（"收不到数据"时先查这里）

| 数据流 | 建议 QoS | 关键原因 |
| --- | --- | --- |
| `/map` | Reliable + Transient Local | 地图低频且必须完整；晚启动的订阅者也要拿到最后一帧 |
| `/tf_static` | Transient Local | 静态变换发布一次后，晚加入节点仍需获取 |
| `/scan` | Sensor Data（通常 Best Effort + Volatile） | 高频数据优先实时性，偶发丢帧可接受 |
| `/tf`、`/odom` | Volatile | 只关心持续到来的最新状态 |
| Action / Service | Reliable | 任务目标、结果和请求不可随意丢失 |

### F.3 常用消息的关键字段（打开`ros2 topic echo` 时的对照卡）

| 消息 | 关键字段 | 看什么 |
| --- | --- | --- |
| nav_msgs/Odometry | pose.pose（位姿）、twist.twist（速度）、两个协方差 | 速度是否死区归零、位姿是否连续 |
| sensor_msgs/LaserScan | angle_min/max/increment、ranges[]、range_min/max | 帧内的距离数组；本车 720 束/圈 |
| nav_msgs/OccupancyGrid | info（分辨率/宽高/origin）、data[] | 0/100/-1 三值图 |
| nav_msgs/Costmap（raw） / OccupancyGrid | 0~255 代价 | 第 3.2 节的语义表 |
| nav_msgs/Path | poses[] | 全局路径（/plan） |
| nav2_msgs/action/NavigateToPose | goal=位姿；feedback=剩余距离/速度/恢复次数 | 导航进度 |
| sensor_msgs/BatteryState | voltage、current | 本车约 12V（3S） |

---

## 下一步与参考资料

- 立刻动手：按附录 A 阶段 0→3 精读一遍（重点 `optimizer.cpp:356-388`）；结合配套实验做“空转测试”。
- 动手实验：把关键实验拆成七天推进；每次实验都录包（第 15.3 节）。
- 进阶读物（按顺序）：
  1. Nav2 官方文档 <https://docs.nav2.org>（概念章节与 tuning 指南是一手权威）。
  2. G. Williams et al., Information-Theoretic Model Predictive Control: Theory and Applications to Autonomous Driving, IEEE T-RO 2018（MPPI 原文；公式附录与本文第 6.6 节直接对应）。
  3. S. Thrun et al.,《概率机器人 Probabilistic Robotics》：第 6 章（AMCL 似然场）、第 8 章（栅格规划）。
  4. S. Macenski et al., The Marathon 2: A Navigation System, IROS 2020（Nav2 架构论文）。

- 中文二手资料遇到公式时，一律回到上面 1、2、3 核对。

记住你最需要的三句话：
① 代价地图是"路感"：把"能不能走"变成"多贵"，一切决策才有权衡空间。
② 全局找大方向、局部管眼前，MPPI 是"撒点 + 软加权"的实时预演。
③ 调参先调物理直觉（速度、膨胀、容差），再动性格旋钮（critics 权重、温度）；一次一个，全程看 RViz。

祝导航顺利！学完这篇再回头看你那份 nav2_params.yaml，每个数字都会"活"过来。

---
