const int PWMA=40;
const int AIN1=41;
const int AIN2=42;
const int Button=5;
int speedPWM=0;
int lastbutton=0;
void setup() {
  Serial.begin(115200);
  delay(1000);
  // put your setup code here, to run once:
  pinMode(PWMA,OUTPUT);
  pinMode(AIN1,OUTPUT);
  pinMode(AIN2,OUTPUT);
  pinMode(Button,INPUT_PULLUP);
  Serial.println("电机驱动！！");
}

void loop() {
  // put your main code here, to run repeatedly:
  
  int button_value=button_prase();
  if(button_value!=lastbutton){
  Serial.print("Button==");
  Serial.println(button_value);
  lastbutton=button_value;
  }
 
  if(button_value==1){
    speedPWM=speedPWM+10;
    SetSpeed(speedPWM);
    Serial.println(speedPWM);
  }
  // SetSpeed(200);
  // delay(5000);
  // SetSpeed(-200);
  // delay(5000);
}

void SetSpeed(int speed){
  if(speed>255) {speed=0; speedPWM=0;}
  if(speed<-255) {speed=0; speedPWM=0;}
  if(speed>0){
  digitalWrite(AIN1,HIGH);
  digitalWrite(AIN2,LOW);
  analogWrite(PWMA,speed);
  }else {
  digitalWrite(AIN1,LOW);
  digitalWrite(AIN2,HIGH);
  analogWrite(PWMA,abs(speed));
  }

}

int button_prase(void){
  int button=digitalRead(Button);
  if(button==0)//按钮按下
  {
    delay(20);
    button=digitalRead(Button);
    if(button==0){
      while(digitalRead(Button)==0);
      Serial.print("按下");
      delay(20);
      return 1;
    }
    
  }
  return 0;
}