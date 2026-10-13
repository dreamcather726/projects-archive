/*
  程序功能：计算并打印固定串口数据帧的六个字段，帮助理解帧结构和加法校验。
  使用方法：打开 115200 串口监视器，复位开发板后观察各字段和完整数据帧。
  运行结果：显示 55 05 01 FF 05 FF，以及每个字段的二进制、十进制和十六进制值。
  产品声明：本程序为青潮智科产品配套示例程序。
*/
/*
  Header  头部
  length 长度
  command 命令
  data 数据
  check 检查
  sum 求和
  tail 尾巴



*/
const byte FRAME_HEADER = 0x55;
const byte FRAME_LENGTH = 0x05;
const byte FRAME_TAIL = 0xFF;
  

// 计算“长度 + 指令 + 数据”，并保留低 8 位
byte calculateChecksum(byte command, byte data) {
  return (byte)(FRAME_LENGTH + command + data);
}

// 把一个字节显示成两位十六进制，数值小于 0x10 时先补 0
void printHexByte(byte value) {
  if (value < 0x10) {
    Serial.print("0");
  }
  Serial.print(value, HEX);
}

// 固定显示 8 位二进制，不足 8 位时在左侧补 0
void printBinaryByte(byte value) {
  for (int bitIndex = 7; bitIndex >= 0; bitIndex--) {
    Serial.print(bitRead(value, bitIndex));
  }
}

// 用二进制、十进制和十六进制显示同一个字节
void printValueInBases(byte value) {
  Serial.print("二进制 0b");
  printBinaryByte(value);
  Serial.print("，十进制 ");
  Serial.print(value, DEC);
  Serial.print("，十六进制 0x");
  printHexByte(value);
}

void setup() {
  Serial.begin(115200);

  byte command = 0x01;
  byte data = 0xFF;
  byte checksum = calculateChecksum(command, data);

  Serial.println("固定数据帧的六个字段：");

  Serial.print("HEADER   = ");
  printValueInBases(FRAME_HEADER);
  Serial.println();

  Serial.print("LENGTH   = ");
  printValueInBases(FRAME_LENGTH);
  Serial.println();

  Serial.print("COMMAND  = ");
  printValueInBases(command);
  Serial.println();

  Serial.print("DATA     = ");
  printValueInBases(data);
  Serial.println();

  Serial.print("CHECKSUM = ");
  printValueInBases(checksum);
  Serial.println();

  Serial.print("TAIL     = ");
  printValueInBases(FRAME_TAIL);
  Serial.println();

  Serial.print("完整帧：");
  printHexByte(FRAME_HEADER);
  Serial.print(" ");
  printHexByte(FRAME_LENGTH);
  Serial.print(" ");
  printHexByte(command);
  Serial.print(" ");
  printHexByte(data);
  Serial.print(" ");
  printHexByte(checksum);
  Serial.print(" ");
  printHexByte(FRAME_TAIL);
  Serial.println();
}

void loop() {
 
}
