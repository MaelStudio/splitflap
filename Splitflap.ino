class Module {
  public:
    // module constructor
    Module(int mtrPins[4], int hallPin) {

      // CONSTANTS
      int flaps_count = 40;
      int stepsPerRev = 2048;
      int stepsPerFlap = stepsPerRev/flaps_count;

      // VARS
      int stepIdx = 0;

      // PINS
      sensorPin = hallPin;

      // rearrange stepper pins in order IN1-IN3-IN2-IN4
      int order[4] = {0, 2, 1, 3};
      for (int i=0; i<4; i++) {
        motorPins[order[i]] = mtrPins[i];
      }

      // set all stepper pins as outputs
      for (int i=0; i<4; i++) {
        pinMode(motorPins[i], OUTPUT);
      }

      // set hall effect sensor pin as an input with pullup resistor
      pinMode(sensorPin, INPUT_PULLUP);
    }

    void step(int n) {
      
      // write correct sequence to step the motor
      for (int i=0; i<n; i++) {
        // write each bit to each input
        for (int in=0; in<4; in++) {
          digitalWrite(motorPins[in], stepSequence[stepIdx][in]);
        }

        // next in sequence
        stepIdx++;
        if (stepIdx == 4) {
          stepIdx = 0;
        }
      }
    }

    void home() {
      while (!digitalRead(sensorPin)) {
        step(1);
        delay(2);
      }
      while (digitalRead(sensorPin)) {
        step(1);
        delay(2);
      }

      displayed = chars[0];
    }

  private:
    int motorPins[4];
    int sensorPin;
    int flaps_count;
    int stepsPerRev;
    int stepsPerFlap;
    int stepIdx;
    bool stepSequence[4][4] = {
        {1,0,0,1},
        {0,1,0,1},
        {0,1,1,0},
        {1,0,1,0}
    };
    char chars[40] = {' ', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', ';', '$'};
    char displayed = chars[0];
};

void setup()
{
  Serial.begin(9600);
  int motorPins[4] = {9, 10, 11, 12};
  Module firstModule(motorPins, 8);

  firstModule.home();
}

void loop()
{

}



// int findCharIdx(char c) {
//   for (int i = 0; i < FLAPS_COUNT; i++) if (chars[i] == c) return i;
//   return -1;
// }

// void displayChar(Stepper motor, char c) {

//   c = toupper(c);

//   int currentIdx = findCharIdx(display);
//   int destinationIdx = findCharIdx(c);

//   // loop back to start if destination character is behind
//   if (destinationIdx < currentIdx)
//   {
//     while (!digitalRead(hallPin))
//     {
//       motor.step(-1);
//     }
//     while (digitalRead(hallPin))
//     {
//       motor.step(-1);
//     }
//     currentIdx = 0;
//   }

//   // rotate to destination character
//   motor.step(STEP*(destinationIdx-currentIdx));
//   display = c;
// }

// void displayString(const char* str, unsigned long delayTime) {
//   for (int i = 0; str[i] != '\0'; i++) {
//     displayChar(myStepper, str[i]);
//     delay(delayTime);
//   }
// }
