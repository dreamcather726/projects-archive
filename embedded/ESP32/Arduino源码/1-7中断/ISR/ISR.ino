const int led=13;
const int button=2;
boolean ledState=LOW;
void toggleLED(){
  ledState=!ledState;
  Serial.println("toggle");
  
}
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(led,OUTPUT);
  pinMode(button,INPUT_PULLDOWN);
  attachInterrupt(button,toggleLED,RISING);
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(led,ledState);
}
