#include <Stepper.h>

#define REV_STEPS -2038

Stepper newStepper(byte in1, byte in2, byte in3, byte in4) {
  return Stepper(abs(REV_STEPS), in1, in3, in2, in4);
}

Stepper myStepper = newStepper(10, 11, 12, 13);

void setup() {
  myStepper.setSpeed(10);
}

void loop() {
	myStepper.step(REV_STEPS);
	delay(500);
}
