# Arduino IDE + conda + ESP32 入门教案

> 面向对象：初学者  
> 目标产出：ESP32 串口输出成功 + LED 闪烁 + PWM 呼吸灯 + 串口指令开关灯  
> 说明：已移除图片链接、个人目录示例、网络/渠道等复杂细节；需要的安装包/地址由老师课堂提供。

---

## 0. 教学目标
- 会用 Arduino IDE：新建/保存/编译/上传/串口监视器
- 会用 conda：创建/激活/删除环境；安装/卸载常用包
- 会配 ESP32：驱动识别 → 安装板卡 → 选端口 → 烧录验证
- 会做 LED 实验：接线 + 限流电阻 + Blink + PWM + 串口控制

## 1. 前置知识（课堂可边做边讲）
- 会打开软件与设备管理器，能找到 COM 端口
- 理解：高电平/低电平；LED 有正负极；电阻用于限流保护

### 1.1 电脑熟悉操作（Windows：命令行与 cd）
课堂会用到“命令行窗口”。命令行里最重要的是两件事：当前所在文件夹 + 运行命令。

常用命令（按使用频率）：
```bash
cd 路径          # 切换到某个文件夹
cd ..            # 返回上一级
cd \             # 回到当前盘根目录
cd /d D:\Class   # 切换到指定盘并进入文件夹（Windows 常用写法）
dir              # 列出当前文件夹内容（类似“查看文件”）
cls              # 清屏
mkdir test       # 新建文件夹 test
rmdir test       # 删除空文件夹 test
explorer .       # 用资源管理器打开当前文件夹

```

路径小规则（课堂统一讲清）：
- 路径里有空格时要加引号：`cd "D:\My Projects\hand1"`
- 相对路径：相对“当前文件夹”（例如 `cd ..`）
- 绝对路径：从盘符开始（例如 `D:\Class\hand1`）

## 2. 软硬件清单
### 2.1 硬件
| 名称 | 数量 | 备注 |
|---|---:|---|
| ESP32 开发板 | 1 | 任意常见型号 |
| USB 数据线 | 1 | 必须可传数据 |
| 面包板 | 1 | |
| LED | 1–2 | 红/绿均可 |
| 杜邦线 | 若干 | |

### 2.2 软件
| 软件 | 用途 |
|---|---|
| Arduino IDE 2.x | 编写/编译/烧录 |
| Anaconda / Miniconda | conda 环境管理 |
| 串口驱动（按板子芯片） | 让电脑识别 COM 口 |

---



---

# 模块 1：Arduino IDE（安装、界面、项目保存、串口调试）

## 1.1 安装流程（教师演示，学生跟做）
1. 安装 Arduino IDE（安装包由老师提供）
2. 打开软件 → 能看到编辑区与工具栏
3. 打开示例：`File → Examples`


## 1.2 界面快速认识
- Verify：只编译不上传
- Upload：编译并上传到开发板
- Board/Port：选择开发板型号与 COM 口
- Serial Monitor：看输出、发输入

## 1.3 项目创建与保存规范（课堂统一）
- 先保存再写：`File → Save As...`
- 工程名用英文/数字/下划线：`esp32_hello`、`esp32_led`

## 1.4 串口调试要点（课堂必考）
- 波特率必须一致：代码 `Serial.begin(115200);`，监视器也选 115200
- 乱码：先改波特率
- 没输出：先确认上传成功、端口正确、监视器已打开

### 课堂演示代码：串口计数器
```cpp
void setup() {
  Serial.begin(115200);
  Serial.println("Serial Ready");
}

void loop() {
  static int counter = 0;
  Serial.print("counter=");
  Serial.println(counter++);
  delay(500);
}
```

### 板书（模块 1）
```
写程序流程：写 → 编译(Verify) → 选 Board/Port → 上传(Upload) → 串口看结果
串口两件事：端口(COMx) + 波特率(115200)
```

### 随堂小测（模块 1）
1. Verify 与 Upload 有什么区别？
2. 串口乱码最常见原因是什么？

### 模块 1 小作业（提交：截图 + 代码）
1. 用命令行创建一个作业文件夹并打开（截图需包含命令与结果）：
```bash
cd /d D:\
mkdir hand_hw
cd hand_hw
explorer .
```
2. 在 Arduino IDE 新建工程并保存到 `hand_hw` 文件夹里，工程名：`m1_arduino_hello`
3. 把下面代码粘贴进去并保存（不要求上传，能保存即可）：
```cpp
void setup() {
  Serial.begin(115200);
  Serial.println("M1 OK");
}

void loop() {
  delay(1000);
}
```
4. 提交：工程文件夹打包 + Arduino IDE 代码截图（不包含个人路径与姓名）

---

# 模块 2：Anaconda/conda（环境创建、激活、删除、装包）

## 2.1 核心概念（1 分钟）
环境 = 一套独立的 Python + 包；不同项目互不影响。

你可以把环境理解成“项目专用工具箱”：
- 同一台电脑上可以有多个工具箱（环境），每个工具箱里装的工具版本不同
- 好处：A 项目需要旧版本，B 项目需要新版本，互不打架

课堂最常见的 3 个场景：
- 电脑里原本的 Python/包太乱，用新环境从零开始最省事
- 同学之间统一环境名与版本，便于老师检查与复现
- 做完实验直接删环境，不留垃圾（注意：删环境会把里面的包一起删除）

快速判断“当前在哪个环境”：
- 命令行提示符通常会显示环境名（例如 `(hand1)`）


## 2.2 课堂统一环境名
环境名：`hand1`

## 2.3 命令演示（投屏逐条执行）
查看 conda 与环境：
```bash
conda --version
conda env list
```

创建环境：
```bash
conda create -n hand1 python=3.10
```

激活环境：
```bash
conda activate hand1
python --version
```

安装常用包：
```bash
conda install numpy pandas matplotlib
```

卸载包：
```bash
conda remove matplotlib
```

删除环境（先退出环境再删）：
```bash
conda deactivate
conda remove -n hand1 --all
```

## 2.4 常见错误（只保留课堂常见 3 个）
| 现象 | 可能原因 | 处理 |
|---|---|---|
| 找不到 conda 命令 | 未用正确终端 | 使用 Anaconda Prompt |
| 环境名不存在 | 名字拼错 | `conda env list` 复制环境名 |
| 安装卡住很久 | 网络慢或依赖解析慢 | 稍等；必要时分开装：先装 numpy 再装 pandas |

### 板书（模块 2）
```
三条主命令：create / activate / remove
删除环境：conda remove -n NAME --all
```

### 随堂小测（模块 2）
1. 为什么要用虚拟环境？
2. 删除环境是哪条命令？

### 模块 2 小作业（提交：截图）
1. 创建并激活环境 `hand1`（截图需包含命令与结果）：
```bash
conda create -n hand1 python=3.10
conda activate hand1
python --version
```
2. 安装 numpy 并验证（截图需包含验证输出）：
```bash
conda install numpy
python
```
在 Python 里输入：
```python
import numpy as np
print(np.arange(10))
```
3. 退出 Python（`exit()`）并退出环境：
```bash
conda deactivate
```

---

# 模块 3：ESP32 + Arduino IDE（驱动、板卡、端口、Hello World）

## 3.1 驱动与端口确认（Windows）
1. 插上 ESP32
2. 设备管理器 → 端口（COM 和 LPT）
3. 能看到类似 `... (COMx)` 即正常

【图片占位符】  
`[此处放：设备管理器 COM 口截图]`

## 3.2 安装 ESP32 开发板支持包（教师带做）
- Arduino IDE → Preferences → Boards Manager URL（由老师提供）
- Boards Manager 搜索 `esp32` 并安装

## 3.3 选择 Board 与 Port（最重要）
- `Tools → Board`：选择 ESP32（课堂统一一个型号）
- `Tools → Port`：选择出现的 COMx

## 3.4 验证程序：Hello World（可直接烧录）
```cpp
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("Hello World!");
}

void loop() {
  Serial.println("ESP32 running...");
  delay(1000);
}
```

### 板书（模块 3）
```
上传失败优先查三项：Board 对 / Port 对 / 线对(数据线)
串口输出验证：115200
```

### 随堂小测（模块 3）
1. 如何确认电脑识别到 ESP32？（一句话）
2. 串口监视器要设置什么参数才不乱码？

### 模块 3 小作业（提交：截图 + 代码）
1. 设备管理器截图：能看到 `端口（COM 和 LPT）` 中出现 `... (COMx)`
2. Arduino IDE 截图：能看到已选择正确的 Board 与 Port
3. 烧录并截图串口监视器：Hello World 稳定输出至少 3 行
4. 提交：`m3_esp32_hello.ino`（或工程文件夹打包）

---

# 模块 4：ESP32 控制 LED（接线 + Blink + PWM + 串口控制）

## 4.1 电路连接（统一 GPIO25）
连接关系：
- GPIO25 → 电阻 → LED 长脚（正极）
- LED 短脚（负极）→ GND

电路示意：
```
GPIO25 ----[330Ω]----|>|---- GND
                     LED
```

【图片占位符】  
`[此处放：面包板接线俯拍图]`

## 4.2 限流电阻怎么选（课堂简化规则）
- 推荐：330Ω（大多数情况下亮度合适且更安全）
- 也可用：220Ω（更亮）或 1kΩ（更暗）

## 4.3 实验 1：Blink（最小可跑）
```cpp
const int LED_PIN = 25;

void setup() {
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
  delay(500);
}
```

逐行讲解关键词：
- `pinMode`：设置输出
- `digitalWrite(HIGH/LOW)`：亮/灭
- `delay`：控制闪烁速度

## 4.4 实验 2：PWM 呼吸灯（简化讲法：0~255 亮度）
```cpp
const int LED_PIN = 25;
const int PWM_CHANNEL = 0;
const int PWM_FREQ = 5000;
const int PWM_RESOLUTION = 8;

void setup() {
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(LED_PIN, PWM_CHANNEL);
}

void loop() {
  for (int duty = 0; duty <= 255; duty++) {
    ledcWrite(PWM_CHANNEL, duty);
    delay(5);
  }
  for (int duty = 255; duty >= 0; duty--) {
    ledcWrite(PWM_CHANNEL, duty);
    delay(5);
  }
}
```

## 4.5 实验 3：串口指令控制（课堂版：on/off/pwm）
使用方法（串口监视器输入）：
- `on`：常亮
- `off`：熄灭
- `pwm 0-255`：设置亮度

```cpp
const int LED_PIN = 25;
const int PWM_CHANNEL = 0;
const int PWM_FREQ = 5000;
const int PWM_RESOLUTION = 8;

String line;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(LED_PIN, PWM_CHANNEL);
  Serial.println("Type: on | off | pwm 0-255");
}

void loop() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      line.trim();
      if (line == "on") {
        digitalWrite(LED_PIN, HIGH);
        Serial.println("OK: ON");
      } else if (line == "off") {
        digitalWrite(LED_PIN, LOW);
        Serial.println("OK: OFF");
      } else if (line.startsWith("pwm")) {
        int spaceIndex = line.indexOf(' ');
        if (spaceIndex > 0) {
          int v = line.substring(spaceIndex + 1).toInt();
          if (v < 0) v = 0;
          if (v > 255) v = 255;
          ledcWrite(PWM_CHANNEL, v);
          Serial.print("OK: PWM=");
          Serial.println(v);
        } else {
          Serial.println("ERR: pwm needs a number (0-255)");
        }
      } else if (line.length() > 0) {
        Serial.print("ERR: ");
        Serial.println(line);
      }
      line = "";
    } else {
      line += c;
    }
  }
}
```

### 学生动手环节（模块 4，建议 30 分钟）
1. 按图接线，先做 Blink
2. 成功后改成呼吸灯
3. 最后做串口指令控制，并现场演示 3 条指令

### 课堂练习（模块 4）
- 把 Blink 间隔改成 100ms 与 1000ms
- 给 `pwm` 加一个默认值（例如开机自动 128）

### 随堂小测（模块 4）
1. LED 为什么一定要串联电阻？
2. PWM 数值越大 LED 越亮，这个数值范围是多少？

---

# 模块 4 小作业（提交：截图 + 代码）
必做（两项都要）：
1. 接线截图：清晰看到 GPIO25 → 电阻 → LED → GND
2. 完成 Blink 并录/截图证明 LED 在闪（建议视频 5–10 秒，或连续两张不同状态照片）

选做加分（任选一项）：
- 完成 PWM 呼吸灯并录/截图
- 完成串口指令控制，截图包含输入 `on`、`off`、`pwm 128` 的返回信息

提交：对应 `.ino` 工程文件夹打包

---

# 大作业（综合）
> 目标：把四个模块串起来，做一个“串口菜单 + LED 多模式控制”的完整作品。

## 功能要求（必做）
1. LED 支持 4 种模式：`off`、`on`、`blink`、`pwm`
2. 串口支持命令（大小写不敏感可选）：
   - `help`：打印命令说明
   - `status`：打印当前模式与亮度值（0–255）
   - `on` / `off`
3. 串口输出必须清晰（建议统一前缀，例如 `OK:` / `ERR:`）

## 电脑与 conda 要求（必做）
1. 用命令行创建文件夹结构（截图）：
```bash
cd /d D:\
mkdir hand_final
cd hand_final
mkdir code
mkdir screenshots
explorer .
```
2. 使用 conda 环境 `hand1` 运行一次验证（截图）：
```bash
conda activate hand1
python --version
python -c "import numpy as np; print(np.sum(np.arange(10)))"
conda deactivate
```

## 提交物（统一格式，避免隐私信息）
- `code/`：Arduino 工程文件夹（含 `.ino`）
- `screenshots/`：至少 4 张截图
  - 命令行建目录截图
  - conda 验证截图
  - 串口 `help/status` 截图
  - 串口控制 LED（`blink` 与 `pwm`）截图
- 将 `hand_final` 整体打包为：`班级_学号_大作业.zip`（不要包含姓名与个人路径）

# 课堂成果评价（简化版）
- 必做 1：ESP32 串口输出能持续打印
- 必做 2：LED Blink 正常
- 选做 1：PWM 呼吸灯
- 选做 2：串口输入控制 LED（on/off/pwm）

---

# 附录：快速故障排查（只保留课堂高频）
| 现象 | 优先检查 |
|---|---|
| 没有 COM 口 | 数据线是否可传数据；驱动是否安装；换 USB 口 |
| 上传失败 | Board 是否选对；Port 是否选对；串口是否被占用 |
| 串口乱码 | 波特率是否一致（常用 115200） |
| LED 不亮 | LED 极性是否反；电阻是否串联；GPIO 与 GND 是否接对 |
