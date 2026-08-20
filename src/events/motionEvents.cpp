#include "Arduino.h"
#include "events/motionEvents.hpp"

// going to be using whole steps for ease for this prototype
#define MM_PER_STEP 8/(360/1.8)  // 1.8 degrees per step so 360/1.8 steps in a revolution. one revolution is 8mm so one step is 8/200 = 0.04mm per step
#define STEPS_PER_MM (1/MM_PER_STEP) // 25 steps per mm


bool MotionHandler2D::setupMotionEvent(void* context) {
  MotionParameters* update = (MotionParameters*)context;
  targetX = update->x;
  targetY = update->y;

  // Calculate the distance to move in each direction
  float deltaX = targetX - currentX;
  float deltaY = targetX - currentY;

  queueX = 0;
  queueY = 0;

  // calculate the ratio of y steps to x steps, avoid division by zero. 
  // if its staight up or down, ratioY will be 1 and other code will manage
  // since currentX = targetX, queueX will be 0 and queueY will be incremented by 1 each time until it reaches targetY
  ratioY = (deltaX != 0) ? abs(deltaY / deltaX) : 1;  

  // Set the direction for each stepper based on the sign of deltaX and deltaY
  (*stepperX).setDirection(deltaX >= 0 ? 1 : 0);
  (*stepperY).setDirection(deltaY >= 0 ? 1 : 0);

  return true;
}

bool MotionHandler2D::handleMotion() {
  if (queueX < 1 && queueY < 1) {
    // proceed to next set of steps
    queueX++;
    queueY += ratioY; // increment the queue for y steps based on the ratio of deltaY to deltaX
  }

  unsigned long currentTime = millis();
  
  if (currentX != targetX ) {  // if the current x position is not equal to the target x position, we need to step the x motor
    if (currentTime - stepperX->lastToggleTime >= stepperX->interval) {
      if (queueX >= 1) {
        (*stepperX).step();
        queueX--;
      }
    }
  }

  if (currentY != targetY) {  // if the current y position is not equal to the target y position, we need to step the y motor
    if (currentTime - stepperY->lastToggleTime >= stepperY->interval) {
      if (queueY >= 1) {
        (*stepperY).step();
        queueY--;
      }
    }
  }
}