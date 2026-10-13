/*
  程序功能：逐个读取电脑发送的串口字符，并显示字符及其 ASCII 数值。
  使用方法：打开 115200 串口监视器，发送示例字符 A，也可以发送数字或回车换行。
  运行结果：发送 A 时显示字符 A 和 ASCII 数值 65，同时显示 available、peek、read 结果。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

void setup() {
  Serial.begin(115200);
  Serial.println("请输入任意字符：");
}

void loop() {
  // available() 返回接收缓冲区中还没有读取的字节数
  if (Serial.available() > 0) {
    int byteCount = Serial.available();

    // peek() 只查看下一个字节，不把它从缓冲区取走
    int nextByte = Serial.peek();

    // read() 读取一个字节，并把它从缓冲区取走
    int receivedByte = Serial.read();

    Serial.print("待读字节数：");
    Serial.print(byteCount);
    Serial.print("，peek：");
    Serial.print(nextByte);
    Serial.print("，read：");
    Serial.print(receivedByte);
    Serial.print("，字符：");

    if (receivedByte == '\r') {
      Serial.println("[回车]");
    } else if (receivedByte == '\n') {
      Serial.println("[换行]");
    } else {
      // write() 直接发送这个字节本身
      Serial.write(receivedByte);
      Serial.println();
    }
  }
}
