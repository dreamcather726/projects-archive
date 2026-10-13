void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(17,INPUT_PULLDOWN);
  pinMode(4,OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  int button=digitalRead(17);
  if(button==1){
  digitalWrite(4,HIGH);
  }else{
  digitalWrite(4,LOW);
  }
} 
