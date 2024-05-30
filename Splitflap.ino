class Module {
public:

  // public vars
  char displayed;

  // module constructor
  Module() {

    // CONSTANTS
    flaps_count = 40;
    stepsPerRev = 2048;
    stepsPerFlap = stepsPerRev / flaps_count;

    // VARS
    stepIdx = 0;
    displayedIdx = 0;
    toStep = 0;
    lastStepTime = 0;
  }

  void setup(int hallPin, int in1, int in2, int in3, int in4) {

    // rearrange stepper pins in order IN1-IN3-IN2-IN4
    motorPins[0] = in1;
    motorPins[1] = in3;
    motorPins[2] = in2;
    motorPins[3] = in4;

    // set all stepper pins as outputs
    for (int i = 0; i < 4; i++) pinMode(motorPins[i], OUTPUT);

    sensorPin = hallPin;
    pinMode(sensorPin, INPUT_PULLUP);  // set hall effect sensor pin as an input with pullup resistor
  }

  void tick() {

    if (homing) {
      if (step() and digitalRead(sensorPin)) {
        homing = false;
        displayedIdx = 0;
        displayed = chars[0];
      }
    }

    if (toStep > 0) {
      if (step()) toStep--;
      
      // check if target character has been reached
      if (toStep == 0) {
        // update status vars
        displayedIdx = findCharIdx(c);
        displayed = c;
      }
    }
  }

  void home() {
    homing = true;
  }

  void display(char c) {

    c = toupper(c);
    int targetIdx = findCharIdx(c);

    // loop back to home if target character is behind currently displayed
    if (targetIdx < displayedIdx) {
      home();
      toStep = stepsPerFlap * targetIdx; // rotate to target character
    } else {
      toStep = stepsPerFlap * (targetIdx - displayedIdx); // rotate to target character
    }

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
  int stepsLeft;
  bool homing;
  unsigned long lastStepTime;
  bool stepSequence[4][4] = {
    { 1, 0, 0, 1 },
    { 0, 1, 0, 1 },
    { 0, 1, 1, 0 },
    { 1, 0, 1, 0 }
  };
  char chars[40] = { ' ', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', ':', '$' };

  bool step() {
    unsigned long now = mircos();
    if (now - lastStepTime < 2000) return false;
  
    // write correct sequence to step the motor
    for (int in = 0; in < 4; in++) digitalWrite(motorPins[in], stepSequence[stepIdx][in]);

    // next in sequence
    stepIdx++;
    if (stepIdx == 4) {
      stepIdx = 0;
    }
    
    lastStepTime = now;
    return true;
  }

  int findCharIdx(char c) {
    for (int i = 0; i < flaps_count; i++)
      if (chars[i] == c) return i;
    return -1;
  }
};


Module modules[2];

void setup() {
  Serial.begin(9600);

  int pins[2][5] = {
    { 8, 9, 10, 11, 12 },
    { 3, 4,  5,   6, 7 }
  };

  for (int i = 0; i < 2; i++) modules[i].setup(pins[i][0], pins[i][1], pins[i][2], pins[i][3], pins[i][4]);
  for (int i = 0; i < 2; i++) modules[i].home();

  delay(1000);
}

void loop() {
  for (int i = 0; i < 2; i++) modules[i].tick();
}
