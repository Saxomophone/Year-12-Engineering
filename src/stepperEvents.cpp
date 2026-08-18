#include "Arduino.h"
#include "stepperEvents.hpp"

// define limits in terms of absolute area
// take preprocessed gcode
// turn gcode into array of instructions
// process instructions:
    // G21: set units to mm (mostly just ignore)
    // G90: set to absolute positioning (mostly just ignore)
    // G0: raise tool and move to position (x, y, z)
    // G1: lower tool and move to position (x, y, z)
    // this should be literally all i need. z shouldn't be used

// create function with take current position, target position, and boolean toolhead down
    // create relative movement vector [x, y] as target position - current position
    // set toolhead up/down position
    // turn distance in mm to steps 
    // schedule x and y motors to move

// gcode preprocessor:
// input: pointer to GCode file as generated from svg by https://sameer.github.io/svg2gcode/
// gcode cheatsheet at https://machiningconceptserie.com/g-code-cheat-sheet/
// output: array of instructions: [START, G0 x y, G1 x y, ... END

bool StepperState::setupStepperEvent(StepperState newState) {
  
  if (newState.stopCondition == TIME) {
    stopCondition = TIME;
    startTime = millis();
    duration = newState.duration;
  } else if (newState.stopCondition == STEPS) {
    stopCondition = STEPS;
    targetSteps = newState.targetSteps;
  } else if (newState.stopCondition == TRIGGER) {
    stopCondition = TRIGGER;
    trigger = newState.trigger;
  } else {
    Serial.println("Invalid stop condition");
    return false;
  }

  interval = newState.interval;
  lastToggleTime = millis();
  digitalWrite(stepPin, LOW); // ensure pin starts low
  pinState = PIN_LOW;

  return true;
}


bool StepperState::handleStepper() {
  unsigned long currentTime = millis();

  if (currentTime - lastToggleTime >= interval) { //if update interval has passed

    digitalWrite(stepPin, !digitalRead(stepPin)); //toggle physical pin
    pinState = (pinState == PIN_LOW) ? PIN_HIGH : PIN_LOW; // update pin state

    lastToggleTime = currentTime;
    
    
    if (pinState == PIN_HIGH) { // only count steps on one edge (e.g. rising)
      stepsTaken++;
    }
  }

  switch (stopCondition) {
    case TIME:
      if (currentTime - startTime >= duration) {
        return true; // Done
      }
      break;

    case STEPS:
      if (stepsTaken >= targetSteps) {
        return true; // Done
      }
      break;

    case TRIGGER:
      return *(trigger); // Done when trigger becomes true
  }

  return false; // Keep waiting
}


bool StepperState::sleepStepper() {
  digitalWrite(stepPin, LOW); // sleep pin is active low
  return true;
}


bool StepperState::wakeStepper() {
  digitalWrite(stepPin, HIGH); // sleep pin is active low
  delay(1); // I don't like using a blocking function but I need to block the stepper and its only a short delay called right at the start. I also don't want to bother cause it really doesn't matter.
  return true;
}

void StepperState::init(int pin) {
  stepPin = pin;
}