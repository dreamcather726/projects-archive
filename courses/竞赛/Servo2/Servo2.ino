#include <ESP32Servo.h>

Servo servoBase;
Servo servoArm;
Servo servoGrip;

#define PIN_BASE 19
#define PIN_ARM 20
#define PIN_GRIP 21

void setup()
{
  Serial.begin(115200);
  Serial.setTimeout(100);

  servoBase.attach(PIN_BASE);
  servoArm.attach(PIN_ARM);
  servoGrip.attach(PIN_GRIP);

  Serial.println("输入：b90、a90 或 g90");
}

void loop()
{
  if(Serial.available() == 0) return;

  String command = Serial.readStringUntil('\n');
  command.trim();//去掉前后空格
  if(command.length() < 2) return;//命令长度至少为2

  char name = command.charAt(0);//获取舵机名称
  int angle = constrain(command.substring(1).toInt(), 0, 180);//获取舵机角度并限制在0~180之间

  if(name == 'b') servoBase.write(angle);
  if(name == 'a') servoArm.write(angle);
  if(name == 'g') servoGrip.write(angle);

  Serial.println(command);
}
