# JXB 五自由度机械臂使用教程

本教程面向第一次接触本项目的同学，介绍如何运行机械臂仿真、使用网页控制真实机械臂，以及如何进行 PPO 强化学习训练。

> 建议第一次使用时只运行 PyBullet 仿真。确认模型、滑条和坐标控制都能正常工作后，再连接真实机械臂。

## 1. 项目能做什么

本项目包含三部分：

1. **ESP32 机械臂固件**
   - 使用 PCA9685 驱动 5 路舵机。
   - 提供网页控制面板。
   - 提供 HTTP 接口和 UDP 控制接口。

2. **PyBullet 机械臂仿真**
   - 加载项目中的 URDF 和 STL 模型。
   - 支持 5 路舵机角度滑条。
   - 支持末端正运动学、逆运动学和碰撞检测。
   - 可以通过 UDP 将仿真角度发送给真实机械臂。

3. **PPO 强化学习**
   - 计算机械臂可达空间。
   - 生成训练目标点。
   - 训练机械臂到达固定目标。
   - 保存模型、训练指标和训练曲线。

## 2. 使用路线

同学可以根据需求选择一条路线：

| 需求 | 推荐路线 | 是否需要实物 |
| --- | --- | --- |
| 只想快速查看机械臂模型 | 运行打包好的 `JXB_Pybullet.exe` | 否 |
| 想修改 PyBullet 程序 | 安装 Python 依赖后运行源码 | 否 |
| 想用浏览器控制机械臂 | 烧录 ESP32 固件并打开控制网页 | 是 |
| 想让仿真同步控制机械臂 | ESP32 固件 + PyBullet UDP 控制 | 是 |
| 想学习强化学习 | 运行 `jxb_study/scripts` 中的脚本 | 否 |

## 3. 安全须知

真实机械臂上电前，请先完成以下检查：

- 第一次测试时将机械臂架空，避免机械臂碰撞桌面或夹伤手指。
- 舵机电源应使用合适的独立电源，不要直接让开发板的 3.3 V 引脚给舵机供电。
- ESP32、PCA9685 和舵机电源必须共地。
- 调整角度时先小幅移动，不要直接测试极限角度。
- 点击网页中的 `Stop` 只会停止继续运动，不会切断舵机电源。
- 出现持续抖动、异响、堵转或过热时，应立即断开舵机电源。
- 实物联动前必须确认仿真角度与真实舵机角度的方向和零位一致。

## 4. 项目目录

```text
JXB/
├─ JXB.ino                         ESP32 主程序
├─ WifiControl.h                   WiFi 控制接口声明
├─ WifiControl.cpp                 网页、HTTP 和 UDP 控制实现
├─ Pybullet/
│  ├─ Pybullet.py                  PyBullet 仿真与实物联动入口
│  ├─ dist/JXB_Pybullet.exe        Windows 免安装版仿真程序
│  └─ robot_arm2.SLDASM/           URDF、STL 和 ROS 模型文件
└─ jxb_study/
   ├─ outputs/                     可达点云、目标点和结果图片
   └─ scripts/
      ├─ reachable_workspace.py    计算机械臂可达空间
      ├─ target_sampler.py         从可达空间生成目标点
      ├─ random_env_test.py        随机动作环境测试
      ├─ parallel_env_test.py      并行环境测试
      ├─ debug_reward_display.py   奖励函数可视化调试
      ├─ train_fixed_target_ppo.py PPO 训练入口
      ├─ play_fixed_target_policy.py 训练模型演示
      └─ arm_ppo_final.pt          已训练模型
```

## 5. 快速运行 PyBullet 仿真

### 5.1 使用打包好的 Windows 程序

此方式不需要安装 Python。

在项目根目录打开 PowerShell，然后运行：

```powershell
.\Pybullet\dist\JXB_Pybullet.exe
```

程序启动后会出现 PyBullet 窗口。右侧调试面板包含以下控件：

| 控件 | 作用 |
| --- | --- |
| `IK enable 0/1` | `1` 使用逆运动学，`0` 使用舵机角度滑条 |
| `IK target X/Y/Z` | 调整红色目标点的位置 |
| 舵机滑条 | IK 关闭时，分别调整舵机角度 |
| `areset` | 单击后恢复初始姿态 |

默认情况下 IK 已开启。调整 `IK target X`、`IK target Y` 和 `IK target Z`，机械臂末端会尝试跟随红色目标点。

如果想手动调整每个舵机：

1. 将 `IK enable 0/1` 调为 `0`。
2. 缓慢拖动舵机滑条。
3. 观察机械臂姿态和终端中的碰撞提示。

程序会进行自碰撞检测。检测到新的碰撞时，如果已连接真实机械臂，当前角度不会继续通过 WiFi 发送。

### 5.2 常见启动问题

如果双击程序后窗口立即关闭，应在 PowerShell 中运行程序，以便查看错误信息。

如果 Windows 安全软件拦截打包程序，建议改用本教程第 8 节的 Python 源码方式运行，不要直接关闭系统的全部安全防护。

## 6. ESP32 与 PCA9685 硬件准备

### 6.1 所需硬件

- ESP32 开发板
- PCA9685 16 路舵机驱动板
- 5 个舵机
- 与舵机匹配的独立电源
- USB 数据线
- 杜邦线

### 6.2 基本接线

| PCA9685 引脚 | 连接位置 |
| --- | --- |
| `VCC` | ESP32 逻辑电源，通常为 3.3 V |
| `GND` | ESP32 GND，并与舵机电源 GND 共地 |
| `SDA` | ESP32 默认 I²C SDA |
| `SCL` | ESP32 默认 I²C SCL |
| `V+` | 舵机独立电源正极 |
| 舵机通道 `0` 至 `4` | 机械臂的 5 路舵机 |

当前程序使用：

```cpp
Wire.begin();
Adafruit_PWMServoDriver pwm(0x40);
```

因此程序使用开发板默认的 I²C 引脚，PCA9685 地址为 `0x40`。如果所用 ESP32 开发板没有采用默认 I²C 引脚，可以把代码改成：

```cpp
Wire.begin(SDA_PIN, SCL_PIN);
```

其中 `SDA_PIN` 和 `SCL_PIN` 应替换为实际接线引脚。

> 不同开发板的默认 I²C 引脚可能不同。接线前应查看开发板引脚图，不要只根据其他型号的接线照搬。

## 7. 烧录 ESP32 固件

### 7.1 安装 Arduino 开发环境

需要安装：

- Arduino IDE
- Arduino IDE 中的 ESP32 开发板支持包
- `Adafruit PWM Servo Driver Library`

`Wire` 和 `WiFi` 随 ESP32 Arduino 环境提供，不需要单独复制到项目中。

### 7.2 打开项目

使用 Arduino IDE 打开项目根目录中的 `JXB.ino`。请保持以下三个文件在同一个 `JXB` 文件夹中：

```text
JXB.ino
WifiControl.h
WifiControl.cpp
```

### 7.3 修改 WiFi

在 `JXB.ino` 中找到：

```cpp
const char* WIFI_SSID = "REDACTED_CONFIGURE_LOCALLY";
const char* WIFI_PASSWORD = "REDACTED_CONFIGURE_LOCALLY";
```

改成现场可用的 2.4 GHz WiFi 名称和密码。电脑与 ESP32 必须连接到同一个局域网。

当前固件会一直等待 WiFi 连接。如果名称、密码或信号不正确，程序会一直停留在连接阶段，网页和 UDP 服务都不会启动。

### 7.4 检查初始角度

当前固件中的初始角度为：

```cpp
int currentAngle[count] = {58, 0, 180, 130, 90};
int targetAngle[count]  = {58, 0, 180, 130, 90};
```

上电后，5 路舵机会依次移动到这些角度。首次测试前应：

1. 断开舵机与机械臂连杆，或将机械臂架空。
2. 确认每个舵机的安装方向。
3. 根据实际机械零位修改初始角度。
4. 确认无碰撞后再安装和联调连杆。

### 7.5 编译与上传

1. 在 Arduino IDE 中选择实际使用的 ESP32 开发板。
2. 选择正确的串口。
3. 点击“验证/编译”。
4. 编译通过后点击“上传”。
5. 打开串口监视器，将波特率设为 `115200`。

连接成功后，串口会输出类似内容：

```text
WiFi connected
Control page: http://192.168.1.100
ESP32 UDP target: 192.168.1.100:8888
PCA9685 JXB ready
```

每次路由器重新分配地址后，ESP32 的 IP 都可能变化，应以串口本次打印的地址为准。

## 8. 使用网页控制真实机械臂

在与 ESP32 相同局域网的电脑或手机浏览器中输入：

```text
http://ESP32的IP地址/
```

例如：

```text
http://192.168.1.100/
```

网页包含 5 个舵机滑条和 4 个操作按钮：

| 操作 | 作用 |
| --- | --- |
| `Servo 0` 至 `Servo 4` | 设置对应舵机目标角度 |
| `reset` | 移动到预设位置 A |
| `Position B` | 移动到预设位置 B |
| `Run` | 允许舵机继续向目标角度运动 |
| `Stop` | 将目标锁定为当前角度并停止继续运动 |

程序每 10 ms 更新一次控制循环，每次最多变化 `1°`，因此舵机会平滑移动到目标角度。

预设位置在 `JXB.ino` 中定义：

```cpp
const int positionA[count] = {90, 0, 180, 130, 90};
const int positionB[count] = {120, 30, 90, 90, 90};
```

如果预设动作不适合实际机械结构，应先修改这两组角度再使用。

## 9. 通过 HTTP 接口控制

除了网页，也可以直接访问 HTTP 接口。

假设 ESP32 地址为 `192.168.1.100`：

### 9.1 设置单个舵机

```text
http://192.168.1.100/set?ch=0&angle=90
```

- `ch` 范围为 `0` 至 `4`。
- `angle` 会被限制在 `0°` 至 `180°`。

### 9.2 同时设置多路舵机

```text
http://192.168.1.100/move?s0=90&s1=0&s2=180&s3=130&s4=90
```

### 9.3 使用预设

```text
http://192.168.1.100/preset?name=A
http://192.168.1.100/preset?name=B
```

### 9.4 停止、继续和查询

```text
http://192.168.1.100/stop
http://192.168.1.100/run
http://192.168.1.100/status
```

`/status` 返回示例：

```json
{
  "current": [58, 0, 180, 130, 90],
  "target": [58, 0, 180, 130, 90],
  "stop": false,
  "udpActive": false,
  "lastUdpMs": 0
}
```

## 10. 使用 PyBullet 控制真实机械臂

### 10.1 联动前校准

当前代码存在一项必须注意的配置差异：

- ESP32 固件的第 1 路初始角度是 `58°`。
- `Pybullet/Pybullet.py` 中 `SERVO_INIT_DEG` 的第 1 路初始角度是 `90°`。

如果直接联动，真实机械臂与仿真的初始姿态可能不一致。应根据实际机械零位，将固件的初始角度和 `SERVO_INIT_DEG` 校准为同一套定义，同时检查：

```python
SERVO_DIRECTION = [1, 1, 1, 1, 1]
```

如果某个仿真关节与真实舵机方向相反，应仅将对应位置改为 `-1`，并重新进行小角度测试。

### 10.2 启动联动

先从串口监视器获得 ESP32 的 IP，然后在项目根目录运行：

```powershell
.\Pybullet\dist\JXB_Pybullet.exe --robot-ip 192.168.1.100
```

如果 UDP 端口没有修改，不需要额外参数。自定义端口的写法为：

```powershell
.\Pybullet\dist\JXB_Pybullet.exe --robot-ip 192.168.1.100 --udp-port 8888
```

PyBullet 会以最高约 20 Hz 的频率发送：

```text
movej:90,0,180,130,90
```

ESP32 在 UDP 数据超过 1.5 秒未更新时会自动停止，并把目标角度锁定为当前角度。

### 10.3 UDP 支持的命令

固件监听 UDP `8888` 端口，支持：

```text
ping
heartbeat
stop
run
movej:90,0,180,130,90
{"cmd":"movej","joints":[90,0,180,130,90]}
```

重要说明：

- 电脑和 ESP32 必须处于同一局域网。
- Windows 防火墙可能拦截 UDP。
- 不要同时让多个程序向机械臂发送控制命令。
- 一旦使用过 UDP 控制，会话超时保护保持有效。准备改回网页控制时，建议先关闭 PyBullet 并重启 ESP32。
- 关闭 PyBullet 窗口后，UDP 数据停止，ESP32 会在约 1.5 秒后停止继续运动。

## 11. 使用 Python 源码运行仿真

如果要阅读或修改代码，应使用源码方式。

### 11.1 创建虚拟环境

在项目根目录执行：

```powershell
python -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install --upgrade pip
```

如果 PowerShell 阻止激活脚本，可以直接使用虚拟环境中的 Python：

```powershell
.\.venv\Scripts\python.exe -m pip install --upgrade pip
```

### 11.2 安装依赖

完整安装命令：

```powershell
python -m pip install numpy pybullet torch tqdm matplotlib
```

各依赖的主要作用：

| 依赖 | 作用 |
| --- | --- |
| `pybullet` | 机械臂物理仿真 |
| `numpy` | 数值计算和点云数据 |
| `torch` | PPO 神经网络训练与推理 |
| `tqdm` | 训练和测试进度条 |
| `matplotlib` | 可达空间图和训练曲线 |

只运行普通 PyBullet 仿真时，安装 `pybullet` 即可：

```powershell
python -m pip install pybullet
```

### 11.3 运行仿真

只运行仿真：

```powershell
python .\Pybullet\Pybullet.py
```

同时控制真实机械臂：

```powershell
python .\Pybullet\Pybullet.py --robot-ip 192.168.1.100
```

## 12. PPO 强化学习教程

> 本节训练和演示的是 PyBullet 仿真环境。`play_fixed_target_policy.py` 当前不会把 PPO 动作发送给真实机械臂。

进入脚本目录：

```powershell
Set-Location .\jxb_study\scripts
```

后续训练模型和训练曲线默认保存在当前目录，因此建议始终从这个目录执行命令。

### 12.1 测试强化学习环境

先进行少量随机动作测试：

```powershell
python .\random_env_test.py --steps 30
```

打开图形界面观察：

```powershell
python .\random_env_test.py --steps 100 --render
```

指定固定目标点：

```powershell
python .\random_env_test.py --steps 100 --render --target -0.02 -0.10 0.14
```

### 12.2 查看奖励函数

```powershell
python .\debug_reward_display.py
```

窗口中可以通过舵机滑条改变姿态，观察：

- 末端位置
- 目标位置
- 末端与目标的距离
- 每一步接近目标的进度
- 朝向分数
- 运动奖励
- 抖动和速度相关惩罚

### 12.3 重新计算可达空间

项目已经包含生成好的可达点云。机械臂模型、初始角度或关节范围发生变化后，应重新生成：

```powershell
python .\reachable_workspace.py --samples 5000
```

结果保存在 `jxb_study/outputs`：

```text
reachable_points.npy
reachable_workspace.json
reachable_workspace.png
```

只计算数据、不生成图片：

```powershell
python .\reachable_workspace.py --samples 5000 --no-plot
```

### 12.4 生成随机目标点

```powershell
python .\target_sampler.py --count 10
```

结果保存到：

```text
jxb_study/outputs/random_targets.json
```

更换随机种子：

```powershell
python .\target_sampler.py --count 10 --seed 123
```

### 12.5 先做短训练测试

正式训练前，先确认环境和依赖正常：

```powershell
python .\train_fixed_target_ppo.py --iterations 10 --rollout-steps 32
```

短训练只能用于检查程序是否正常，不代表模型已经学会到达目标。

### 12.6 正式训练

使用默认固定目标：

```powershell
python .\train_fixed_target_ppo.py
```

默认参数：

| 参数 | 默认值 | 含义 |
| --- | ---: | --- |
| `--target` | `-0.02 -0.10 0.14` | 固定目标点，单位为米 |
| `--iterations` | `2000` | PPO 训练迭代次数 |
| `--rollout-steps` | `64` | 每次更新前采集的步数 |
| `--device` | `cpu` | PyTorch 运行设备 |
| `--metrics-path` | `training_metrics.csv` | 训练指标文件 |
| `--plot-path` | `training_curve.png` | 训练曲线图片 |

自定义示例：

```powershell
python .\train_fixed_target_ppo.py `
  --target -0.02 -0.10 0.14 `
  --iterations 2000 `
  --rollout-steps 64 `
  --metrics-path training_metrics.csv `
  --plot-path training_curve.png
```

训练期间每 200 次迭代保存一次模型：

```text
arm_ppo_200.pt
arm_ppo_400.pt
...
arm_ppo_final.pt
```

训练结束后还会生成：

- `training_metrics.csv`：奖励、成功次数、抖动惩罚、速度惩罚和损失。
- `training_curve.png`：平均奖励与成功次数曲线。

再次训练会覆盖同名输出文件。需要保留旧结果时，应先复制文件或为输出参数指定新名称。

### 12.7 使用训练模型进行可视化演示

```powershell
python .\play_fixed_target_policy.py --model-path .\arm_ppo_final.pt
```

指定与训练时相同的目标点：

```powershell
python .\play_fixed_target_policy.py `
  --model-path .\arm_ppo_final.pt `
  --target -0.02 -0.10 0.14
```

如果目标点与训练时差别很大，固定目标模型可能无法到达，这是正常现象。

### 12.8 并行环境测试

并行环境用于验证多个无界面 PyBullet 环境能否一起运行：

```powershell
python .\parallel_env_test.py --num-envs 8 --steps 1000
```

并行环境会占用更多 CPU 和内存。普通课堂电脑可以先将 `--num-envs` 改为 `2` 或 `4`。

## 13. 关键配置位置

### 13.1 ESP32 配置

在 `JXB.ino` 中：

| 配置 | 当前值 | 说明 |
| --- | --- | --- |
| PCA9685 地址 | `0x40` | I²C 地址 |
| 舵机通道 | `0` 至 `4` | 共 5 路 |
| 脉宽范围 | `75` 至 `510` | 需按舵机实际情况校准 |
| PWM 频率 | `50 Hz` | 常见模拟舵机频率 |
| 平滑步长 | `1°` | 每次控制循环的角度变化 |
| HTTP 端口 | `80` | 网页和接口 |
| UDP 端口 | `8888` | PyBullet 实物控制 |
| UDP 超时 | `1500 ms` | 超时后停止 |

### 13.2 PyBullet 配置

在 `Pybullet/Pybullet.py` 中：

| 配置 | 说明 |
| --- | --- |
| `ROBOT_BASE_POSITION` | 模型在仿真世界中的位置 |
| `SERVO_INIT_DEG` | 仿真舵机初始角度 |
| `SERVO_DIRECTION` | 仿真与真实舵机的方向映射 |
| `UDP_PORT` | 真实机械臂 UDP 端口 |
| `SEND_HZ` | 最大角度发送频率 |
| `TOOL_TIP_LOCAL_OFFSET` | 末端工具点偏移 |

### 13.3 PPO 配置

在 `jxb_study/scripts/reach_env.py` 中可以调整：

- 动作幅度
- 动作平滑系数
- 最大步数
- 成功距离
- 接近目标奖励
- 不动和小动作惩罚
- 抖动惩罚
- 关节速度惩罚
- 关节活动范围

调整奖励参数后，应重新训练模型，旧模型不会自动适应新的奖励设置。

## 14. 常见问题

### 14.1 ESP32 一直显示 `Connecting to WiFi`

检查：

- WiFi 名称和密码是否正确。
- 路由器是否开启 2.4 GHz WiFi。
- ESP32 是否在信号覆盖范围内。
- WiFi 是否需要网页认证。

当前固件没有连接超时和热点备用模式，连接失败时会一直等待。

### 14.2 浏览器打不开控制页面

检查：

- 电脑与 ESP32 是否连接同一个局域网。
- 是否使用串口最新打印的 IP。
- 浏览器地址是否以 `http://` 开头。
- 路由器是否启用了客户端隔离。

可以先在 PowerShell 测试：

```powershell
Test-Connection 192.168.1.100
```

### 14.3 网页可以打开，但舵机不动

检查：

- PCA9685 的 `V+` 是否接入舵机电源。
- 舵机电源与 ESP32 是否共地。
- 舵机是否接在通道 `0` 至 `4`。
- PCA9685 地址是否为 `0x40`。
- 是否点击过 `Stop`，可尝试点击 `Run` 后再次设置角度。

### 14.4 舵机方向相反

实物舵机方向由安装方式决定。不要直接大范围拖动滑条，应先使用小角度测试。

PyBullet 与实物联动方向不一致时，可调整 `SERVO_DIRECTION` 中对应舵机的 `1` 和 `-1`。

### 14.5 PyBullet 源码提示缺少模块

例如：

```text
ModuleNotFoundError: No module named 'pybullet'
```

执行：

```powershell
python -m pip install pybullet
```

如果提示缺少 `torch`、`numpy`、`tqdm` 或 `matplotlib`，安装完整依赖：

```powershell
python -m pip install numpy pybullet torch tqdm matplotlib
```

### 14.6 PPO 演示提示找不到模型

应先进入脚本目录：

```powershell
Set-Location .\jxb_study\scripts
python .\play_fixed_target_policy.py --model-path .\arm_ppo_final.pt
```

也可以传入模型的完整路径。

### 14.7 PPO 训练成功率低

检查：

- 训练目标是否位于可达空间内。
- 演示目标是否与训练目标一致。
- 训练迭代次数是否足够。
- 是否修改过模型、初始角度、关节范围或奖励参数。
- `training_curve.png` 中平均奖励是否有上升趋势。

### 14.8 PyBullet 能运行，但真实机械臂没有同步

检查：

- 启动命令是否包含 `--robot-ip`。
- IP 是否为 ESP32 当前地址。
- ESP32 串口是否已经输出 UDP 目标地址。
- Windows 防火墙是否允许程序发送 UDP。
- UDP 端口是否为 `8888`。
- 终端是否出现 `Collision detected, WiFi command not sent`。

## 15. 课堂使用建议

推荐按以下顺序组织实验：

1. 运行免安装版 PyBullet，认识机械臂和 5 路舵机。
2. 关闭 IK，手动观察舵机角度与关节运动的关系。
3. 开启 IK，调整目标点并观察末端跟随。
4. 在断开舵机动力的情况下完成 ESP32 烧录和联网。
5. 将机械臂架空，使用网页进行小角度测试。
6. 校准固件和 PyBullet 的初始角度、方向。
7. 使用 `--robot-ip` 进行仿真与实物联动。
8. 运行随机环境和奖励可视化。
9. 进行短训练测试，再开始正式 PPO 训练。
10. 使用训练曲线和模型演示比较训练效果。

## 16. 使用检查表

### 只运行仿真

- [ ] 项目文件已完整解压。
- [ ] `Pybullet/robot_arm2.SLDASM` 目录存在。
- [ ] `JXB_Pybullet.exe` 能正常打开。
- [ ] IK 和舵机滑条可以使用。

### 控制真实机械臂

- [ ] 舵机使用独立电源。
- [ ] ESP32、PCA9685 和舵机电源已经共地。
- [ ] 已确认 I²C 引脚和 PCA9685 地址。
- [ ] 已修改现场 WiFi 名称和密码。
- [ ] 初始角度不会导致机械臂碰撞。
- [ ] 串口波特率为 `115200`。
- [ ] 已记录 ESP32 当前 IP。
- [ ] 电脑与 ESP32 在同一局域网。

### 进行 PPO 训练

- [ ] Python 依赖安装完成。
- [ ] `jxb_study/outputs/reachable_points.npy` 存在。
- [ ] 随机环境测试通过。
- [ ] 短训练测试通过。
- [ ] 正式训练前已规划好模型和指标文件名。

## 17. 当前版本的已知限制

- 固件只连接指定局域网，没有 WiFi 连接超时或备用热点。
- 固件与 PyBullet 的第 1 路默认初始角度目前不一致，实物联动前必须校准。
- UDP 会话开始后，超时保护会持续生效；从 UDP 切换回网页控制时建议重启 ESP32。
- PPO 演示程序只控制 PyBullet，不会直接控制真实机械臂。
- 训练脚本针对固定目标，换到差别较大的目标后不一定有效。
- 当前没有自动保存网页调整角度的功能，重启后会恢复代码中的初始配置。

完成以上步骤后，同学即可分别使用仿真、网页实物控制和 PPO 训练功能。
