#include "Arduino.h"
int led=5;

void LED_init(){
  pinMode(led,OUTPUT);
  // /
  // 

}


void LED_ON(int LED_PIN){
  digitalWrite(LED_PIN,HIGH);
}
void BreathLED(int LED_PIN,int TIME){

   for(int i=0;i<255;i++){
    analogWrite(LED_PIN,i);
    delay(TIME);
  }
  delay(1000);
  for(int x=255;x>0;x--){
    analogWrite(LED_PIN,x);
    delay(TIME);
  }

}