// RTC libraries
#include <Wire.h>
#include <DS3231.h>

class Module {
public:

  // Public vars
  int motorPins[4];
  bool homing;
  char displayed;

  // Module constructor
  Module() {

    // CONSTANTS
    flapsCount = 40; // Flaps in module
    stepsPerRev = 2048; // Steps per motor full revolution
    stepsPerFlap = stepsPerRev / flapsCount; // Steps required to rotate one character forward
    stepInterval = 1700; // Time interval between each motor step in microseconds

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

    if (homing) {
      // If module is already on magnet before homing, rotate until not on magnet
      if (preHoming) {
        if (step() && !isOnMagnet()) {
          preHoming = false;
        }
        return;
      }
      // Offset
      if (offsetSteps) {
        if (step()) {
          offsetSteps--;
          if (!offsetSteps) {
            setHome(); // stop homing sequence
            turnOffInputs();
          }
        }
        return;
      }
      // Try to step and check if home position is reached
      if (step() && isOnMagnet()) {
        if (offset) {
          offsetSteps = offset;
        } else {
          setHome(); // stop homing sequence
          turnOffInputs();
        }
      }
      return; // Finish homing before rotating to target character
    }

    // Step until target character has been reached
    if (moving && step() && displayedIdx == targetIdx) {
      moving = false;
      turnOffInputs();
    }
    
  }
  
  void home() {
    preHoming = true;
    homing = true;
  }

  void display(char c) {

    c = toupper(c);
    if (targetIdx == findCharIdx(c)) return;
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
  bool preHoming;
  bool stepSequence[4][4] = {
    { 1, 0, 0, 1 },
    { 0, 1, 0, 1 },
    { 0, 1, 1, 0 },
    { 1, 0, 1, 0 }
  };
  char chars[40] = { ' ', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', ':', '$' };

  void setHome() {
    displayedIdx = 0;
    displayed = chars[0];
    flapStepIdx = 0;
    homing = false;
  }

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

  void turnOffInputs() {
    for (int in = 0; in < 4; in++) {
      digitalWrite(motorPins[in], LOW);
    }
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

  void setup(int pins[][5], int offsets[]) {
    for (int i = 0; i < size; i++) {
      modules[i].setup(pins[i][0], pins[i][1], pins[i][2], pins[i][3], pins[i][4], offsets[i]);
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

  void calibrate() {
    for (int i = 0; i < size; i++) {
      modules[i].home();
    }

    // Wait until all modules are homed
    while (!calibrated()) {
      tick();
    }
  }

  void getDisplayedMessage(char *buffer) {
    
    for (int i = 0; i < size; i++) {
      buffer[i] = modules[i].displayed;
    }
    
    buffer[size] = '\0'; // Null-terminate the string
  }


  // Power each motor pin 1 by 1
  void debugPins() {
    for (int i = 0; i < size; i++) {
      digitalWrite(modules[i].motorPins[0], HIGH); // in1
      delay(150);
      digitalWrite(modules[i].motorPins[2], HIGH); // in2
      delay(150);
      digitalWrite(modules[i].motorPins[1], HIGH); // in3
      delay(150);
      digitalWrite(modules[i].motorPins[3], HIGH); // in4
      delay(150);
    }
}

private:
  Module *modules;

  bool calibrated() {
    for (int i = 0; i < size; i++) {
      if (modules[i].homing) {
        return false;
      }
    }
    return true;
  }
};


// Display with 6 modules
#define DISPLAY_SIZE 6
Display display(DISPLAY_SIZE);
char buf[DISPLAY_SIZE+1]; // string buffer with extra null terminator char

// RTC
#define ROTARY_CLK_PIN 2
#define ROTARY_DATA_PIN A0
#define ROTARY_SW_PIN A1
RTClib rtc;
int rotaryCtr = 0;
DateTime dateTime;

// WiFi
bool wifiConnected = false;
bool receivedWeather = false;
int temperature;
int humidity;
char lastDisplayedMsg[DISPLAY_SIZE+1];

// MODES
#define START_MODE 0
#define END_MODE 3

int mode = START_MODE;
int lastMode = START_MODE;

#define MESSAGE -1
#define TIME 0
#define DATE 1
#define TEMP 2
#define SLEEP 3

// Auto sleep
int autoSleepTime[2] = {21, 0}; // 21:00
int autoWakeTime[2] = {6, 30}; // 6:30

void setup() {
  Serial.begin(115200); // Serial monitor prints
  Serial1.begin(115200); // Communication with ESP32 for WiFi

  // RTC
  Wire.begin();

  // Rotary encoder
  pinMode(ROTARY_CLK_PIN, INPUT);
  pinMode(ROTARY_DATA_PIN, INPUT);
  pinMode(ROTARY_SW_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ROTARY_CLK_PIN), rotaryEncoderISR, CHANGE);

  // Module pins
  // sensor, in1, in2, in3, in4
  int pins[DISPLAY_SIZE][5] = {
    {42, 44, 46, 48, 50},
    {32, 34, 36, 38, 40},
    {A3, A4, A5, A6, A7},
    {8, 4, 5, 6, 7},
    {9, 10, 11, 12, 13},
    {22, 24, 26, 28, 30}
  };

  int offsets[DISPLAY_SIZE] = {0, 20, 0, 10, 20, 10};

  display.setup(pins, offsets);

  Serial.println("Calibrating modules...");
  display.calibrate();
  Serial.println("Done!");

  // Connect to WiFi
  Serial1.println("CONNECT");
}

void loop() {
  unsigned long now = millis();
  
  // Continuously update the display
  display.tick();
  
  // == ESP32 COMMUNICATION OVER UART ==

  // Check if received response/command from ESP32
  if (Serial1.available() > 0) {
    String msg = Serial1.readStringUntil('\n');
    msg.trim();

    Serial.println(msg); // debug

    String* args = splitStr(msg, ' '); // command, arg1, arg2, ...
    String command = args[0]; // Command name

    // CONNECT response
    if (command == "CONNECT") {
      String ip = args[1];
      wifiConnected = true;
    }
    // WEATHER response
    else if (command == "WEATHER") {
      receivedWeather = true;
      temperature = args[1].toInt();
      humidity = args[2].toInt();
    }
    // SEND command
    else if (command == "SEND") {
      display.write(msg.substring(command.length()+1).c_str()); // Display the message
      mode = MESSAGE; // Set mode to MESSAGE
    }

    // MODE command
    else if (command == "MODE") {
      mode = args[1].toInt();
    }
    
    delete[] args;
  }

  // Send real time displayed message
  display.getDisplayedMessage(buf);
  if (strcmp(buf, lastDisplayedMsg)) {
    Serial1.print("DISPLAY "); Serial1.println(buf);
    strcpy(lastDisplayedMsg, buf);
  }

  // Send current mode
  if (mode != lastMode) {
    Serial1.print("MODE "); Serial1.println(mode);
    lastMode = mode;
  }
  
  // Request weather data to ESP32 every 10 minutes
  static unsigned long lastWeatherTime = now - 1000UL*60*10;
  
  if (wifiConnected && now - lastWeatherTime > 1000UL*60*10) {
    Serial1.println("WEATHER");
    lastWeatherTime = now;
  }

  // == ROTARY ENCODER AND RTC ==

  // Change mode by pressing the rotary encoder switch
  static unsigned long lastPressTime = 0;
  if (digitalRead(ROTARY_SW_PIN) == LOW) {
    if (now - lastPressTime > 300) {
      mode++;
      if (mode > END_MODE) {
        mode = START_MODE;
      }
      lastPressTime = now;
    }
  }

  // Get time and date from RTC every second
  static unsigned long lastReadTime = now - 1000;
  
  if (now - lastReadTime > 1000) {
    dateTime = rtc.now();
    lastReadTime = now;
  }

  // == DISPLAY MODES ==

  // Auto sleep and wake
  if (mode != SLEEP && dateTime.hour() == autoSleepTime[0] && dateTime.minute() == autoSleepTime[1]) {
    mode = SLEEP;
  }
  if (mode == SLEEP && dateTime.hour() == autoWakeTime[0] && dateTime.minute() == autoWakeTime[1]) {
    mode = TIME;
  }

  // TIME mode
  if (mode == TIME) {
    sprintf(buf, "%02d:%02d ", dateTime.hour(), dateTime.minute()); // Format time
    display.write(buf);
  }

  // DATE mode
  if (mode == DATE) {
    char months[12][4] = { "JAN", "FEV", "MAR", "AVR", "MAI", "JUN", "JUL", "AOU", "SEP", "OCT", "NOV", "DEC" };
    sprintf(buf, "%02d %s", dateTime.day(), months[dateTime.month() - 1]); // Format date
    display.write(buf);
  }

  // TEMP mode
  if (mode == TEMP) {
    if (receivedWeather) {

      // Format temperature
      // Determine how much space is available
      int tempLen = snprintf(NULL, 0, "%d", temperature); // Get the length of the temperature as a string, including "-" sign
      int spaces = DISPLAY_SIZE-3-tempLen; // Spaces left between "T:" and temperature + "C"
      if (spaces<0) {
        spaces = 0;
      }
      strcpy(buf, "T:");
      for (int i=0;i<spaces;i++) {
        strcat(buf, " ");
      }
      sprintf(buf, "%s%dC", buf, temperature);
      display.write(buf);
    } else {
      display.write("NOWIFI");
    }
  }

  // SLEEP mode
  if (mode == SLEEP) {
    display.write("SLEEP");
  }

  // Check if there's any serial input available
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');

    if (input.length() > DISPLAY_SIZE) {
      input = input.substring(0, DISPLAY_SIZE);
    }
    
    display.write(input.c_str()); // Display the message
    mode = MESSAGE; // Set mode to MESSAGE
  }
}

void rotaryEncoderISR() {
  static unsigned long lastISRTime = 0;
  unsigned long now = millis();

  // prevent bounce
  if(now - lastISRTime < 30) {
    return;
  }

  if (digitalRead(ROTARY_CLK_PIN) == digitalRead(ROTARY_DATA_PIN)) {
    // CCW
    rotaryCtr--;
  } else {
    // CW
    rotaryCtr++;
  }

  Serial.println(rotaryCtr);

  lastISRTime = now;
}

String* splitStr(String str, char delimiter) {
  // Count how many substrings there will be
  int len = 1; // Always at least one substring
  for (int i = 0; i < str.length(); i++) {
    if (str[i] == delimiter) {
      len++;
    }
  }

  String* output = new String[len];

  int startIndex = 0;
  int substringCount = 0;

  for (int i = 0; i < str.length(); i++) {
    if (str[i] == delimiter) {
      output[substringCount] = str.substring(startIndex, i);
      startIndex = i + 1;
      substringCount++;
    }
  }
  output[substringCount] = str.substring(startIndex);

  return output;
}