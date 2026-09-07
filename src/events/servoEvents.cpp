#include "Arduino.h"
#include "events/servoEvents.hpp"

int Servo::angleToDutyCycle(int angle) {
  // Map the angle (0-180) to the duty cycle
  return map(angle, 0, 180, 60, 250);
}

void Servo::setup(int pin) {
  controlPin = pin;
  pinMode(controlPin, OUTPUT);
}

void Servo::setAngle(int angle) {
  int dutyCycle = angleToDutyCycle(angle);
  analogWrite(controlPin, dutyCycle);
}