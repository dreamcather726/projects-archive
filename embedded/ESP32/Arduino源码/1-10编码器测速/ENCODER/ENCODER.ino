int encoder_pinA=5;
int encoder_pinB=6;
volatile int count=0;
long lasttime=0;
float speed_cm_s=0.0;
long last_count=0;
const int WHEEL_TOTAL_PULSES=430;
const float WHEEL_C_CM=PI*6;//60mm==6cm
float PRE_PULSE_C=WHEEL_C_CM/WHEEL_TOTAL_PULSES;//一个中断的长度是PRE_PULSE_C
float distance=0.0f;
void encoder_count(){
  int BLEVEL=digitalRead(encoder_pinB);
  if (BLEVEL==1){
     count++;
  }else{
     count--;
  }
}

void setup() {
  // put your setup code here, to run once:
  
  pinMode(encoder_pinA,INPUT_PULLUP);
  pinMode(encoder_pinB,INPUT_PULLUP);//设置成上拉模式
  Serial.begin(115200);
  Serial.println("***编码器计数***");
  attachInterrupt(encoder_pinA,encoder_count,RISING);
}

void loop() {
  long now=millis();
  if(now-lasttime>=1000){
    lasttime=now;
    noInterrupts();
    int temp_count=count;
    interrupts();

    distance=temp_count*PRE_PULSE_C;
    long delta_times=temp_count-last_count;
    speed_cm_s=delta_times*PRE_PULSE_C;
    Serial.print("脉冲次数：");
    Serial.print(temp_count);
    Serial.print("     行驶距离：");
    Serial.print(distance);
    Serial.print("cm");
    Serial.print("     行驶速度：");
    Serial.print(speed_cm_s);
    Serial.println("cm/s");
    last_count=temp_count;
  }
  

}
