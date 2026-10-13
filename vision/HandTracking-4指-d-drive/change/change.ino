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

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#define PCA9685_ADDR    0x40
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(PCA9685_ADDR);

#define PWM_FREQ        50
#define SERVO_MIN_PULSE 150
#define SERVO_MAX_PULSE 600

const uint8_t SERVO_CHANNELS[8] = {0, 1, 2, 3, 4, 5, 6, 7};

const int ANGLE_MIN = 0;
const int ANGLE_MAX = 180;
const int ANGLE_INIT = 90;
const uint16_t STEP_DELAY_MS = 2;

int servoAngles[8] = {ANGLE_INIT, ANGLE_INIT, ANGLE_INIT, ANGLE_INIT,
                      ANGLE_INIT, ANGLE_INIT, ANGLE_INIT, ANGLE_INIT};

uint16_t angleToPulse(int angle) {
  angle = constrain(angle, ANGLE_MIN, ANGLE_MAX);
  return map(angle, ANGLE_MIN, ANGLE_MAX, SERVO_MIN_PULSE, SERVO_MAX_PULSE);
}

void setServoAngle(uint8_t ch, int angle) {
  angle = constrain(angle, ANGLE_MIN, ANGLE_MAX);
  uint16_t pulselen = angleToPulse(angle);
  pwm.setPWM(ch, 0, pulselen);
}

void moveServosSmooth(int targets[8]) {
  bool needMove = false;
  for (int i = 0; i < 8; i++) {
    targets[i] = constrain(targets[i], ANGLE_MIN, ANGLE_MAX);
    if (servoAngles[i] != targets[i]) {
      needMove = true;
    }
  }
  if (!needMove) {
    return;
  }

  int steps[8];
  for (int i = 0; i < 8; i++) {
    if (servoAngles[i] < targets[i]) {
      steps[i] = 1;
    } else if (servoAngles[i] > targets[i]) {
      steps[i] = -1;
    } else {
      steps[i] = 0;
    }
  }

  bool moving = true;
  while (moving) {
    moving = false;
    for (int i = 0; i < 8; i++) {
      if (servoAngles[i] != targets[i]) {
        servoAngles[i] += steps[i];
        moving = true;
      }
    }

    for (int i = 0; i < 8; i++) {
      setServoAngle(SERVO_CHANNELS[i], servoAngles[i]);
    }

    delay(STEP_DELAY_MS);
  }

  for (int i = 0; i < 8; i++) {
    setServoAngle(SERVO_CHANNELS[i], servoAngles[i]);
  }
}

void handleFingerAngle(int fingerNum, int angleIn) {
  if (fingerNum < 1 || fingerNum > 4) {
    Serial.println("[FINGER] 手指编号无效，必须是1-4");
    return;
  }

  int A = constrain(angleIn, 90, 180);

  int targets[8];
  for (int i = 0; i < 8; i++) {
    targets[i] = servoAngles[i];
  }

  int baseIdx = (fingerNum - 1) * 2;
  targets[baseIdx] = A;
  targets[baseIdx + 1] = 180 - A;

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

  moveServosSmooth(targets);
}

// 同时控制所有4个手指
void handleAllFingers(int angle1, int angle2, int angle3, int angle4) {
  int angles[4] = {angle1, angle2, angle3, angle4};
  
  int targets[8];
  for (int i = 0; i < 4; i++) {
    int A = constrain(angles[i], 90, 180);
    targets[i * 2] = A;
    targets[i * 2 + 1] = 180 - A;
  }

  Serial.print("[ALL] 手指角度=[");
  Serial.print(angles[0]);
  Serial.print(", ");
  Serial.print(angles[1]);
  Serial.print(", ");
  Serial.print(angles[2]);
  Serial.print(", ");
  Serial.print(angles[3]);
  Serial.println("]");

  moveServosSmooth(targets);
}

void parseCommand(String line) {
  line.trim();
  if (line.length() == 0) return;

  Serial.print("[串口接收] ");
  Serial.println(line);

  if (line.startsWith("FINGER1:") || line.startsWith("finger1:")) {
    int colonIndex = line.indexOf(':');
    String angleStr = line.substring(colonIndex + 1);
    int angle = angleStr.toInt();
    if (angle == 0 && !angleStr.startsWith("0")) {
      Serial.println("[错误] FINGER1 指令角度解析失败");
      return;
    }
    handleFingerAngle(1, angle);
    return;
  }

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

  // -------- ALL:角度1,角度2,角度3,角度4 --------
  if (line.startsWith("ALL:") || line.startsWith("all:")) {
    int colonIndex = line.indexOf(':');
    String params = line.substring(colonIndex + 1);
    
    // 解析4个角度值
    int angles[4] = {90, 90, 90, 90};
    int paramIndex = 0;
    int startIdx = 0;
    
    for (int i = 0; i <= params.length() && paramIndex < 4; i++) {
      if (i == params.length() || params.charAt(i) == ',') {
        String angleStr = params.substring(startIdx, i);
        angleStr.trim();
        int angle = angleStr.toInt();
        if (angleStr.length() > 0 || (angle == 0 && angleStr.startsWith("0"))) {
          angles[paramIndex] = angle;
          paramIndex++;
        }
        startIdx = i + 1;
      }
    }
    
    if (paramIndex == 4) {
      handleAllFingers(angles[0], angles[1], angles[2], angles[3]);
    } else {
      Serial.println("[错误] ALL指令需要4个角度值，格式: ALL:90,90,90,90");
    }
    return;
  }

  Serial.println("[提示] 未识别的指令（可用: FINGER1-4:角度 或 ALL:角度1,角度2,角度3,角度4）");
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("=== hand_arduino_v1_servo1-8.ino ===");

  Wire.begin(14, 15);

  pwm.begin();
  pwm.setPWMFreq(PWM_FREQ);

  delay(10);

  for (int i = 0; i < 8; i++) {
    setServoAngle(SERVO_CHANNELS[i], ANGLE_INIT);
    servoAngles[i] = ANGLE_INIT;
  }

  Serial.println("[初始化] 所有 8 个舵机已复位到 90°");
  Serial.println("[提示] FINGER1-4:角度 - 单独控制手指");
  Serial.println("[提示] ALL:角度1,角度2,角度3,角度4 - 同时控制4个手指");
}

void loop() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    parseCommand(line);
  }
}
