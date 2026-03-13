#include "Module.h"

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

  bool moving() {
    for (int i = 0; i < size; i++) {
      if (modules[i].moving) {
        return true;
      }
    }
    return false;
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