#include "Arduino.h"
#include "gcodeScheduler.hpp"
#include "eventScheduler.hpp"
#include "events/motionEvents.hpp"


const String gcodeInstructions[] = {
  "G0 X65.544 Y221.164",
  "G1 X174.422 Y221.164",
  "G1 X174.422 Y117.161",
  "G1 X65.544 Y117.161",
  "G1 X65.544 Y221.164",
};

int numInstructions = sizeof(gcodeInstructions) / sizeof(gcodeInstructions[0]);

String nextInstruction() {
  static unsigned int index = 0;
  if (index < sizeof(gcodeInstructions) / sizeof(gcodeInstructions[0])) {
    index++;
    return gcodeInstructions[index - 1];
  } else {
    return "END";
  }
}

GcodeInstruction preProcessInstruction() {
  String instructionString = nextInstruction();
  GcodeInstruction instruction;
  
  int gIndex = instructionString.indexOf('G');
  int xIndex = instructionString.indexOf('X');
  int yIndex = instructionString.indexOf('Y');

  if (gIndex == -1 || xIndex == -1 || yIndex == -1) {
    Serial.println("invalid gcode");
  }
  instruction.toolHeadDown = instructionString.substring(gIndex+1, xIndex).toInt() ? true : false;
  instruction.x = instructionString.substring(xIndex+1, yIndex).toFloat();
  instruction.y = instructionString.substring(yIndex+1).toFloat();
  return instruction;
}

void scheduleInstruction(GcodeSchedulePackage* package) {

  package->scheduler->push([](void* context) {  // push takes a function (which needs to be unpacked from the void pointer) and an object on which to run the function
    GcodeSchedulePackage* package = (GcodeSchedulePackage*)context;
    return setupMotionInstruction(package);
  }, package->motionHandler); 

  package->scheduler->push([](void* context) {
    MotionHandler2D* motionHandler = (MotionHandler2D*)context;
    return motionHandler->handleMotion();
  }, package->motionHandler);
}

bool setupMotionInstruction(void* context) {
  GcodeSchedulePackage* package = (GcodeSchedulePackage*)context;
  GcodeInstruction instruction = package->instruction;
  MotionHandler2D* motionHandler = package->motionHandler; 
  return motionHandler->setupMotionEvent(new MotionParameters{instruction.x, instruction.y, instruction.toolHeadDown});
}

