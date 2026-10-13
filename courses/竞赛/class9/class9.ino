HardwareSerial softSerial(1);
int SOFT_BAUD=115200;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  softSerial.begin(SOFT_BAUD,SERIAL_8N1,18,17);
  Serial.print("HEllO ESP32!!!!");
}

void loop() {
  // put your main code here, to run repeatedly:
 
 

}

void handUSBSerialInput(){

   while(Serial.available()>0){
  //  char receiveChar=Serial.read();
  char receiveChar=Serial.read();
   Serial.print("电脑串口接收到的数据是：");
   Serial.println(receiveChar);
   softSerial.print("from USB");
   softSerial.print("receiveChar");
  }
}

void handsoftSerialInput(){
 while(softSerial.available()>0){
    char receiveChar=softSerial.read();
    Serial.print("软件串口收到：")
    Serial.println(receiveChar);
    // softSerial.print("from USB");
    // softSerial.print("receiveChar");
  }
}
