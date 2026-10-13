// ############################################################
// 敲击顺序：1 准备工作：注释\引入库 → 2设定规则： 常量数组对象 → 3基础指令:工具函数 → 4 针对性指令：手指动作 → 5 总控制执行:串口解析、setup+loop
// ############################################################

// ==============================================
// 【模块1：头部注释】—— 第1个敲
// 涉及语法：// 单行注释
// 代码作用：程序说明书，记录功能、硬件接线、串口指令格式
// ==============================================
// =======================================================
// hand_arduino_v1_servo1-8.ino
// 舵机1-8 控制（PCA9685 通道 0-7，4 个手指，每指 2 舵机）
// 硬件: ESP32-S3 → PCA9685: SDA=14, SCL=15
// 串口波特率: 115200
// 指令: FINGER1:角度  /  ALL:角度1,角度2,角度3,角度4
// =======================================================

// ==============================================
// 【模块2：引入库文件】—— 第2个敲
// 涉及语法：#include <库名.h>
// 代码作用：引入工具包，让ESP32能通信、能驱动舵机
// ==============================================
// I2C通信库，用于ESP32与PCA9685通信
#include <Wire.h>
// 舵机驱动库，用于控制PCA9685输出PWM信号
#include <Adafruit_PWMServoDriver.h>

// ==============================================
// 【模块3：常量、对象、数组】—— 第3个敲
// 涉及语法：#define、const、对象、数组
// 代码作用：定义固定参数、创建舵机控制器、存储角度
// ==============================================
// PCA9685驱动板I2C地址
#define PCA9685_ADDR    0x40
// 创建舵机驱动对象，核心控制器
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(PCA9685_ADDR);

// 舵机标准频率50Hz
#define PWM_FREQ        50
// 0度对应脉宽
#define SERVO_MIN_PULSE 150
// 180度对应脉宽
#define SERVO_MAX_PULSE 600

// 8个舵机通道号数组
const uint8_t SERVO_CHANNELS[8] = {0,1,2,3,4,5,6,7};

// 角度范围与初始值
const int ANGLE_MIN = 0;
const int ANGLE_MAX = 180;
const int ANGLE_INIT = 90;
const uint16_t STEP_DELAY_MS = 2;

// 存储8个舵机当前角度
int servoAngles[8] = {
  ANGLE_INIT,ANGLE_INIT,ANGLE_INIT,ANGLE_INIT,
  ANGLE_INIT,ANGLE_INIT,ANGLE_INIT,ANGLE_INIT
};

// ==============================================
// 【模块4：工具函数】—— 第4个敲
// 涉及语法：函数、if、for、while、map、constrain
// 代码作用：角度转脉宽、单舵机控制、多舵机平滑移动
// ==============================================
// ------------------------------
// 函数名：angleToPulse
// 作用：把角度(0-180)转换成舵机可识别的脉宽值
// 参数：angle 要转换的角度
// 返回值：对应的脉宽数值
// ------------------------------
uint16_t angleToPulse(int angle) {
  // 限制角度在0~180之间，防止超出范围
  angle = constrain(angle, ANGLE_MIN, ANGLE_MAX);
  // map函数：将0-180映射为150-600
  return map(angle, ANGLE_MIN, ANGLE_MAX, SERVO_MIN_PULSE, SERVO_MAX_PULSE);
}

// ------------------------------
// 函数名：setServoAngle
// 作用：控制单个舵机转到指定角度
// 参数：ch 舵机通道号  angle 目标角度
// ------------------------------
void setServoAngle(uint8_t ch, int angle) {
  // 限制角度安全范围
  angle = constrain(angle, ANGLE_MIN, ANGLE_MAX);
  // 调用angleToPulse把角度转为脉宽
  uint16_t pulselen = angleToPulse(angle);
  // 给PCA9685发指令，控制舵机
  pwm.setPWM(ch, 0, pulselen);
}

// ------------------------------
// 函数名：moveServosSmooth
// 作用：8个舵机同步平滑移动，不抖动、不瞬移
// 参数：targets[8] 8个舵机的目标角度数组
// ------------------------------
void moveServosSmooth(int targets[8]) {
  // 1. 判断是否需要移动
  bool needMove = false;
  for (int i = 0; i < 8; i++) {
    targets[i] = constrain(targets[i], ANGLE_MIN, ANGLE_MAX);
    if (servoAngles[i] != targets[i]) needMove = true;
  }
  if (!needMove) return;

  // 2. 计算每个舵机移动方向：+1 或 -1
  int steps[8];
  for (int i = 0; i < 8; i++) {
    if (servoAngles[i] < targets[i]) steps[i] = 1;
    else if (servoAngles[i] > targets[i]) steps[i] = -1;
    else steps[i] = 0;
  }

  // 3. 逐度移动，直到全部到达目标
  bool moving = true;
  while (moving) {
    moving = false;
    // 更新角度
    for (int i = 0; i < 8; i++) {
      if (servoAngles[i] != targets[i]) {
        servoAngles[i] += steps[i];
        moving = true;
      }
    }
    // 输出到舵机
    for (int i = 0; i < 8; i++) {
      setServoAngle(SERVO_CHANNELS[i], servoAngles[i]);
    }
    // 延时控制速度
    delay(STEP_DELAY_MS);
  }

  // 4. 最后校准一次
  for (int i = 0; i < 8; i++) {
    setServoAngle(SERVO_CHANNELS[i], servoAngles[i]);
  }
}

// ==============================================
// 【模块5：手指控制逻辑】—— 第5个敲
// 涉及语法：函数、数组赋值、函数调用
// 代码作用：单指控制、四指控制，一正一反转动
// ==============================================
// ------------------------------
// 函数名：handleFingerAngle
// 作用：控制单根手指（2个舵机）
// 参数：fingerNum 手指编号(1-4)  angleIn 目标角度
// ------------------------------
void handleFingerAngle(int fingerNum, int angleIn) {
  // 判断手指编号是否合法
  if (fingerNum < 1 || fingerNum > 4) return;
  // 限制角度90~180
  int A = constrain(angleIn, 90, 180);
  // 复制当前角度，避免其他舵机乱动
  int targets[8];
  for (int i = 0; i < 8; i++) targets[i] = servoAngles[i];
  // 计算该手指对应的2个舵机
  int baseIdx = (fingerNum - 1) * 2;
  // 正向舵机 = 设定角度
  targets[baseIdx] = A;
  // 反向舵机 = 180 - 设定角度
  targets[baseIdx + 1] = 180 - A;
  // 调用平滑移动
  moveServosSmooth(targets);
}

// ------------------------------
// 函数名：handleAllFingers
// 作用：同时控制4根手指（8个舵机）
// 参数：angle1~4 四根手指的角度
// ------------------------------
void handleAllFingers(int angle1, int angle2, int angle3, int angle4) {
  int angles[4] = {angle1, angle2, angle3, angle4};
  int targets[8];
  // 循环给4根手指分配舵机角度
  for (int i = 0; i < 4; i++) {
    int A = constrain(angles[i], 90, 180);
    targets[i * 2] = A;
    targets[i * 2 + 1] = 180 - A;
  }
  // 平滑移动
  moveServosSmooth(targets);
}

// ==============================================
// 【模块6：串口指令解析】—— 第6个敲
// 涉及语法：String、if、字符串处理
// 代码作用：识别串口指令，调用对应控制函数
// ==============================================
// ------------------------------
// 函数名：parseCommand
// 作用：解析串口收到的指令字符串并执行
// 参数：line 串口收到的一行文本
// ------------------------------
void parseCommand(String line) {
  // 去掉空格与换行
  line.trim();
  // 空指令直接跳过
  if (line.length() == 0) return;

  // 识别 FINGER1
  if (line.startsWith("FINGER1:") || line.startsWith("finger1:")) {
    int idx = line.indexOf(':');
    int angle = line.substring(idx + 1).toInt();
    handleFingerAngle(1, angle);
    return;
  }
  // 识别 FINGER2
  if (line.startsWith("FINGER2:") || line.startsWith("finger2:")) {
    int idx = line.indexOf(':');
    int angle = line.substring(idx + 1).toInt();
    handleFingerAngle(2, angle);
    return;
  }
  // 识别 FINGER3
  if (line.startsWith("FINGER3:") || line.startsWith("finger3:")) {
    int idx = line.indexOf(':');
    int angle = line.substring(idx + 1).toInt();
    handleFingerAngle(3, angle);
    return;
  }
  // 识别 FINGER4
  if (line.startsWith("FINGER4:") || line.startsWith("finger4:")) {
    int idx = line.indexOf(':');
    int angle = line.substring(idx + 1).toInt();
    handleFingerAngle(4, angle);
    return;
  }
  // 识别 ALL
  if (line.startsWith("ALL:") || line.startsWith("all:")) {
    int idx = line.indexOf(':');
    String params = line.substring(idx + 1);
    int angles[4] = {90,90,90,90};
    int p = 0, s = 0;
    // 按逗号分割4个角度
    for (int i = 0; i <= params.length() && p < 4; i++) {
      if (i == params.length() || params.charAt(i) == ',') {
        angles[p++] = params.substring(s, i).toInt();
        s = i + 1;
      }
    }
    // 执行4指控制
    if (p == 4) {
      handleAllFingers(angles[0], angles[1], angles[2], angles[3]);
    }
  }
}

// ==============================================
// 【模块7：setup + loop】—— 第7个敲
// 涉及语法：setup()、loop()、串口、I2C
// 代码作用：开机初始化、循环等待串口指令
// ==============================================
// ------------------------------
// 函数名：setup
// 作用：开机只执行一次，初始化所有硬件
// ------------------------------
void setup() {
  // 开启串口，波特率115200
  Serial.begin(115200);
  // 初始化I2C，引脚SDA=14 SCL=15
  Wire.begin(14, 15);
  // 初始化舵机驱动
  pwm.begin();
  // 设置50Hz频率
  pwm.setPWMFreq(PWM_FREQ);
  delay(10);
  // 所有舵机回中90度
  for (int i = 0; i < 8; i++) {
    setServoAngle(SERVO_CHANNELS[i], ANGLE_INIT);
    servoAngles[i] = ANGLE_INIT;
  }
}

// ------------------------------
// 函数名：loop
// 作用：循环执行，等待串口指令
// ------------------------------
void loop() {
  // 如果串口收到数据
  if (Serial.available()) {
    // 读取一行
    String line = Serial.readStringUntil('\n');
    // 解析并执行
    parseCommand(line);
  }
}
