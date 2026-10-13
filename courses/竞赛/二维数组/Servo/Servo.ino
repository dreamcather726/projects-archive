#include <ESP32Servo.h>

const int SERVO_NUM = 3;
Servo servo[SERVO_NUM];

// 三个舵机的信号引脚
int servoPin[SERVO_NUM] = {13, 14, 27};

// 每一行是一条命令，分别表示三个舵机的角度
int command[][SERVO_NUM] = {
  {90, 90, 90},
  {30, 90, 150},
  {60, 120, 60},
  {150, 90, 30},
  {90, 90, 90}
};

// 自动计算二维数组中有多少条命令
const int COMMAND_NUM = sizeof(command) / sizeof(command[0]);

// 发布二维数组中的一条命令
void sendCommand(int row) {
  for (int i = 0; i < SERVO_NUM; i++) {
    servo[i].write(command[row][i]);
  }
}

void setup() {
  for (int i = 0; i < SERVO_NUM; i++) {
    servo[i].attach(servoPin[i]);
  }
}

void loop() {
  for (int row = 0; row < COMMAND_NUM; row++) {
    sendCommand(row);
    delay(1000);
  }
}
