#pragma once

#include "Arduino.h"

// Having id not be part of context means that the context does not need to be unpacked to access the id. 
// The id will be accessed a lot so this is good for both speed and readability.
typedef bool (*ListenerFunction)(); // pointer to a function which returns a boolean

struct Listener {
  int id;
  bool* targetFlag; // pointer the global flag which the listener will toggle
  ListenerFunction listenerFunction; // function pointer to the condition check function which will determine whether the listener should toggle the flag
  bool lastConditionState; // stores the last state of the condition to detect changes
};

