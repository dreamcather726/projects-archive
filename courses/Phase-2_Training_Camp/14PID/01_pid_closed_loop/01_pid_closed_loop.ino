/*
  程序功能：用软件模拟对象演示 PID 闭环控制，不连接电机也能观察目标值跟随过程。
  使用方法：打开 115200 串口监视器或绘图器；可发送 target 80、target 30 改变目标。
  运行结果：Input 逐渐接近 Target，串口持续显示 Target、Input、Output 三条数据。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include "speed_pid.h"

float targetValue = 80;
float processValue = 0;
unsigned long lastControlTime = 0;

// 创建一组PID数据
PID demoPid;

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(100);

  // 参数顺序：PID、P、I、D、输出最大值、输出最小值
  PID_Init(&demoPid, 5, 0.5, 0.1, 2000, -20000);
  PID_Clear(&demoPid);
  lastControlTime = millis();

  Serial.println("输入 target 80 修改目标值");
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.startsWith("target ")) {
      targetValue = command.substring(7).toFloat();
      targetValue = constrain(targetValue, 0, 100);
      PID_Clear(&demoPid);
    }
  }

  unsigned long currentTime = millis();

  if (currentTime - lastControlTime >= 100) {
    float output = PID_IncPIDCal(&demoPid, processValue, targetValue);

    // 用简单公式模拟真实对象响应：输出越大，过程值上升越快
    processValue = processValue + (output - processValue) * 0.08;

    Serial.print("Target:");
    Serial.print(targetValue);
    Serial.print(",Input:");
    Serial.print(processValue);
    Serial.print(",Output:");
    Serial.println(output);

    lastControlTime = currentTime;
  }
}
