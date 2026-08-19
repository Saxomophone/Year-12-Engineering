#pragma once

#include "events/stepperEvents.hpp"


class MotionHandler2D {
  private:
    float currentX;
    float currentY;
    StepperState *stepperY;
    StepperState *stepperX;

  public:
    MotionHandler2D(StepperState* stepperY, StepperState* stepperX) {

    }
};