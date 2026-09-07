#include "Arduino.h"
#include "events/motionEvents.hpp"
#include "main.hpp"
#include "listeners/limits.hpp"

// going to be using whole steps for ease for this prototype
#define MM_PER_STEP 8/(360/1.8)  // 1.8 degrees per step so 360/1.8 steps in a revolution. one revolution is 8mm so one step is 8/200 = 0.04mm per step
#define STEPS_PER_MM (1/MM_PER_STEP) // 25 steps per mm
#define MIN_STEP_DISTANCE 0.04 // minimum distance to move in mm, which is one step


bool MotionHandler2D::setupMotionEvent(void* context) {
  MotionParameters* update = (MotionParameters*)context;
  targetX = update->x;
  targetY = update->y;

  // Calculate the distance to move in each direction
  float deltaX = targetX - currentX;
  float deltaY = targetY - currentY;

  queueX = 0;
  queueY = 0;

  // calculate the ratio of y steps to x steps, avoid division by zero. 
  // if its staight up or down, ratioY will be 1 and other code will manage
  // since currentX = targetX, queueX will be 0 and queueY will be incremented by 1 each time until it reaches targetY
  ratioY = (deltaX != 0) ? abs(deltaY / deltaX) : 1;  

  // Set the direction for each stepper based on the sign of deltaX and deltaY
  stepperX->setDirection(deltaX >= 0 ? 1 : -1);
  stepperY->setDirection(deltaY >= 0 ? 1 : -1);

  return true;
}

bool MotionHandler2D::handleMotion() {
  if (queueX < 1 && queueY < 1) {
    // proceed to next set of steps
    if (decimalRound(abs(targetX - currentX), 2) >= MIN_STEP_DISTANCE ) {
      queueX++;
    }
    if (decimalRound(abs(targetY - currentY), 2) >= MIN_STEP_DISTANCE ) {
      queueY += ratioY;
    } // increment the queue for y steps based on the ratio of deltaY to deltaX
  }

  unsigned long currentTime = millis();
  
  if (decimalRound(abs(targetX - currentX), 2) >= MIN_STEP_DISTANCE ) {  // if the current x position is not equal to the target x position, we need to step the x motor
    if ((currentTime - stepperX->lastToggleTime) >= stepperX->interval) {
      if (queueX >= 1) {
        stepperX->step();
        currentX += MM_PER_STEP * stepperX->direction;
        queueX--;
      }
    }
  }

  if (decimalRound(abs(targetY - currentY), 2) >= MIN_STEP_DISTANCE) {  // if the current y position is not equal to the target y position, we need to step the y motor
    if ((currentTime - stepperY->lastToggleTime) >= stepperY->interval) {
      if (queueY >= 1) {
        stepperY->step();
        currentY += MM_PER_STEP * stepperY->direction;
        queueY--;
      }
    }
  }
  
  // Serial.println("deltaX = " + String(targetX - currentX) + " deltaY = " + String(targetY - currentY));
  if (decimalRound(abs(targetY - currentY), 2) < MIN_STEP_DISTANCE && decimalRound(abs(targetY - currentY), 2) < MIN_STEP_DISTANCE) {
    return true;
  } else {
    return false;
  }
}

bool MotionHandler2D::wakeMotion() {
  stepperY->wakeStepper();
  stepperX->wakeStepper();
  return true;
}

bool MotionHandler2D::sleepMotion() {
  stepperY->sleepStepper();
  stepperX->sleepStepper();
  return true;
}

bool MotionHandler2D::setupHoming() {
  stepperX->setDirection(-1);
  stepperY->setDirection(-1);

  stepperX->lastToggleTime = millis();
  stepperY->lastToggleTime = millis();

  stepperX->interval = 2;
  stepperY->interval = 2;

  wakeMotion();

  xHomed = false;
  yHomed = false;
  
  return true;
}

bool MotionHandler2D::homeToolhead() {
  static int xHomedTime = 0;
  if (!xHomed) {
    // Serial.println("x not homed");
    if (xLimitReached) {
      xHomed = true;
      currentX = 0;
      xHomedTime = millis();
      return false; // return false as still need to home Y
    } else {
      if (millis() - stepperX->lastToggleTime >= stepperX->interval) {
        stepperX->step();
        return false;
      }
    }
  } else if (!yHomed && millis() - xHomedTime >= 1000) { // wait 1000ms after homing X before homing Y
    if (yLimitReached) {
      yHomed = true;
      currentY = 0;
      return true;
    } else {
      if (millis() - stepperY->lastToggleTime >= stepperY->interval) {
        stepperY->step();
        return false;
      }
    }
  }

  return false; // I don't see what case could cause this code to even run but there for redundancy
}


float* MotionHandler2D::getCurrentPosition() {
  static float position[2];
  position[0] = currentX;
  position[1] = currentY;
  return position;
}