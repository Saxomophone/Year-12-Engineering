#pragma once

#include "eventScheduler.hpp"
#include "events/motionEvents.hpp"
struct GcodeInstruction {
  bool toolHeadDown;
  float x;
  float y;
};

struct GcodeSchedulePackage {
  GcodeInstruction instruction;
  EventScheduler* scheduler;
  MotionHandler2D* motionHandler;
};

String nextInstruction();

GcodeInstruction preProcessInstruction();

void scheduleInstruction(GcodeSchedulePackage* package);

bool setupMotionInstruction(void* context);

extern int numInstructions;