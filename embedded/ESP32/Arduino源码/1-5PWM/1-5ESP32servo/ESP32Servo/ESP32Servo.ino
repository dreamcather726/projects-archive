#include <ESP32Servo.h>
int Servo_PIN1=10;
int Servo_PIN1=11;
int Servo_PIN1=12;
int Servo_PIN1=13;
int Servo_PIN1=1;
Servo myservo;
void setup() {

  myservo.attach(Servo_PIN);

}

void loop() {
  // put your main code here, to run repeatedly:
  
  myservo.write(0);
  delay(1000);
  Serial.println("舵机转到0度");
   myservo.write(180);
  delay(1000);
  Serial.println("舵机转到180度");
}
