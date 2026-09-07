#include <Arduino.h>
#include "eventScheduler.hpp"
#include "listeners/listener.hpp"
#include "pin.hpp"
#include "gcodeScheduler.hpp"
#include "main.hpp"
#include "listeners/ultrasonic.hpp"

#define SERIAL_UPDATE_INTERVAL 500 //ms

#define PANIC_OUTPUT_PIN 5
#define TRIGGER_PIN 3
#define ECHO_PIN 4
#define ULTRASONIC_TIMEOUT 30000 // microseconds
#define ULTRASONIC_SUCCESS_PROPORTION 0.96 // 48/50 as a default  

int steps_per_rev = 200; // 360/1.8
unsigned long prev_millis = 0;
unsigned long lastSerialUpdate = 0; // Declare and initialize lastSerialUpdate

int loopCounter = 0;

bool handleDelay(void* context);
bool resetDelay(void* context);

// Define a type for a function taking no args and returning void
typedef bool (*EventHandler)(void *context);


struct DelayState { 
  unsigned long startTime; 
  unsigned long duration; 
};

DelayState delayState1{0, 0};

EventScheduler scheduler;
bool areaObstructed = false;

GcodeSchedulePackage* gcodeInstructionPackage;

void setup() {
  Serial.begin(9600);
  Serial.println("Starting setup...");

  scheduler.initListeners();

  initUltrasonic(TRIGGER_PIN, ECHO_PIN, ULTRASONIC_TIMEOUT, ULTRASONIC_SUCCESS_PROPORTION);

  scheduler.addListener(&areaObstructed, check_area_obstructed);



  scheduler.push([](void* context){
    return false; // loops forever as it never returns true to tell eventScheduler it's finished
  }, nullptr);


}


void loop() {
  scheduler.processNext(); // Call this repeatedly in the loop to process events

  
  if (millis() - lastSerialUpdate >= SERIAL_UPDATE_INTERVAL) {
    lastSerialUpdate = millis();
    // insert debug statments here
  }

  if (areaObstructed) {
    Serial.println("Area obstructed! Stopping further processing.");
    digitalWrite(PANIC_OUTPUT_PIN, HIGH);
    while (true) {} // Stop further processing
  }

  if (scheduler.isEmpty()) {
    Serial.println("All events completed");
    while (true) {} // Stop further processing
  };

  loopCounter++;
}

// Handler for Delay
bool handleDelay(void* context) {
  DelayState* state = (DelayState*)context; // state is the context given cast to a DelayState struct
  // Serial.println(millis() - state->startTime);
  
  if (millis() - state->startTime >= state->duration) {
    return true; // Done
  }
  return false; // Keep waiting
}

bool resetDelay(void* context) {
  DelayState* state = (DelayState*)context;
  state->startTime = millis();
  return true; // This handler just resets the timer and is done immediately
}

float decimalRound(float input, int decimals) {
  float scale=pow(10,decimals);
  return round(input*scale)/scale;
}