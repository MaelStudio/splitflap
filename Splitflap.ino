class Module {
public:

  // Public vars
  bool homing;
  char displayed;

  // Module constructor
  Module() {

    // CONSTANTS
    flapsCount = 40; // Flaps in module
    stepsPerRev = 2048; // Steps per motor full revolution
    stepsPerFlap = stepsPerRev / flapsCount; // Steps required to rotate one character forward
    stepInterval = 1800; // Time interval between each motor step in microseconds

    // VARS
    stepSequenceIdx = 0;
    flapStepIdx = 0;
    lastStepTime = 0;
    displayedIdx = 0;
    moving = false;
    offsetSteps = 0;
  }

  void setup(int hallPin, int in1, int in2, int in3, int in4, int off) {

    offset = off;

    // Rearrange stepper pins in order IN1-IN3-IN2-IN4
    motorPins[0] = in1;
    motorPins[1] = in3;
    motorPins[2] = in2;
    motorPins[3] = in4;

    // Set all stepper pins as outputs
    for (int i = 0; i < 4; i++) pinMode(motorPins[i], OUTPUT);

    sensorPin = hallPin;
    pinMode(sensorPin, INPUT_PULLUP);  // Set hall effect sensor pin as an input with pullup resistor
  }

  void tick() {

    // Reset before homing
    if (resetting) {
      if (step() && !isOnMagnet()) {
        resetting = false;
      }
      return;
    }

    if (homing) {
      // Try to step and check if home position is reached
      if (step() && isOnMagnet()) {
        displayedIdx = 0;
        displayed = chars[0];
        flapStepIdx = 0;
        homing = false;
      }
      return; // Finish homing before rotating to target character
    }

    // Step until target character has been reached
    if (moving && step() && displayedIdx == targetIdx) {
      offsetSteps = offset;
      moving = false;
    }

    if (offsetSteps && step()) {
      offsetSteps--;
    }
  }

  void reset() {
    resetting = true;
    home();
  }

  void display(char c) {

    c = toupper(c);
    targetIdx = findCharIdx(c);

    // Loop back to home if target character is behind currently displayed
    if (targetIdx < displayedIdx) {
      home();
    }
    if (targetIdx != displayedIdx) {
      moving = true;
    }
    
  }

private:
  int motorPins[4];
  int sensorPin;
  int flapsCount;
  int stepsPerRev;
  int stepsPerFlap;
  int stepInterval;
  int stepSequenceIdx;
  int flapStepIdx;
  int displayedIdx;
  int targetIdx;
  int moving;
  int offset;
  int offsetSteps;
  unsigned long lastStepTime;
  bool resetting;
  bool stepSequence[4][4] = {
    { 1, 0, 0, 1 },
    { 0, 1, 0, 1 },
    { 0, 1, 1, 0 },
    { 1, 0, 1, 0 }
  };
  char chars[40] = { ' ', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', ':', '$' };

  bool step() {
    unsigned long now = micros();
    if (now - lastStepTime < stepInterval) return false;
  
    // Write correct sequence to step the motor
    for (int in = 0; in < 4; in++) digitalWrite(motorPins[in], stepSequence[stepSequenceIdx][in]);

    // Next in sequence
    stepSequenceIdx++;
    if (stepSequenceIdx == 4) {
      stepSequenceIdx = 0;
    }

    // flap step
    flapStepIdx++;
    if (flapStepIdx == stepsPerFlap) {
      flapStepIdx = 0;
      displayedIdx++;
      if (displayedIdx == flapsCount) {
        displayedIdx = 0;
      }
      displayed = chars[displayedIdx];
    }
    
    lastStepTime = now;
    return true;
  }

  void home() {
    homing = true;
  }

  bool isOnMagnet() {
    return !digitalRead(sensorPin);
  }

  int findCharIdx(char c) {
    for (int i = 0; i < flapsCount; i++) if (chars[i] == c) return i;
    return -1;
  }

};

class Display {
public:
  int size;

  Display(int size) : size(size) {
    modules = new Module[size];
  }

  void setup(int pins[][6]) {
    for (int i = 0; i < size; i++) {
      modules[i].setup(pins[i][0], pins[i][1], pins[i][2], pins[i][3], pins[i][4], pins[i][5]);
      modules[i].reset();
    }

    // Wait until all modules are reset to the home position
    while (!allModulesAtHome()) {
      tick();
    }
  }

  void write(const char *message) {
    for (int i = 0; i < size; i++) {
      if (i < strlen(message)) {
        modules[i].display(message[i]);
      } else {
        modules[i].display(' ');  // Fill remaining modules with blank spaces
      }
    }
  }

  void tick() {
    for (int i = 0; i < size; i++) {
      modules[i].tick();
    }
  }

private:
  Module *modules;

  bool allModulesAtHome() {
    for (int i = 0; i < size; i++) {
      if (modules[i].homing || modules[i].displayed != ' ') {
        return false;
      }
    }
    return true;
  }
};

// Display with 6 modules
Display display(6);

void setup() {
  Serial.begin(9600);

  // Module pins and offsets
  int pins[display.size][6] = {
    {42, 44, 46, 48, 50, 0},
    {32, 34, 36, 38, 40, 0},
    {A3, A4, A5, A6, A7, 0},
    {3, 4, 5, 6, 7, 0},
    {9, 10, 11, 12, 13, 0},
    {22, 24, 26, 28, 30, 0}
  };

  Serial.println("Setting up modules...");
  display.setup(pins);

  Serial.println("Done! Type messages to display them!");
}

void loop() {
  // Check if there's any serial input available
  if (Serial.available() > 0) {
    // Read the input string
    String input = Serial.readStringUntil('\n');
    
    // Ensure the input string length matches the number of modules
    if (input.length() > display.size) {
      Serial.println("Error: Message is too long for display.");
      return;
    }

    // Display the message
    display.write(input.c_str());
  }

  // Continuously update the display
  display.tick();
}