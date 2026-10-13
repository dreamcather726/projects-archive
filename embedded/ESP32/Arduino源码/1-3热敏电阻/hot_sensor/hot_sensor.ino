void setup() {
  // put yo ur setup code here, to run once:
  Serial.begin(115200);
  Serial.println("初始化成功！");

}

void loop() {
  // put your main code here, to run repeatedly:
  int Sensor_Value=analogRead(4);
  Serial.println(Sensor_Value);
  delay(1000);
}
