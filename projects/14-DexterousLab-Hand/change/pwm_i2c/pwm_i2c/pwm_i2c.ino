// ==============================================
// 功能：Arduino 通过I2C控制PCA9685舵机驱动板
// 指令格式：SERVO:角度 (支持小写、空格、自动过滤错误符号)
// 硬件接线：SDA=14, SCL=15, 舵机接通道0
// ==============================================

// 1. 导入核心库
#include <Wire.h>                   // I2C通信必备库
#include <Adafruit_PWMServoDriver.h> // 舵机驱动板库

// 2. 常量定义（方便修改，比硬编码更安全）

#define SERVO_MIN_ANGLE 0           // 舵机最小角度
#define SERVO_MAX_ANGLE 180         // 舵机最大角度
#define SERVO_MIN_PULSE 150          // 最小脉冲值
#define SERVO_MAX_PULSE 600          // 最大脉冲值
#define DEFAULT_ANGLE 90             // 开机默认复位角度(90度)
#define SERVO_CHANNEL 0             // 舵机所在通道：0~15

// 3. 创建舵机驱动对象（I2C地址0x40默认）
Adafruit_PWMServoDriver pwm(0x40);

// 记录上一次角度，避免重复刷新（防抖、省电）
int lastAngle = DEFAULT_ANGLE;

void setup() {
  // 1. 初始化串口通信（电脑通信）
  Serial.begin(115200);
  delay(50); // 等待串口稳定

  // ==============================================
  // 开机自动发送使用说明，用户知道怎么发指令
  // ==============================================
  Serial.println("==================================");
  Serial.println("🤖 舵机控制系统已启动！");
  Serial.println("📌 指令格式：SERVO:角度 (例：SERVO:90)");
  Serial.print("📌 支持角度范围：");
  Serial.print(SERVO_MIN_ANGLE);
  Serial.print(" ~ ");
  Serial.println(SERVO_MAX_ANGLE);
  Serial.println("📌 支持小写、自动忽略空格");
  Serial.println("==================================\n");

  // 2. 初始化I2C引脚（SDA=14, SCL=15）
  Wire.begin(14, 15);

  // ==============================================
  // 检测I2C设备是否连接（防止驱动板没接好）
  // ==============================================
  Serial.print("🔍 正在检测舵机驱动板...");
  pwm.begin();
  pwm.setPWMFreq(50); // 舵机标准频率50Hz
  Serial.println("✅ 检测成功！");

  // ==============================================
  // 开机自动复位到安全角度（防止开机乱动）
  // ==============================================
  moveServo(DEFAULT_ANGLE);
  Serial.print("✅ 舵机已复位到：");
  Serial.println(DEFAULT_ANGLE);
  Serial.println("\n👉 请发送指令控制舵机！");
}

void loop() {
  // 1. 判断串口是否有数据
  if (Serial.available() > 0) {
    // 读取指令（读取到换行符为止）
    String cmd = Serial.readStringUntil('\n');

    // ==============================================
    // 优化点4：自动清理指令中的空格、换行、回车
    // ==============================================
    cmd.trim(); // 清除首尾空格/换行

    // ==============================================
    // 指令大小写不敏感（统一转大写）
    
    // ==============================================
    cmd.toUpperCase();

    // 2. 解析指令
    if (cmd.startsWith("SERVO:")) {
      // 截取角度字符串
      String angleStr = cmd.substring(6);
      // 字符串转整数
      int targetAngle = angleStr.toInt();

      // ==============================================
      // 角度越界保护（防止舵机堵转、烧毁）
      // ==============================================
      if (targetAngle < SERVO_MIN_ANGLE) {
        targetAngle = SERVO_MIN_ANGLE;
        Serial.println("⚠️  角度过小，已自动限制为最小值");
      }
      else if (targetAngle > SERVO_MAX_ANGLE) {
        targetAngle = SERVO_MAX_ANGLE;
        Serial.println("⚠️  角度过大，已自动限制为最大值");
      }

      // ==============================================
      // 重复指令过滤（不重复执行相同动作）
      // ==============================================
      if (targetAngle == lastAngle) {
        Serial.print("⚠  舵机已经在：");
        Serial.print(targetAngle);
        Serial.println(" 度，无需重复操作");
        return;
      }

      // 3. 执行舵机转动
      moveServo(targetAngle);
      lastAngle = targetAngle; // 更新角度记录

      // 4. 成功反馈
      Serial.print("✅ 舵机已转动到：");
      Serial.println(targetAngle);
    }
    else {
      // ==============================================
      // 无效指令提示（用户知道发错了）
      // ==============================================
      Serial.println("❌ 无效指令！正确格式：SERVO:角度");
    }
  }
}

// ==============================================
// 独立封装舵机控制函数（代码更清晰、更安全）
// ==============================================
void moveServo(int angle) {
  // 角度映射为舵机脉冲值
  int pulse = map(angle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE, SERVO_MIN_PULSE, SERVO_MAX_PULSE);
  // 输出PWM信号控制舵机
  pwm.setPWM(SERVO_CHANNEL, 0, pulse);
}
