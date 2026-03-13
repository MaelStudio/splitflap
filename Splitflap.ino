// RTC libraries
#include <Wire.h>
#include <DS3231.h>

// Import Display class
#include "Display.h"

// Display with 6 modules
#define DISPLAY_SIZE 6
Display display(DISPLAY_SIZE);

char buf[DISPLAY_SIZE+1]; // string buffer with extra null terminator char

// RTC
RTClib rtc;
DateTime dateTime;

// Rotary encoder
#define ROTARY_CLK_PIN 2
#define ROTARY_DATA_PIN A0
#define ROTARY_SW_PIN A1
int rotaryCtr = 0;

// WiFi
bool wifiConnected = false;
byte ip[4];
bool receivedWeather = false;
bool receivedYtSubs = false;
int temperature;
int humidity;
int ytSubs;
char lastDisplayedMsg[DISPLAY_SIZE+1];

// MODES
#define MESSAGE -1
#define IP 0
#define TIME 1
#define DATE 2
#define TEMP 3
#define YOUTUBE 4
#define SLEEP 5

#define START_MODE 0
#define END_MODE 5

int mode = START_MODE;
int lastMode = START_MODE;

// IP mode
int ipByteIdx = 0;
bool displayingByte = false;

// Auto sleep
#define AUTO_SLEEP 0
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

  int offsets[DISPLAY_SIZE] = {0, 20, 0, 10, 18, 10};

  display.setup(pins, offsets);

  Serial.println();
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

  // Check if received command from ESP32
  if (Serial1.available() > 0) {
    String msg = Serial1.readStringUntil('\n');
    msg.trim();

    Serial.println(msg); // debug

    String* args = splitStr(msg, ' '); // command, arg1, arg2, ...
    String command = args[0]; // Command name

    // < IP ip
    if (command == "IP") {
      char ipStr[16];
      strcpy(ipStr, args[1].c_str());
      sscanf(ipStr, "%hhu.%hhu.%hhu.%hhu", &ip[0], &ip[1], &ip[2], &ip[3]); // Parse IP address into individual bytes

      wifiConnected = true;
    }
    // < SEND msg
    else if (command == "SEND") {
      display.write(msg.substring(command.length()+1).c_str()); // Display the message
      mode = MESSAGE; // Set mode to MESSAGE
    }
    // < MODE id
    else if (command == "MODE") {
      mode = args[1].toInt();
    }
    // < WEATHER temp humidity
    else if (command == "WEATHER") {
      receivedWeather = true;
      temperature = args[1].toInt();
      humidity = args[2].toInt();
    }
    // < YOUTUBE subs
    else if (command == "YOUTUBE") {
      receivedYtSubs = true;
      ytSubs = args[1].toInt();
    }
    // < ESP_RESET
    else if (command == "ESP_RESET") {
      wifiConnected = false;

      // Send current displayed message and mode
      display.getDisplayedMessage(buf);
      Serial1.print("DISPLAY "); Serial1.println(buf);
      Serial1.print("MODE "); Serial1.println(mode);

      // Reconnect to WiFi
      Serial1.println("CONNECT");
    }

    delete[] args;
  }

  // Send real time displayed message
  display.getDisplayedMessage(buf);
  if (strcmp(buf, lastDisplayedMsg)) {
    // > DISPLAY msg
    Serial1.print("DISPLAY "); Serial1.println(buf);
    strcpy(lastDisplayedMsg, buf);
  }

  // Send current mode
  if (mode != lastMode) {
    // MODE id
    Serial1.print("MODE "); Serial1.println(mode);
    lastMode = mode;

    // Setup modes
    if (mode == IP) {
      ipByteIdx = 0;
      displayingByte = false;
    }
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
  if (AUTO_SLEEP) {
    if (mode != SLEEP && dateTime.hour() == autoSleepTime[0] && dateTime.minute() == autoSleepTime[1]) {
      mode = SLEEP;
    }
    if (mode == SLEEP && dateTime.hour() == autoWakeTime[0] && dateTime.minute() == autoWakeTime[1]) {
      mode = TIME;
    }
  }

  // IP mode
  if (mode == IP) {
    if (wifiConnected) {
      static unsigned long ipByteTime = now;

      if (ipByteIdx < 4) {
        sprintf(buf, "IP %3d", ip[ipByteIdx]); // Format current ip byte
      } else {
        sprintf(buf, "IP    ", ip[ipByteIdx]); // Blank so we know it's starting over
      }
      display.write(buf);

      // Wait for current byte to appear on display
      if (!displayingByte && !display.moving()) {
        displayingByte = true;
        ipByteTime = now;
      }
      if (displayingByte && now - ipByteTime > 2000) { // Pause 2 seconds before moving on to next byte
        ipByteIdx++;
        if (ipByteIdx > 4) { // Index 4 is blank so we know it's starting over
          ipByteIdx = 0;
        }
        displayingByte = false;
      }
    } else {
      display.write("NOWIFI");
    }
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
      display.write("NODATA");
    }
  }

  // YOUTUBE mode
  if (mode == YOUTUBE) {
    if (receivedYtSubs) {
      sprintf(buf, "YT %d", ytSubs);
      display.write(buf);
    } else {
      display.write("NODATA");
    }
  }

  // SLEEP mode
  if (mode == SLEEP) {
    display.write("      ");
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