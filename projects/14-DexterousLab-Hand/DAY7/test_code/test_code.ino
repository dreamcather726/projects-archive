//导入核心库
#include <Wire.h> //I2C通信必备库
#include <Adafruit_PWMServoDriver.h> //舵机驱动必备库

//定义常量
#define SERVO_MIN_ANGLE 0     //舵机的最小角度
#define SERVO_MAX_ANGLE 180   //舵机的最大角度
#define SERVO_MIN_PULSE 150   //最小脉冲
#define SERVO_MAX_PULSE 600   //最大脉冲
#define DEFAULT_ANGLE 90      //舵机初始角度
#define SERVO_CHANNEL 0       //舵机所在通道

Adafruit_PWMServoDriver pwm(0x40);


int lastAngle = DEFAULT_ANGLE; //记录上一次角度

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(50);  //  为了保证串口稳定

  //esp32开机正常自动发送信息
  Serial.println("==========================");
  Serial.println("舵机控制系统已启动");
  Serial.println("指令格式：SERVO:角度(例如 SERVO:90)");
  Serial.println("支持角度范围:");
  Serial.println(SERVO_MIN_ANGLE);
  Serial.println("~");
  Serial.println(SERVO_MAX_ANGLE);

  //初始化I2C(SDA=14,SCL=15)
  Wire.begin(14, 15);

  Serial.println("正在检查舵机驱动板");
  pwm.begin();
  pwm.setPWMFreq(50);
  Serial.println("检测成功！");

  moveServo(DEFAULT_ANGLE);
  Serial.print("舵机复位成功，复位位置:");
  Serial.println(DEFAULT_ANGLE);


  

}

void loop() {
  // put your main code here, to run repeatedly:

}

void moveServo(int angle)
{
  //角度映射为脉冲宽度
  int pulse = map(angle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE, SERVO_MIN_PULSE, SERVO_MAX_PULSE);
  //输出PWM信号控制舵机
  pwm.setPWM(SERVO_CHANNEL, 0, pulse);

}
