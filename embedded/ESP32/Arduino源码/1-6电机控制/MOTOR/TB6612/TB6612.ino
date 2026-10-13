const int PWMA=40;
const int AIN1=41;
const int AIN2=42;
const int speedPWM=128;
void setup() {
  Serial.begin(115200);
  delay(1000);
  // put your setup code here, to run once:
  pinMode(PWMA,OUTPUT);
  pinMode(AIN1,OUTPUT);
  pinMode(AIN2,OUTPUT);
  Serial.println("电机驱动！！");
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(AIN1,HIGH);
  digitalWrite(AIN2,LOW);
  analogWrite(PWMA,speedPWM);
  delay(2000);

  analogWrite(PWMA,0);
  delay(2000);

  digitalWrite(AIN1,LOW);
  digitalWrite(AIN2,HIGH);
  analogWrite(PWMA,speedPWM);
  delay(2000);
}
