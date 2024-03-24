#include <Stepper.h>

const int stepsPerRevolution = 2038;
Stepper myStepper = Stepper(stepsPerRevolution, 8, 10, 9, 11);

void setup() {
  myStepper.setSpeed(15);
}

void loop() {
	myStepper.step(-1450);
	delay(1000);
}
