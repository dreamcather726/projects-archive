// =======================================================
// hand_arduino_v1_servo1-8.ino
// 舵机1-8 控制（PCA9685 通道 0-7，4 个手指，每指 2 舵机）
//
// 硬件:
//   ESP32-S3 → PCA9685: SDA = 14, SCL = 15
//   PCA9685 通道:
//     手指1: 0 = 舵机1, 1 = 舵机2
//     手指2: 2 = 舵机3, 3 = 舵机4
//     手指3: 4 = 舵机5, 5 = 舵机6
//     手指4: 6 = 舵机7, 7 = 舵机8
//
// 串口: 115200
//
// 串口协议:
//   FINGER1:角度 -> 控制手指1 (舵机通道 0, 1)
//   FINGER2:角度 -> 控制手指2 (舵机通道 2, 3)
//   FINGER3:角度 -> 控制手指3 (舵机通道 4, 5)
//   FINGER4:角度 -> 控制手指4 (舵机通道 6, 7)
//   ALL:角度1,角度2,角度3,角度4 -> 同时控制4个手指
//   角度范围: 90~180，舵机1跟随角度，舵机2反向(180-角度)
// =======================================================

// 引入I2C通信库（用于ESP32和PCA9685通信）
#include <Wire.h>
// 引入PCA9685舵机驱动库
#include <Adafruit_PWMServoDriver.h>

// PCA9685模块的I2C地址（默认0x40）
#define PCA9685_ADDR    0x40
// 创建PCA9685驱动对象
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(PCA9685_ADDR);

// 舵机PWM频率：标准模拟舵机使用50Hz（20ms周期）
#define PWM_FREQ        50
// 舵机最小脉宽（对应0°）
#define SERVO_MIN_PULSE 150
// 舵机最大脉宽（对应180°）
#define SERVO_MAX_PULSE 600

// 定义8个舵机对应的PCA9685通道号（0~7）
const uint8_t SERVO_CHANNELS[8] = {0, 1, 2, 3, 4, 5, 6, 7};

// 舵机角度限制：最小0°
const int ANGLE_MIN = 0;
// 舵机角度限制：最大180°
const int ANGLE_MAX = 180;
// 舵机初始化角度：90°（中间位置）
const int ANGLE_INIT = 90;
// 舵机平滑移动时每步的延时（毫秒），数值越大动作越慢
const uint16_t STEP_DELAY_MS = 2;

// 存储8个舵机当前角度的数组，初始全部为90°
int servoAngles[8] = {ANGLE_INIT, ANGLE_INIT, ANGLE_INIT, ANGLE_INIT,
                      ANGLE_INIT, ANGLE_INIT, ANGLE_INIT, ANGLE_INIT};

// ===================== 工具函数 =====================
// 功能：将角度值（0-180）转换为PCA9685可识别的脉宽值
// 输入：angle 角度值
// 输出：对应的PWM脉宽值
uint16_t angleToPulse(int angle) {
  // 限制角度在0~180之间，防止超出范围
  angle = constrain(angle, ANGLE_MIN, ANGLE_MAX);
  // 映射：把0-180角度 → 150-600脉宽
  return map(angle, ANGLE_MIN, ANGLE_MAX, SERVO_MIN_PULSE, SERVO_MAX_PULSE);
}

// ===================== 舵机控制函数 =====================
// 功能：设置单个舵机的目标角度
// 输入：ch 舵机通道；angle 目标角度
void setServoAngle(uint8_t ch, int angle) {
  // 限制角度安全范围
  angle = constrain(angle, ANGLE_MIN, ANGLE_MAX);
  // 角度转PWM脉宽
  uint16_t pulselen = angleToPulse(angle);
  // 给PCA9685发送PWM信号，控制舵机
  pwm.setPWM(ch, 0, pulselen);
}

// 功能：平滑移动所有8个舵机到目标角度（防止抖动、卡顿）
// 输入：targets[8] 8个舵机的目标角度数组
void moveServosSmooth(int targets[8]) {
  // 标记是否需要移动（只要有一个舵机角度不同就需要动）
  bool needMove = false;
  for (int i = 0; i < 8; i++) {
    // 目标角度限制安全范围
    targets[i] = constrain(targets[i], ANGLE_MIN, ANGLE_MAX);
    // 判断当前角度≠目标角度 → 需要移动
    if (servoAngles[i] != targets[i]) {
      needMove = true;
    }
  }
  // 如果所有舵机都已经在目标位置，直接退出，不执行移动
  if (!needMove) {
    return;
  }

  // 定义每个舵机的移动方向：1=递增，-1=递减，0=不动
  int steps[8];
  for (int i = 0; i < 8; i++) {
    if (servoAngles[i] < targets[i]) {
      steps[i] = 1;   // 当前角度小 → 向上加
    } else if (servoAngles[i] > targets[i]) {
      steps[i] = -1;  // 当前角度大 → 向下减
    } else {
      steps[i] = 0;   // 已到位 → 不动
    }
  }

  // 循环逐度移动，直到所有舵机到达目标
  bool moving = true;
  while (moving) {
    moving = false;  // 先假设已经停止移动
    // 逐个更新舵机角度（每次+1或-1）
    for (int i = 0; i < 8; i++) {
      if (servoAngles[i] != targets[i]) {
        servoAngles[i] += steps[i];
        moving = true;  // 还有舵机没到位，继续循环
      }
    }

    // 把更新后的角度同步输出到所有舵机
    for (int i = 0; i < 8; i++) {
      setServoAngle(SERVO_CHANNELS[i], servoAngles[i]);
    }

    // 每步延时，控制移动速度
    delay(STEP_DELAY_MS);
  }

  // 最后再刷新一次，确保舵机精准到位
  for (int i = 0; i < 8; i++) {
    setServoAngle(SERVO_CHANNELS[i], servoAngles[i]);
  }
}

// ===================== 手指控制逻辑 =====================
// 功能：处理单根手指的角度控制（1根手指=2个舵机，正反向联动）
// 输入：fingerNum 手指编号(1-4)；angleIn 输入角度
void handleFingerAngle(int fingerNum, int angleIn) {
  // 手指编号必须1~4，否则报错
  if (fingerNum < 1 || fingerNum > 4) {
    Serial.println("[FINGER] 手指编号无效，必须是1-4");
    return;
  }

  // 限制输入角度在90~180之间（项目要求）
  int A = constrain(angleIn, 90, 180);

  // 先复制当前所有舵机角度，避免未控制的舵机乱动
  int targets[8];
  for (int i = 0; i < 8; i++) {
    targets[i] = servoAngles[i];
  }

  // 计算该手指对应的2个舵机索引
  // 手指1 → 0、1；手指2 → 2、3；手指3 →4、5；手指4→6、7
  int baseIdx = (fingerNum - 1) * 2;
  // 第一个舵机 = 设定角度
  targets[baseIdx] = A;
  // 第二个舵机 = 反向角度（180 - 设定角度）
  targets[baseIdx + 1] = 180 - A;

  // 串口打印调试信息
  Serial.print("[FINGER");
  Serial.print(fingerNum);
  Serial.print("] 角度=");
  Serial.print(A);
  Serial.print(" -> 舵机");
  Serial.print(baseIdx);
  Serial.print("=");
  Serial.print(targets[baseIdx]);
  Serial.print(", 舵机");
  Serial.print(baseIdx + 1);
  Serial.print("=");
  Serial.println(targets[baseIdx + 1]);

  // 调用平滑移动函数
  moveServosSmooth(targets);
}

// 功能：同时控制4根手指（8个舵机）
// 输入：angle1~4 四根手指的角度
void handleAllFingers(int angle1, int angle2, int angle3, int angle4) {
  // 把4个角度存入数组
  int angles[4] = {angle1, angle2, angle3, angle4};
  
  // 目标角度数组
  int targets[8];
  // 循环处理4根手指，每根手指对应2个舵机
  for (int i = 0; i < 4; i++) {
    // 限制角度90~180
    int A = constrain(angles[i], 90, 180);
    // 正向舵机
    targets[i * 2] = A;
    // 反向舵机
    targets[i * 2 + 1] = 180 - A;
  }

  // 串口打印调试信息
  Serial.print("[ALL] 手指角度=[");
  Serial.print(angles[0]);
  Serial.print(", ");
  Serial.print(angles[1]);
  Serial.print(", ");
  Serial.print(angles[2]);
  Serial.print(", ");
  Serial.print(angles[3]);
  Serial.println("]");

  // 平滑移动所有舵机
  moveServosSmooth(targets);
}

// ===================== 串口指令解析 =====================
// 功能：解析串口收到的字符串指令
// 输入：line 串口收到的一行字符串
void parseCommand(String line) {
  // 去掉字符串首尾空格、换行符
  line.trim();
  // 空指令直接忽略
  if (line.length() == 0) return;

  // 串口回显收到的指令（方便调试）
  Serial.print("[串口接收] ");
  Serial.println(line);

  // ---------------- 指令1：FINGER1:角度 ----------------
  if (line.startsWith("FINGER1:") || line.startsWith("finger1:")) {
    // 找到冒号位置
    int colonIndex = line.indexOf(':');
    // 截取冒号后面的内容（角度字符串）
    String angleStr = line.substring(colonIndex + 1);
    // 转成整数角度
    int angle = angleStr.toInt();
    // 防错：不是数字且不是0 → 解析失败
    if (angle == 0 && !angleStr.startsWith("0")) {
      Serial.println("[错误] FINGER1 指令角度解析失败");
      return;
    }
    // 调用手指1控制函数
    handleFingerAngle(1, angle);
    return;
  }

  // ---------------- 指令2：FINGER2:角度 ----------------
  if (line.startsWith("FINGER2:") || line.startsWith("finger2:")) {
    int colonIndex = line.indexOf(':');
    String angleStr = line.substring(colonIndex + 1);
    int angle = angleStr.toInt();
    if (angle == 0 && !angleStr.startsWith("0")) {
      Serial.println("[错误] FINGER2 指令角度解析失败");
      return;
    }
    handleFingerAngle(2, angle);
    return;
  }

  // ---------------- 指令3：FINGER3:角度 ----------------
  if (line.startsWith("FINGER3:") || line.startsWith("finger3:")) {
    int colonIndex = line.indexOf(':');
    String angleStr = line.substring(colonIndex + 1);
    int angle = angleStr.toInt();
    if (angle == 0 && !angleStr.startsWith("0")) {
      Serial.println("[错误] FINGER3 指令角度解析失败");
      return;
    }
    handleFingerAngle(3, angle);
    return;
  }

  // ---------------- 指令4：FINGER4:角度 ----------------
  if (line.startsWith("FINGER4:") || line.startsWith("finger4:")) {
    int colonIndex = line.indexOf(':');
    String angleStr = line.substring(colonIndex + 1);
    int angle = angleStr.toInt();
    if (angle == 0 && !angleStr.startsWith("0")) {
      Serial.println("[错误] FINGER4 指令角度解析失败");
      return;
    }
    handleFingerAngle(4, angle);
    return;
  }

  // ---------------- 指令5：ALL:角度1,角度2,角度3,角度4 ----------------
  if (line.startsWith("ALL:") || line.startsWith("all:")) {
    int colonIndex = line.indexOf(':');
    // 截取冒号后所有参数
    String params = line.substring(colonIndex + 1);
    
    // 存储解析出的4个角度，默认90°
    int angles[4] = {90, 90, 90, 90};
    int paramIndex = 0;
    int startIdx = 0;
    
    // 遍历字符串，按逗号分割出4个角度
    for (int i = 0; i <= params.length() && paramIndex < 4; i++) {
      // 遇到逗号或结尾，就截取一段
      if (i == params.length() || params.charAt(i) == ',') {
        String angleStr = params.substring(startIdx, i);
        angleStr.trim();
        int angle = angleStr.toInt();
        // 有效角度才存入
        if (angleStr.length() > 0 || (angle == 0 && angleStr.startsWith("0"))) {
          angles[paramIndex] = angle;
          paramIndex++;
        }
        startIdx = i + 1;
      }
    }
    
    // 必须解析出4个角度才执行
    if (paramIndex == 4) {
      handleAllFingers(angles[0], angles[1], angles[2], angles[3]);
    } else {
      Serial.println("[错误] ALL指令需要4个角度值，格式: ALL:90,90,90,90");
    }
    return;
  }

  // 不认识的指令
  Serial.println("[提示] 未识别的指令（可用: FINGER1-4:角度 或 ALL:角度1,角度2,角度3,角度4）");
}

// ===================== 初始化函数 =====================
void setup() {
  // 开启串口，波特率115200
  Serial.begin(115200);
  Serial.println();
  Serial.println("=== hand_arduino_v1_servo1-8.ino ===");

  // 初始化I2C：SDA=14，SCL=15（ESP32-S3引脚）
  Wire.begin(14, 15);

  // 初始化PCA9685
  pwm.begin();
  // 设置PWM频率50Hz（舵机标准频率）
  pwm.setPWMFreq(PWM_FREQ);

  // 短暂延时，等待硬件稳定
  delay(10);

  // 所有8个舵机复位到90°初始位置
  for (int i = 0; i < 8; i++) {
    setServoAngle(SERVO_CHANNELS[i], ANGLE_INIT);
    servoAngles[i] = ANGLE_INIT;
  }

  // 开机提示信息
  Serial.println("[初始化] 所有 8 个舵机已复位到 90°");
  Serial.println("[提示] FINGER1-4:角度 - 单独控制手指");
  Serial.println("[提示] ALL:角度1,角度2,角度3,角度4 - 同时控制4个手指");
}

// ===================== 主循环 =====================
void loop() {
  // 如果串口有数据可读
  if (Serial.available()) {
    // 读取一行数据（直到换行符\n）
    String line = Serial.readStringUntil('\n');
    // 解析并执行指令
    parseCommand(line);
  }
}
