int Breath_LED=4;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(Breath_LED,OUTPUT);
  Serial.print("开始呼吸灯！");
}

void loop() {
  // put your main code here, to run repeatedly:
  for(int i=0;i<255;i++){
    int light=map(i,0,255,0,100);//把i的值映射成0-100%
    Serial.print("亮度是");
    Serial.print(light);
    Serial.println("%");
    //------------------呼吸灯控制
    analogWrite(Breath_LED,i);
    delay(20);
  }
  delay(100);
  for(int i=255;i>0;i--){
    int light=map(i,0,255,0,100);//把i的值映射成0-100%
    Serial.print("亮度是");
    Serial.print(light);
    Serial.println("%");
    //------------------呼吸灯控制
    analogWrite(Breath_LED,i);
    delay(20);
  }
}
