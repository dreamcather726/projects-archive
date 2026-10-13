// =======================================================
// ESP_jxb.ino
// 6 个舵机单独控制（PCA9685 通道 0-5）
//
// 硬件:
//   ESP32 → PCA9685: SDA = 14, SCL = 15
//   PCA9685 通道:
//     通道 0 = 舵机1
//     通道 1 = 舵机2
//     通道 2 = 舵机3
//     通道 3 = 舵机4
//     通道 4 = 舵机5
//     通道 5 = 舵机6
//
// 串口: 115200
//
// 串口协议:
//   SERVO1:角度 → 控制舵机1 (通道 0)
//   SERVO2:角度 → 控制舵机2 (通道 1)
//   SERVO3:角度 → 控制舵机3 (通道 2)
//   SERVO4:角度 → 控制舵机4 (通道 3)
//   SERVO5:角度 → 控制舵机5 (通道 4)
//   SERVO6:角度 → 控制舵机6 (通道 5)
//   ALL:角度1,角度2,角度3,角度4,角度5,角度6 → 同时控制6个舵机
//   角度范围: 0~180
// =======================================================

// 引入必要的库
#include <Wire.h>           // I2C 通信库
#include <Adafruit_PWMServoDriver.h>  // PCA9685 舵机驱动库

// PCA9685 配置
#define PCA9685_ADDR    0x40  // PCA9685 I2C 地址
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(PCA9685_ADDR);

// 舵机控制参数
#define PWM_FREQ        50          // PWM 频率（50Hz）
#define SERVO_MIN_PULSE 150         // 舵机最小脉冲宽度（对应 0°）
#define SERVO_MAX_PULSE 600         // 舵机最大脉冲宽度（对应 180°）

// 舵机通道映射
const uint8_t SERVO_CHANNELS[6] = {0, 1, 2, 3, 4, 5};  // 6个舵机对应的 PCA9685 通道

// 角度参数
const int ANGLE_MIN = 0;         // 最小角度
const int ANGLE_MAX = 180;       // 最大角度
const int ANGLE_INIT = 90;       // 初始角度
const uint16_t STEP_DELAY_MS = 2;  // 平滑移动的步长延迟（毫秒）

// 存储每个舵机当前角度的数组
int servoAngles[6] = {ANGLE_INIT, ANGLE_INIT, ANGLE_INIT, ANGLE_INIT, 
                      ANGLE_INIT, ANGLE_INIT};

/**
 * 将角度转换为脉冲宽度
 * @param angle 角度值（0-180）
 * @return 对应的脉冲宽度
 */
uint16_t angleToPulse(int angle) {
  // 限制角度在有效范围内
  angle = constrain(angle, ANGLE_MIN, ANGLE_MAX);
  // 将角度映射到脉冲宽度范围
  return map(angle, ANGLE_MIN, ANGLE_MAX, SERVO_MIN_PULSE, SERVO_MAX_PULSE);
}

/**
 * 设置单个舵机的角度
 * @param ch PCA9685 通道号
 * @param angle 目标角度
 */
void setServoAngle(uint8_t ch, int angle) {
  // 限制角度在有效范围内
  angle = constrain(angle, ANGLE_MIN, ANGLE_MAX);
  // 计算脉冲宽度
  uint16_t pulselen = angleToPulse(angle);
  // 通过 PCA9685 设置 PWM
  pwm.setPWM(ch, 0, pulselen);
}

/**
 * 平滑移动舵机到目标角度
 * @param targets 目标角度数组
 */
void moveServosSmooth(int targets[6]) {
  // 检查是否需要移动
  bool needMove = false;
  for (int i = 0; i < 6; i++) {
    // 限制目标角度在有效范围内
    targets[i] = constrain(targets[i], ANGLE_MIN, ANGLE_MAX);
    // 如果任何一个舵机角度需要改变，标记需要移动
    if (servoAngles[i] != targets[i]) {
      needMove = true;
    }
  }
  // 如果不需要移动，直接返回
  if (!needMove) {
    return;
  }

  // 计算每个舵机的移动方向
  int steps[6];
  for (int i = 0; i < 6; i++) {
    if (servoAngles[i] < targets[i]) {
      steps[i] = 1;   // 正方向移动
    } else if (servoAngles[i] > targets[i]) {
      steps[i] = -1;  // 反方向移动
    } else {
      steps[i] = 0;   // 不需要移动
    }
  }

  // 开始平滑移动
  bool moving = true;
  while (moving) {
    moving = false;
    // 更新每个舵机的角度
    for (int i = 0; i < 6; i++) {
      if (servoAngles[i] != targets[i]) {
        servoAngles[i] += steps[i];
        moving = true;
      }
    }

    // 应用新角度
    for (int i = 0; i < 6; i++) {
      setServoAngle(SERVO_CHANNELS[i], servoAngles[i]);
    }

    // 延迟一段时间，使移动更平滑
    delay(STEP_DELAY_MS);
  }

  // 确保所有舵机都到达目标位置
  for (int i = 0; i < 6; i++) {
    setServoAngle(SERVO_CHANNELS[i], servoAngles[i]);
  }
}

/**
 * 处理单个舵机的控制指令
 * @param servoNum 舵机编号（1-6）
 * @param angle 目标角度
 */
void handleSingleServo(int servoNum, int angle) {
  // 检查舵机编号是否有效
  if (servoNum < 1 || servoNum > 6) {
    Serial.println("[SERVO] 舵机编号无效，必须是1-6");
    return;
  }

  // 限制角度在有效范围内
  int A = constrain(angle, ANGLE_MIN, ANGLE_MAX);

  // 创建目标角度数组，保持其他舵机角度不变
  int targets[6];
  for (int i = 0; i < 6; i++) {
    targets[i] = servoAngles[i];
  }

  // 设置目标舵机的角度
  int idx = servoNum - 1;
  targets[idx] = A;

  // 打印控制信息
  Serial.print("[SERVO");
  Serial.print(servoNum);
  Serial.print("] 角度=");
  Serial.print(A);
  Serial.print(" -> 通道");
  Serial.print(SERVO_CHANNELS[idx]);
  Serial.println();

  // 平滑移动舵机
  moveServosSmooth(targets);
}

/**
 * 同时控制所有6个舵机
 * @param angle1 舵机1的目标角度
 * @param angle2 舵机2的目标角度
 * @param angle3 舵机3的目标角度
 * @param angle4 舵机4的目标角度
 * @param angle5 舵机5的目标角度
 * @param angle6 舵机6的目标角度
 */
void handleAllServos(int angle1, int angle2, int angle3, int angle4, int angle5, int angle6) {
  // 存储所有舵机的目标角度
  int angles[6] = {angle1, angle2, angle3, angle4, angle5, angle6};
  
  // 创建目标角度数组，限制每个角度在有效范围内
  int targets[6];
  for (int i = 0; i < 6; i++) {
    targets[i] = constrain(angles[i], ANGLE_MIN, ANGLE_MAX);
  }

  // 打印控制信息
  Serial.print("[ALL] 舵机角度=[");
  Serial.print(angles[0]);
  Serial.print(", ");
  Serial.print(angles[1]);
  Serial.print(", ");
  Serial.print(angles[2]);
  Serial.print(", ");
  Serial.print(angles[3]);
  Serial.print(", ");
  Serial.print(angles[4]);
  Serial.print(", ");
  Serial.print(angles[5]);
  Serial.println("]");

  // 平滑移动所有舵机
  moveServosSmooth(targets);
}

/**
 * 解析串口命令
 * @param line 串口接收的命令行
 */
void parseCommand(String line) {
  // 去除首尾空白字符
  line.trim();
  // 如果命令为空，直接返回
  if (line.length() == 0) return;

  // 打印接收到的命令
  Serial.print("[串口接收] ");
  Serial.println(line);

  // 处理 SERVO1 指令
  if (line.startsWith("SERVO1:") || line.startsWith("servo1:")) {
    int colonIndex = line.indexOf(':');
    String angleStr = line.substring(colonIndex + 1);
    int angle = angleStr.toInt();
    // 检查角度解析是否成功
    if (angle == 0 && !angleStr.startsWith("0")) {
      Serial.println("[错误] SERVO1 指令角度解析失败");
      return;
    }
    handleSingleServo(1, angle);
    return;
  }

  // 处理 SERVO2 指令
  if (line.startsWith("SERVO2:") || line.startsWith("servo2:")) {
    int colonIndex = line.indexOf(':');
    String angleStr = line.substring(colonIndex + 1);
    int angle = angleStr.toInt();
    if (angle == 0 && !angleStr.startsWith("0")) {
      Serial.println("[错误] SERVO2 指令角度解析失败");
      return;
    }
    handleSingleServo(2, angle);
    return;
  }

  // 处理 SERVO3 指令
  if (line.startsWith("SERVO3:") || line.startsWith("servo3:")) {
    int colonIndex = line.indexOf(':');
    String angleStr = line.substring(colonIndex + 1);
    int angle = angleStr.toInt();
    if (angle == 0 && !angleStr.startsWith("0")) {
      Serial.println("[错误] SERVO3 指令角度解析失败");
      return;
    }
    handleSingleServo(3, angle);
    return;
  }

  // 处理 SERVO4 指令
  if (line.startsWith("SERVO4:") || line.startsWith("servo4:")) {
    int colonIndex = line.indexOf(':');
    String angleStr = line.substring(colonIndex + 1);
    int angle = angleStr.toInt();
    if (angle == 0 && !angleStr.startsWith("0")) {
      Serial.println("[错误] SERVO4 指令角度解析失败");
      return;
    }
    handleSingleServo(4, angle);
    return;
  }

  // 处理 SERVO5 指令
  if (line.startsWith("SERVO5:") || line.startsWith("servo5:")) {
    int colonIndex = line.indexOf(':');
    String angleStr = line.substring(colonIndex + 1);
    int angle = angleStr.toInt();
    if (angle == 0 && !angleStr.startsWith("0")) {
      Serial.println("[错误] SERVO5 指令角度解析失败");
      return;
    }
    handleSingleServo(5, angle);
    return;
  }

  // 处理 SERVO6 指令
  if (line.startsWith("SERVO6:") || line.startsWith("servo6:")) {
    int colonIndex = line.indexOf(':');
    String angleStr = line.substring(colonIndex + 1);
    int angle = angleStr.toInt();
    if (angle == 0 && !angleStr.startsWith("0")) {
      Serial.println("[错误] SERVO6 指令角度解析失败");
      return;
    }
    handleSingleServo(6, angle);
    return;
  }

  // 处理 ALL 指令（同时控制所有舵机）
  if (line.startsWith("ALL:") || line.startsWith("all:")) {
    int colonIndex = line.indexOf(':');
    String params = line.substring(colonIndex + 1);
    
    // 解析6个角度值，默认为90°
    int angles[6] = {90, 90, 90, 90, 90, 90};
    int paramIndex = 0;
    int startIdx = 0;
    
    // 按逗号分割参数
    for (int i = 0; i <= params.length() && paramIndex < 6; i++) {
      if (i == params.length() || params.charAt(i) == ',') {
        String angleStr = params.substring(startIdx, i);
        angleStr.trim();
        int angle = angleStr.toInt();
        // 检查角度解析是否成功
        if (angleStr.length() > 0 || (angle == 0 && angleStr.startsWith("0"))) {
          angles[paramIndex] = angle;
          paramIndex++;
        }
        startIdx = i + 1;
      }
    }
    
    // 检查是否解析到6个角度值
    if (paramIndex == 6) {
      handleAllServos(angles[0], angles[1], angles[2], angles[3], angles[4], angles[5]);
    } else {
      Serial.println("[错误] ALL指令需要6个角度值，格式: ALL:90,90,90,90,90,90");
    }
    return;
  }

  // 未识别的指令
  Serial.println("[提示] 未识别的指令（可用: SERVO1-6:角度 或 ALL:角度1,角度2,角度3,角度4,角度5,角度6）");
}

/**
 * 初始化函数
 */
void setup() {
  // 初始化串口通信
  Serial.begin(115200);
  Serial.println();
  Serial.println("=== ESP_jxb.ino ===");

  // 初始化 I2C 通信（SDA=14, SCL=15）
  Wire.begin(14, 15);

  // 初始化 PCA9685
  pwm.begin();
  // 设置 PWM 频率为 50Hz
  pwm.setPWMFreq(PWM_FREQ);

  // 短暂延迟，确保 PCA9685 初始化完成
  delay(10);

  // 初始化所有舵机到初始角度（90°）
  for (int i = 0; i < 6; i++) {
    setServoAngle(SERVO_CHANNELS[i], ANGLE_INIT);
    servoAngles[i] = ANGLE_INIT;
  }

  // 打印初始化信息
  Serial.println("[初始化] 所有 6 个舵机已复位到 90°");
  Serial.println("[提示] SERVO1-6:角度 - 单独控制舵机");
  Serial.println("[提示] ALL:角度1,角度2,角度3,角度4,角度5,角度6 - 同时控制6个舵机");
}

/**
 * 主循环函数
 */
void loop() {
  // 检查串口是否有数据
  if (Serial.available()) {
    // 读取串口数据直到换行符
    String line = Serial.readStringUntil('\n');
    // 解析命令
    parseCommand(line);
  }
}
