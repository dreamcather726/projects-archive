#define button 17
#define LED  4

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(button,INPUT_PULLDOWN);
  pinMode(LED,OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  int button_value=digitalRead(button);
  switch(button_value){
    case 0:
    digitalWrite(LED,LOW);
    break;
    case 1:
    digitalWrite(LED,HIGH);
    break;
    default:
    break;
  }
  // if(button_value==1){
  // digitalWrite(LED,HIGH);
  // }else{
  // digitalWrite(LED,LOW);
  // }
} 
