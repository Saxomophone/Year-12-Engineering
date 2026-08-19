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

    StepperState(int stepPin, int sleepPin, int directionPin, unsigned int interval = 2, StepperStopCondition stopCondition = STEPS, void* stopValue = nullptr) {
        this->stepPin = stepPin;
        this->sleepPin = sleepPin;
        this->directionPin = directionPin;
        pinMode(stepPin, OUTPUT);
        pinMode(sleepPin, OUTPUT);
        pinMode(directionPin, OUTPUT);

        this->interval = interval;
        this->stepsTaken = 0;
        this->stopCondition = stopCondition;

        switch (stopCondition) {
            case TIME: {
                unsigned long* duration = (unsigned long*)stopValue;
                this->duration = *duration;
                break;
            }
            case STEPS: {
                int* targetSteps = (int*)stopValue;
                this->targetSteps = *targetSteps;
                break;
            }
            case TRIGGER: {
                this->trigger = trigger;
                break;
            }
        }  
    }

    bool setupStepperEvent(void* context);

    bool handleStepper();

    bool wakeStepper();

    bool sleepStepper();
};