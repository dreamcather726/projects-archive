void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.print("循环开始！");
}

void loop() {
  // put your main code here, to run repeatedly:
//   for (int i = 0; i < 10; i++)
//  { 
//    Serial.println(i); 
//    delay(1000);
//   }
  int i=0;
  while(i<10){
    Serial.print(i);
    i=i+2;
    delay(1000);
  }
}
