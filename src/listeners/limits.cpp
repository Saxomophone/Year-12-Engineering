#include "listeners/limits.hpp"
#include "eventScheduler.hpp"


#include "Arduino.h"

#define X_LIMIT_PIN 8
#define Y_LIMIT_PIN 8

bool xLimitReached = false;
bool yLimitReached = false;

bool x_limit_button() {
  int pressed = digitalRead(X_LIMIT_PIN);
  return (pressed) ? false : true;  // false if 1 and true if 0 as pulled up 
}

bool y_limit_button() {
  int pressed = digitalRead(Y_LIMIT_PIN);
  return (pressed) ? false : true;
}

void initLimits(EventScheduler* scheduler) {
  pinMode(X_LIMIT_PIN, INPUT_PULLUP);
  pinMode(Y_LIMIT_PIN, INPUT_PULLUP);

  scheduler->addListener(&xLimitReached, x_limit_button);
  scheduler->addListener(&yLimitReached, y_limit_button);
}