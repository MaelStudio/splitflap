#include <Servo.h>

Servo myServo;
int servoMax = 180;
int servoMin = 0;

void setup() {
  Serial.begin(115200);

  pinMode(D0, INPUT_PULLUP);
  myServo.attach(D1);

  myServo.write(servoMin);
  delay(500);
  myServo.write(90);
  delay(500);
  myServo.write(servoMin);
}

void loop() {
  while (digitalRead(D0) == 0) {
    delay(10);
  }z
  
  while (digitalRead(D0) == 1) {
    delay(10);
  }

  myServo.write(servoMin);
  Serial.println("ON");

  while (digitalRead(D0) == 0) {
    delay(10);
  }

  while (digitalRead(D0) == 1) {
    delay(10);
  } 
  
  myServo.write(servoMax);
  Serial.println("OFF");
}
