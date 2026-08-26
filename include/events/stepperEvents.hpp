#pragma once


#include "pin.hpp"
#include "Arduino.h"

enum StepperStopCondition { TIME, STEPS, TRIGGER };


struct toolheadPosition {
  int x;
  int y;
  bool down;
};


// State for the stepper motor
class StepperState {
  private:
    int stepPin;
    PinState pinState;

    int sleepPin;
    int directionPin;

    // stop condition
    StepperStopCondition stopCondition;

    // values for stop conditions
    unsigned long startTime = millis(); // for TIME condition
    unsigned long duration; // for TIME condition

    int stepsTaken = 0; // for STEPS condition
    int targetSteps; // for STEPS condition

    bool* trigger; // for TRIGGER condition (set up such that the stepper will run until the trigger becomes true)

  public:

    StepperState(int stepPin, int sleepPin, int directionPin, int direction, unsigned int interval = 2, StepperStopCondition stopCondition = STEPS, int targetSteps = 0, unsigned long duration = 0, bool* trigger = nullptr) {
      this->stepPin = stepPin;
      this->sleepPin = sleepPin;
      this->directionPin = directionPin;
      pinMode(stepPin, OUTPUT);
      pinMode(sleepPin, OUTPUT);
      pinMode(directionPin, OUTPUT);

      if (direction) {
        digitalWrite(directionPin, HIGH);
      } else {
          digitalWrite(directionPin, LOW);
      }

      this->interval = interval;
      this->stepsTaken = 0;
      this->stopCondition = stopCondition;

      this->duration = duration;
      this->targetSteps = targetSteps;
      this->trigger = trigger;
    }

    unsigned long lastToggleTime;
    unsigned int interval; // ms between toggles
    int direction; // 1 for one direction, -1 for the other



    bool setupStepperEvent(void* context);

    bool handleStepper();

    bool wakeStepper();

    bool sleepStepper();

    bool setDirection(int direction);

    void step(); // has no checks for the interval, is just a raw step
};