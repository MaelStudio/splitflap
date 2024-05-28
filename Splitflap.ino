class Module {
  public:
    // module constructor
    Module(int mtrPins[4], int hallPin) {

      // CONSTANTS
      flaps_count = 40;
      stepsPerRev = 2048;
      stepsPerFlap = stepsPerRev/flaps_count;

      // VARS
      stepIdx = 0;
      displayedIdx = 0;

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

    void home() {
      while (!digitalRead(sensorPin)) {
        step(1);
      }
      while (digitalRead(sensorPin)) {
        step(1);
      }

      displayedIdx = 0;
      displayed = chars[0];
    }

    void display(char c) {

      c = toupper(c);

      targetIdx = findCharIdx(c);
      if (targetIdx < displayedIdx) home(); // loop back to home if target character is behind currently displayed
      
      step(stepsPerFlap * (targetIdx - displayedIdx)); // rotate to target character
      
      // update status vars
      displayedIdx = findCharIdx(c);
      displayed = c;
    }

    void displayStr(const char* str, unsigned long interval) {
      for (int i = 0; str[i] != '\0'; i++) {
        display(str[i]);
        delay(interval);
      }
    }

  private:
    int motorPins[4];
    int sensorPin;
    int flaps_count;
    int stepsPerRev;
    int stepsPerFlap;
    int stepIdx;
    int displayedIdx;
    int targetIdx;
    bool stepSequence[4][4] = {
        {1,0,0,1},
        {0,1,0,1},
        {0,1,1,0},
        {1,0,1,0}
    };
    char chars[40] = {' ', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', ':', '$'};
    char displayed = chars[0];

    void step(int n) {
      for (int i=0; i<n; i++) {
        // write correct sequence to step the motor
        for (int in=0; in<4; in++) {
          digitalWrite(motorPins[in], stepSequence[stepIdx][in]);
        }

        // next in sequence
        stepIdx++;
        if (stepIdx == 4) {
          stepIdx = 0;
        }
        
        delay(2);

      }
    }

    int findCharIdx(char c) {
      for (int i = 0; i < flaps_count; i++) if (chars[i] == c) return i;
      return -1;
    }

};



int motorPins[4] = {9, 10, 11, 12};
Module firstModule(motorPins, 8);

void setup()
{
  Serial.begin(9600);
  firstModule.home();
  delay(1000);
}

void loop()
{
  firstModule.displayStr("0123456789", 500);
}