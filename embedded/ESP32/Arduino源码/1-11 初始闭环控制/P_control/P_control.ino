// P控制 电机速度模拟 简化教学版
float target = 80.0;    // 目标速度
float feedback = 0.0;   // 实际反馈速度（对应编码器读到的值）
float Kp = 1.0;         // 比例系数
float out = 0.0;        // 控制器输出

unsigned long lastT = 0;
const int dt = 100;     // 控制周期 100ms

void setup()
{
  Serial.begin(115200);
  Serial.println("P控制模拟：target xx / kp xx");
}

void loop()
{
  // 串口命令解析
  if(Serial.available()>0)
  {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    
    if(cmd.startsWith("target ")){
      target = cmd.substring(7).toFloat();
      target = constrain(target,0,100);
      Serial.print("目标速度 = ");Serial.println(target);
    }
    if(cmd.startsWith("kp ")){
      Kp = cmd.substring(3).toFloat();
      Serial.print("Kp = ");Serial.println(Kp);
    }
  }

  // 定时执行P控制
  unsigned long now = millis();
  if(now - lastT >= dt)
  {
    // 1. 误差 = 目标 - 实际
    float err = target - feedback;
    
    // 2. P控制公式：输出 = Kp * 误差
    out = Kp * err;

    // 3. 模拟电机：输出越大，速度上升越快
    feedback = feedback + out * 0.05;
    feedback = constrain(feedback, 0, 100);

    // 串口绘图输出 Target,Input,Output
  
    Serial.print(target);
    Serial.print(",");
    Serial.print(feedback);
    Serial.print(",");
    Serial.println(out);

    lastT = now;
  }
}
