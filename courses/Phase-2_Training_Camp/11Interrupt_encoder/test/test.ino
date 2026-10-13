#include <Ticker.h>

Ticker task;
long lastTime=0;
long lastTime1=0;
int count=0;
const int MOTOR_DIR_PIN=1;
const int MOTOR_PWM_PIN=41;
int a=4;
int b=5;
const int frequency=10000;
const int channel=0;
const int resolution=8;
int HighSpeed=80;
int LowSpeed=0;
bool flag=false;

const float d=60.0;
const float c=PI*d;
const int interrupt_times=430;
float p=c/interrupt_times;

int tasktimes=0;
bool Serialflag=false;
bool speedflag=false;
void  encodercount(){
  int Blevel=digitalRead(b);
  if(Blevel==HIGH){
    count--;
  }else {
    count++;
  }
}


void setup() {
    // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(MOTOR_DIR_PIN,OUTPUT);
  digitalWrite(MOTOR_DIR_PIN,HIGH);
  attachInterrupt(a,encodercount,RISING);
  ledcAttachChannel(MOTOR_PWM_PIN,frequency,resolution,channel);
  ledcWriteChannel(channel,0);
  task.attach(1,taskflag);
}
void loop() {
  // put your main code here, to run repeatedly:
   //记录当前的时间
   noInterrupts();
   int count1=count;
   interrupts();
   if(speedflag==true){
    // speedflag=false;
    flag=!flag;
    if(flag==false){
      ledcWriteChannel(channel,LowSpeed);
    }else {
      ledcWriteChannel(channel,HighSpeed);
    }
   }
   if(serialtimes==true){
    Serial.print("前进的距离:");
    float distance=count*p/1000;
    Serial.print(distance,2);
    Serial.println("m");      
   }
}//






void taskflag(){
  speedtimes++;
  serialtimes++;
  if(speedtimes>=10){
    speedtimes=0;
    speedflag=true;
  }
  if(serialtimes>20){
    serialtimes=0;
    Serialflag=true;
  }
}

