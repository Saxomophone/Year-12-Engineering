#pragma once

#include "events/stepperEvents.hpp"


class MotionHandler2D {
  private:
    float currentX;
    float currentY;

    float targetX;
    float targetY;

    float deltaX;
    float deltaY;

    float queueY; // queue for y steps to be taken
    float queueX; // queue for x steps to be taken
    float ratioY; // ratio of deltaY to deltaX;

    StepperState *stepperY; // important that both steppers have the same interval
    StepperState *stepperX;

  public:
    MotionHandler2D(StepperState* stepperY, StepperState* stepperX) {
      this->stepperY = stepperY;
      this->stepperX = stepperX;
    }

    bool setupMotionEvent(void* context);

    bool handleMotion();
};

struct MotionParameters {
  float x;
  float y;
  bool toolheadDown;
};