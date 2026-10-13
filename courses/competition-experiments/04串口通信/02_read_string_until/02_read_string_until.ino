/*
  程序功能：使用 readStringUntil() 读取一行文字，并根据文字内容进行回复。
  使用方法：串口设为 115200 和 Newline，发送示例文字 hello、name 或 QingChao。
  运行结果：hello 回复“你好”，name 回复程序名称，其他文字会被原样显示。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

void setup() {
  Serial.begin(115200);

  // readStringUntil() 最多等待 100 毫秒，避免长时间卡住程序
  Serial.setTimeout(100);

  Serial.println("请选择串口监视器的 Newline 或 Both NL & CR：");
  Serial.println("输入 hello、name 或其他文字，然后点击发送。");
}

void loop() {
  if (Serial.available() > 0) {
    // 读取到换行符为止，返回的字符串中不包含换行符
    String message = Serial.readStringUntil('\n');

    // trim() 去掉字符串两端的空格、回车和换行
    message.trim();

    if (message.length() == 0) {
      return;
    }

    if (message == "hello") {
      Serial.println("ESP32：你好！");
    } else if (message == "name") {
      Serial.println("ESP32：我是串口实验程序。");
    } else {
      Serial.print("ESP32 收到：");
      Serial.println(message);
    }
  }
}
