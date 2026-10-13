void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(17,INPUT)
}

void loop() {
  // put your main code here, to run repeatedly:
  int a=analogRead(17);
  Serial.print(a);
  delay(1000);
} 
