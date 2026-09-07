#pragma once

#include "pin.hpp"
#include "Arduino.h"

class Servo {
  private:
    int controlPin;

    int angleToDutyCycle(int angle);

  public:

    void setup(int pin);

    void setAngle(int angle);
};