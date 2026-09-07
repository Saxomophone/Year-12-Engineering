#include <Arduino.h>
#include "eventScheduler.hpp"
#include "listeners/listener.hpp"
#include "pin.hpp"
#include "gcodeScheduler.hpp"
#include "main.hpp"

#define SERIAL_UPDATE_INTERVAL 500 //ms
#define ULTRASONIC_TRIGGER 3
#define ULTRASONIC_ECHO 4


// bool ultrasonicReadings[50]; // array to hold recent ultrasonic readings for smoothing

int steps_per_rev = 200; // 360/1.8
unsigned long prev_millis = 0;
unsigned long lastSerialUpdate = 0; // Declare and initialize lastSerialUpdate

// unsigned long prevUltrasonic_ms = 0;


int loopCounter = 0;

void panic();
bool off(void*);
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
  // setup pins
  // pinMode(THERMISTOR, INPUT);
  // pinMode(ELECTROMAGNET, OUTPUT);
  // pinMode(GREEN_LED, OUTPUT);
  // pinMode(SERVO, OUTPUT);
  // pinMode(TOOLHEAD_SWITCH, INPUT_PULLUP);
  // pinMode(TRIGGER_PIN, OUTPUT);
  // pinMode(ECHO_PIN, INPUT);

  Serial.begin(9600);
  Serial.println("Starting setup...");

  // nextInstruction(); 
  // nextInstruction(); 
  // nextInstruction(); 
  // nextInstruction(); 
  // nextInstruction(); 

  scheduler.initListeners();

  //init pins
  pinMode(ULTRASONIC_TRIGGER, OUTPUT);
  pinMode(ULTRASONIC_ECHO, OUTPUT);

  scheduler.push([](void* context){
    return false; // loops forever as it never returns true to tell eventScheduler it's finished
  }, nullptr);

  // Listener areaObstructionListener;
  // scheduler.addListener(&areaObstructed, area_obstructed, &areaObstructionListener);
}


void loop() {
  scheduler.processNext(); // Call this repeatedly in the loop to process events

  
  if (millis() - lastSerialUpdate >= SERIAL_UPDATE_INTERVAL) {
    lastSerialUpdate = millis();
    // insert debug statments here
  }

  if (scheduler.isEmpty()) {
    Serial.println("All events completed");
    off(nullptr); // ensure everything is turned off at the end
    while (true) {} // Stop further processing
  };

  loopCounter++;
}

// bool area_obstructed() {
//   if (millis() - prevUltrasonic_ms <= 1000) {
//     // Serial.println("Ultrasonic reading did not occur");
//     return false;
//   }
//   // Serial.println("Checking area obstruction with ultrasonic sensor...");

//   for (int i = 0; i <= 50; i++) {
      
//     if (i == 50) {
//       // Serial.println("checking time");
//       int trueCount = 0;
//       for (int j = 0; j < 50; j++) {
//         if (ultrasonicReadings[j]) { // if any reading detects an object, consider the area obstructed
//           trueCount++;
//         }
//       }
//       if (trueCount > 48) {
//         Serial.println("Area obstructed! Ultrasonic readings: " + String(trueCount));
//         prevUltrasonic_ms = millis();
//         return true;
//       } else {
//         // Serial.println("Area clear. Ultrasonic readings: " + String(trueCount));
//         prevUltrasonic_ms = millis();
//         return false;
//       }
//     }
    
//     // send ultrasonic pulse
//     digitalWrite(TRIGGER_PIN, LOW);
//     delayMicroseconds(5);
//     digitalWrite(TRIGGER_PIN, HIGH);
//     delayMicroseconds(10);
//     digitalWrite(TRIGGER_PIN, LOW);
    
//     long duration_us = pulseIn(ECHO_PIN, HIGH, ULTRASONIC_TIMEOUT); // wait for echo or timeout

//     ultrasonicReadings[i] = false; // no object detected
//     double distance_cm = duration_us * 0.034 / 2;
//     // Serial.println("Ultrasonic reading: " + String(distance_cm) + " cm, duration: " + String(duration_us) + " us");
//     if (duration_us > 0 && distance_cm < 30) { // if we got a valid reading and it's less than 30cm, consider it an obstruction
//       ultrasonicReadings[i] = true;
//     } else {
//       ultrasonicReadings[i] = false;
//     }
//   }
//   prevUltrasonic_ms = millis();
//   return false; // if we got here then the threshold was not met to consider there to be an obstruction
// }

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