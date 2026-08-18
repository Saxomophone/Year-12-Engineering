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
    unsigned long lastToggleTime;
    unsigned int interval; // ms between toggles

    // stop condition
    StepperStopCondition stopCondition;

    // values for stop conditions
    unsigned long startTime = millis(); // for TIME condition
    unsigned long duration; // for TIME condition

    int stepsTaken = 0; // for STEPS condition
    int targetSteps; // for STEPS condition

    bool* trigger; // for TRIGGER condition (set up such that the stepper will run until the trigger becomes true)

  public:
    void init(int pin);

    bool setupStepperEvent(StepperState newState);

    bool handleStepper();

    bool wakeStepper();

    bool sleepStepper();
};