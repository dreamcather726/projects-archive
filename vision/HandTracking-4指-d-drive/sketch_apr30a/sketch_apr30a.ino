
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>


#define FAST_MODE 1          // 1=极速响应模式（无平滑），0=平滑模式
#define PCA9685_ADDR 0x40    // PCA9685 I2C地址
#define PWM_FREQ 50          // 舵机标准50Hz
#define ANGLE_MIN 0
#define ANGLE_MAX 180
#define ANGLE_INIT 90
#define STEP_DELAY_MS 2      // 平滑模式步长延时

typedef struct {
  uint8_t ch_pos;   // 正向舵机通道
  uint8_t ch_neg;   // 反向舵机通道
  int8_t offset_pos; // 正向舵机校准偏移
  int8_t offset_neg; // 反向舵机校准偏移
} FingerConfig;

FingerConfig finger[4] = {
  {0, 1, 0, 0},  // 食指：通道0(正向)、通道1(反向)
  {2, 3, 0, 0},  // 中指：通道2(正向)、通道3(反向)
  {4, 5, 0, 0},  // 无名指：通道4(正向)、通道5(反向)
  {6, 7, 0, 0}   // 大拇指：通道6(正向)、通道7(反向)
};


Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(PCA9685_ADDR);
int current_finger_angles[4] = {ANGLE_INIT, ANGLE_INIT, ANGLE_INIT, ANGLE_INIT};


bool waveMode = false;
int waveStep = 0;
unsigned long lastWaveTime = 0;
const unsigned long waveInterval = 100;



uint16_t angleToPulse(int angle) {
  angle = constrain(angle, ANGLE_MIN, ANGLE_MAX);
  return map(angle, ANGLE_MIN, ANGLE_MAX, 150, 600);
}

// 【关键修复】直接用Wire库实现PCA9685单手指双电机同步写入
// 替代writeMultiple，100%可靠，时序差<10μs
void writeFingerSync(uint8_t finger_idx, uint16_t pulse_pos, uint16_t pulse_neg) {
  if (finger_idx >= 4) return;
  
  // 计算起始寄存器地址：PCA9685每个通道占4个寄存器
  // 寄存器0x06 + 通道号*4
  uint8_t start_reg = 0x06 + finger[finger_idx].ch_pos * 4;
  
  // 发起I2C传输，一次性写入2个通道的8个字节
  Wire.beginTransmission(PCA9685_ADDR);
  Wire.write(start_reg);
  // 正向通道：ON_L=0, ON_H=0, OFF_L, OFF_H
  Wire.write((uint8_t)0x00);
  Wire.write((uint8_t)0x00);
  Wire.write(lowByte(pulse_pos));
  Wire.write(highByte(pulse_pos));
  // 反向通道：ON_L=0, ON_H=0, OFF_L, OFF_H
  Wire.write((uint8_t)0x00);
  Wire.write((uint8_t)0x00);
  Wire.write(lowByte(pulse_neg));
  Wire.write(highByte(pulse_neg));
  Wire.endTransmission();
}

// ========== 【强制同步】手指控制函数 ==========
void setFingerSync(uint8_t finger_idx, int angle) {
  if (finger_idx >= 4) return;
  
  // 强制绑定正反向角度
  int angle_pos = constrain(angle, 90, 180);
  int angle_neg = 180 - angle_pos;

  // 加上校准偏移
  angle_pos += finger[finger_idx].offset_pos;
  angle_neg += finger[finger_idx].offset_neg;

  // 计算PWM值
  uint16_t pulse_pos = angleToPulse(angle_pos);
  uint16_t pulse_neg = angleToPulse(angle_neg);

  // 【核心】直接用Wire库同步写入双电机
  writeFingerSync(finger_idx, pulse_pos, pulse_neg);

  // 更新当前角度
  current_finger_angles[finger_idx] = constrain(angle, 90, 180);
}

// 4根手指全同步
void setAllFingerSync(int angles[4]) {
  for (uint8_t i = 0; i < 4; i++) {
    int angle_pos = constrain(angles[i], 90, 180);
    int angle_neg = 180 - angle_pos;
    angle_pos += finger[i].offset_pos;
    angle_neg += finger[i].offset_neg;

    uint16_t pulse_pos = angleToPulse(angle_pos);
    uint16_t pulse_neg = angleToPulse(angle_neg);

    writeFingerSync(i, pulse_pos, pulse_neg);
    current_finger_angles[i] = constrain(angles[i], 90, 180);
  }
}

// ========== 平滑模式 ==========
void moveFingerSmooth(uint8_t finger_idx, int target_angle) {
#if FAST_MODE == 0
  if (finger_idx >= 4) return;
  target_angle = constrain(target_angle, 90, 180);
  int start_angle = current_finger_angles[finger_idx];
  
  if (start_angle == target_angle) return;
  
  int step = (target_angle > start_angle) ? 1 : -1;
  int current = start_angle;

  while (current != target_angle) {
    current += step;
    setFingerSync(finger_idx, current);
    delay(STEP_DELAY_MS);
  }
#endif
}

void moveAllFingerSmooth(int target_angles[4]) {
#if FAST_MODE == 0
  bool need_move = false;
  int steps[4];
  for (int i = 0; i < 4; i++) {
    target_angles[i] = constrain(target_angles[i], 90, 180);
    if (current_finger_angles[i] != target_angles[i]) need_move = true;
    steps[i] = (target_angles[i] > current_finger_angles[i]) ? 1 : -1;
  }
  if (!need_move) return;

  bool moving = true;
  while (moving) {
    moving = false;
    int current_angles[4];
    for (int i = 0; i < 4; i++) {
      if (current_finger_angles[i] != target_angles[i]) {
        current_angles[i] = current_finger_angles[i] + steps[i];
        moving = true;
      } else {
        current_angles[i] = current_finger_angles[i];
      }
    }
    setAllFingerSync(current_angles);
    delay(STEP_DELAY_MS);
  }
#endif
}

// ========== 波浪模式 ==========
void waveLoop() {
  if (!waveMode) return;
  unsigned long now = millis();
  if (now - lastWaveTime < waveInterval) return;
  lastWaveTime = now;

  int wave_angles[4] = {90, 90, 90, 90};
  switch (waveStep) {
    case 0: wave_angles[0] = 180; break;
    case 1: wave_angles[0] = 180; wave_angles[1] = 180; break;
    case 2: wave_angles[0] = 180; wave_angles[1] = 180; wave_angles[2] = 180; break;
    case 3: wave_angles[0] = 180; wave_angles[1] = 180; wave_angles[2] = 180; wave_angles[3] = 180; break;
    case 4: wave_angles[0] = 90; wave_angles[1] = 180; wave_angles[2] = 180; wave_angles[3] = 180; break;
    case 5: wave_angles[0] = 90; wave_angles[1] = 90; wave_angles[2] = 180; wave_angles[3] = 180; break;
    case 6: wave_angles[0] = 90; wave_angles[1] = 90; wave_angles[2] = 90; wave_angles[3] = 180; break;
    case 7: wave_angles[0] = 90; wave_angles[1] = 90; wave_angles[2] = 90; wave_angles[3] = 90; break;
  }
  setAllFingerSync(wave_angles);
  waveStep = (waveStep + 1) % 8;
}

// ========== 串口指令解析 ==========
void parseCommand(String line) {
  line.trim();
  if (line.length() == 0) return;

  // 单手指控制（强制同步）
  if (line.startsWith("FINGER1:") || line.startsWith("finger1:")) {
    int angle = line.substring(line.indexOf(':') + 1).toInt();
    #if FAST_MODE == 1
    setFingerSync(0, angle);
    #else
    moveFingerSmooth(0, angle);
    #endif
    Serial.print("[FINGER1] 同步角度="); Serial.println(angle);
    return;
  }
  if (line.startsWith("FINGER2:") || line.startsWith("finger2:")) {
    int angle = line.substring(line.indexOf(':') + 1).toInt();
    #if FAST_MODE == 1
    setFingerSync(1, angle);
    #else
    moveFingerSmooth(1, angle);
    #endif
    Serial.print("[FINGER2] 同步角度="); Serial.println(angle);
    return;
  }
  if (line.startsWith("FINGER3:") || line.startsWith("finger3:")) {
    int angle = line.substring(line.indexOf(':') + 1).toInt();
    #if FAST_MODE == 1
    setFingerSync(2, angle);
    #else
    moveFingerSmooth(2, angle);
    #endif
    Serial.print("[FINGER3] 同步角度="); Serial.println(angle);
    return;
  }
  if (line.startsWith("FINGER4:") || line.startsWith("finger4:")) {
    int angle = line.substring(line.indexOf(':') + 1).toInt();
    #if FAST_MODE == 1
    setFingerSync(3, angle);
    #else
    moveFingerSmooth(3, angle);
    #endif
    Serial.print("[FINGER4] 同步角度="); Serial.println(angle);
    return;
  }

  // 全手指控制（强制同步）
  if (line.startsWith("ALL:") || line.startsWith("all:")) {
    String data = line.substring(4);
    int comma1 = data.indexOf(',');
    int comma2 = data.indexOf(',', comma1 + 1);
    int comma3 = data.indexOf(',', comma2 + 1);
    if (comma1 == -1 || comma2 == -1 || comma3 == -1) {
      Serial.println("[错误] ALL格式错误，正确：ALL:a1,a2,a3,a4");
      return;
    }
    int angles[4] = {
      data.substring(0, comma1).toInt(),
      data.substring(comma1 + 1, comma2).toInt(),
      data.substring(comma2 + 1, comma3).toInt(),
      data.substring(comma3 + 1).toInt()
    };
    #if FAST_MODE == 1
    setAllFingerSync(angles);
    #else
    moveAllFingerSmooth(angles);
    #endif
    Serial.print("[ALL] 同步角度=[");
    Serial.print(angles[0]); Serial.print(",");
    Serial.print(angles[1]); Serial.print(",");
    Serial.print(angles[2]); Serial.print(",");
    Serial.print(angles[3]); Serial.println("]");
    return;
  }

  // 复位指令
  if (line.equals("RESET") || line.equals("reset")) {
    Serial.println("[RESET] 全手指同步复位");
    waveMode = false;
    int reset_angles[4] = {ANGLE_INIT, ANGLE_INIT, ANGLE_INIT, ANGLE_INIT};
    setAllFingerSync(reset_angles);
    return;
  }

  // 波浪模式
  if (line.equals("WAVEON") || line.equals("waveon")) {
    Serial.println("[WAVE] 开启同步波浪模式");
    waveMode = true;
    waveStep = 0;
    return;
  }
  if (line.equals("WAVEOFF") || line.equals("waveoff")) {
    Serial.println("[WAVE] 关闭波浪模式");
    waveMode = false;
    int reset_angles[4] = {ANGLE_INIT, ANGLE_INIT, ANGLE_INIT, ANGLE_INIT};
    setAllFingerSync(reset_angles);
    return;
  }
}

// ========== 初始化 ==========
void setup() {
  Serial.begin(115200);
  Wire.begin(14, 15);  // ESP32-S3 I2C引脚，其他ESP32改为Wire.begin()
  pwm.begin();
  pwm.setPWMFreq(PWM_FREQ);
  delay(500);

  Serial.println("[初始化] 完成，强制同步模式已开启");
  int reset_angles[4] = {ANGLE_INIT, ANGLE_INIT, ANGLE_INIT, ANGLE_INIT};
  setAllFingerSync(reset_angles);
}

// ========== 主循环 ==========
void loop() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    parseCommand(line);
  }
  waveLoop();
  delay(5);
}