#include "Arduino.h"
#include "gcodeScheduler.hpp"


const String gcodeInstructions[] = {
  "G0 X65.544 Y221.164",
  "G1 X174.422 Y221.164",
  "G1 X174.422 Y117.161",
  "G1 X65.544 Y117.161",
  "G1 X65.544 Y221.164",
};

String nextInstruction() {
  static int index = 0;
  if (index < sizeof(gcodeInstructions) / sizeof(gcodeInstructions[0])) {
    Serial.println("Processing: " + gcodeInstructions[index]);
    index++;
    return gcodeInstructions[index - 1];
  }
}

