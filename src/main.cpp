#include <Arduino.h>
#include "eventScheduler.hpp"
#include "listeners/listener.hpp"
#include "events/stepperEvents.hpp"
#include "pin.hpp"
#include "events/motionEvents.hpp"
#include "gcodeScheduler.hpp"

// #define THERMISTOR A0
// #define ELECTROMAGNET A4
// #define GREEN_LED A3
// #define SERVO 11
// #define TOOLHEAD_SWITCH A5
// #define TRIGGER_PIN A1
// #define ECHO_PIN A2
// #define ULTRASONIC_TIMEOUT 1000
// signal travels 0.33mm/us, so this gives max distance of 330mm when multiplied by 2 cause return trip which is about right and is a nice round number :3


#define STEPPER_Y_DIR 2
#define STEPPER_Y_STEP 3
#define STEPPER_Y_SLEEP 4
#define STEPPER_X_DIR 999
#define STEPPER_X_STEP 999
#define STEPPER_X_SLEEP 999


// physical limits of the area in mm
int X_LIMIT = 200;
int Y_LIMIT = 200;


// bool ultrasonicReadings[50]; // array to hold recent ultrasonic readings for smoothing

int steps_per_rev = 200; // 360/1.8
unsigned long prev_millis = 0;

// unsigned long prevUltrasonic_ms = 0;


int loopCounter = 0;

void panic();
bool off(void*);
bool toolhead_in_place();
bool handleDelay(void* context);
bool resetDelay(void* context);
bool driver_overheated();
bool area_obstructed();

// Define a type for a function taking no args and returning void
typedef bool (*EventHandler)(void *context);


struct DelayState { unsigned long startTime; unsigned long duration; };


EventScheduler scheduler;
bool stepperOverheated = false;
bool toolheadAttached = false;
bool areaObstructed = false;

StepperState stepperY{STEPPER_Y_STEP, STEPPER_Y_SLEEP, STEPPER_Y_DIR, 0, 2, STEPS, 0, 0, nullptr};
StepperState stepperX{STEPPER_X_STEP, STEPPER_X_SLEEP, STEPPER_X_DIR, 0, 2, STEPS, 0, 0, nullptr};

MotionHandler2D motionHandler{&stepperY, &stepperX};


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
  nextInstruction(); 
  nextInstruction(); 
  nextInstruction(); 
  nextInstruction(); 
  nextInstruction(); 


  // init
  // analogWrite(SERVO, 210); // ensures servo is in open position
  // digitalWrite(ELECTROMAGNET, LOW);
  // digitalWrite(GREEN_LED, HIGH);
  scheduler.initListeners();

  Serial.println("Starting setup...");


  // add listeners
  // Listener toolheadAttachedListener;
  // scheduler.addListener(&toolheadAttached, toolhead_in_place, &toolheadAttachedListener);

  // Listener stepperOverheatListener;
  // scheduler.addListener(&stepperOverheated, driver_overheated, &stepperOverheatListener);

  // Listener areaObstructionListener;
  // scheduler.addListener(&areaObstructed, area_obstructed, &areaObstructionListener);

  // // schedule events for testing 

  motionHandler.sleepMotion();

  scheduler.push([](void* context) {  // push takes a function (which needs to be unpacked from the void pointer) and an object on which to run the function
    MotionHandler2D* motionHandler = (MotionHandler2D*)context;
    return motionHandler->wakeMotion();
  }, &motionHandler); 

  scheduler.push([](void* context) {  // push takes a function (which needs to be unpacked from the void pointer) and an object on which to run the function
    MotionHandler2D* motionHandler = (MotionHandler2D*)context;
    return motionHandler->setupMotionEvent(new MotionParameters{100, 100, true});
  }, &motionHandler); 

  scheduler.push([](void* context) {
    MotionHandler2D* motionHandler = (MotionHandler2D*)context;
    return motionHandler->handleMotion();
  }, &motionHandler);
 
  scheduler.push([](void* context) {  // push takes a function (which needs to be unpacked from the void pointer) and an object on which to run the function
    MotionHandler2D* motionHandler = (MotionHandler2D*)context;
    return motionHandler->setupMotionEvent(new MotionParameters{0, 0, true});
  }, &motionHandler); 

  scheduler.push([](void* context) {
    MotionHandler2D* motionHandler = (MotionHandler2D*)context;
    return motionHandler->handleMotion();
  }, &motionHandler);

  
  // scheduler.push([](void* context) {  // push takes a function (which needs to be unpacked from the void pointer) and an object on which to run the function
  //   StepperState* stepper = (StepperState*)context;
  //   stepper->wakeStepper();
  //   return true;
  // }, &stepperY); 

  // scheduler.push([](void* context) {
  //   StepperState* stepper = (StepperState*)context;
  //   stepper->setupStepperEvent(new StepperState{STEPPER_Y_STEP, STEPPER_Y_SLEEP, STEPPER_Y_DIR, 0, 2, STEPS, 200, 0, nullptr});
  //   return true;
  // }, &stepperY);

  // scheduler.push([](void* context) {
  //   StepperState* stepper = (StepperState*)context;
  //   return stepper->handleStepper();
  // }, &stepperY);

  // scheduler.push([](void* context) {
  //   StepperState* stepper = (StepperState*)context;
  //   stepper->setupStepperEvent(new StepperState{STEPPER_Y_STEP, STEPPER_Y_SLEEP, STEPPER_Y_DIR, 1, 2, STEPS, 400, 0, nullptr});
  //   return true;
  // }, &stepperY);

  // scheduler.push([](void* context) {
  //   StepperState* stepper = (StepperState*)context;
  //   return stepper->handleStepper();
  // }, &stepperY);

  

  // // StepperResetPackage *setupPackage = new StepperResetPackage{&stepperYState, TRIGGER, &toolheadAttached, 2};
  // scheduler.push([](void*) {digitalWrite(STEPPER_Y_DIR, LOW); return true;}, nullptr);
  // scheduler.push(setupStepper, new StepperResetPackage{&stepperYState, TRIGGER, &toolheadAttached, 2}); // move stepper until toolhead is detected by switch
  // scheduler.push(handleStepper, &stepperYState);
  
  // scheduler.push([](void*) {digitalWrite(ELECTROMAGNET, HIGH); return true;}, nullptr); // turn on electromagnet to grab toolhead

  // DelayState delayState1 = { 0, 500 }; // 500ms delay
  // scheduler.push(resetDelay, &delayState1); // resets the timer for the delay handler
  // scheduler.push(handleDelay, &delayState1);

  // scheduler.push([](void*) {digitalWrite(STEPPER_Y_DIR, HIGH); return true;}, nullptr);
  // scheduler.push(setupStepper, new StepperResetPackage{&stepperYState, TIME, new unsigned long(5000), 2}); // move stepper for 5 seconds
  // scheduler.push(handleStepper, &stepperYState);

  // scheduler.push([](void* context) { //TODO: make updateDelay later to make this easier :sparkles:
  //   DelayState* state = (DelayState*)context;
  //   state->duration = 1000;
  //   return true;
  // }, &delayState1);
  // scheduler.push(handleDelay, &delayState1); // delay for 1 second with updated duration

  // scheduler.push([](void*) {analogWrite(SERVO, 90); return true;}, nullptr); // activate tool using servo (move to down position)

  // scheduler.push([](void* context) { //TODO: make updateDelay later to make this easier :sparkles:
  //   DelayState* state = (DelayState*)context;
  //   state->duration = 10000;
  //   return true;
  // }, &delayState1);
  // scheduler.push(handleDelay, &delayState1); // delay for 10 seconds with updated duration

  scheduler.push(off, nullptr);
}


void loop() {
  // analogWrite(SERVO, 90);
  // delay(500);
  // analogWrite(SERVO, 210);
  // delay(500);


  scheduler.processNext(); // Call this repeatedly in the loop to process events
  // Serial.println("StepperOverheated: " + String(stepperOverheated ? "True" : "False"));
  // Serial.println("Stepper Heat: " + String(analogRead(THERMISTOR)));
  // Serial.println("ToolheadAttached: " + String(toolheadAttached ? "True" : "False"));
  
  // if (stepperOverheated) {
  //   Serial.println("Stepper overheated! Current thermistor reading: " + String(analogRead(THERMISTOR)));
  //   panic();
  // }
  
  // if (areaObstructed) {
  //   Serial.println("Area obstructed!");
  //   panic();
  // }

  if (scheduler.isEmpty()) {
    Serial.println("All events completed");
    off(nullptr); // ensure everything is turned off at the end
    while (true) {} // Stop further processing
  };

  loopCounter++;
}

void panic() {
  // if (!toolheadAttached) {
  //   digitalWrite(ELECTROMAGNET, LOW);
  // }
  Serial.println("panic");
  // sleepStepper(&stepperYState);  
  // while (true) {
  //     digitalWrite(GREEN_LED, HIGH);
  //     delay(500);
  //     digitalWrite(GREEN_LED, LOW);
  //     delay(500);
  // } // halts program
}

bool off(void* context) {
  // digitalWrite(ELECTROMAGNET, LOW);
  // digitalWrite(GREEN_LED, LOW);
  stepperY.sleepStepper();
  return true;
}

// bool driver_overheated() {
//   int thermistor_read = analogRead(THERMISTOR);
//   return thermistor_read > 590;
// }

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


// void get_toolhead(EventScheduler scheduler) {
//   /*  Move toolhead away from y stepper
//       until switch is triggered
//       then turn on electromagnet to grab toolhead
//       then move toolhead towards y stepper
//       then activate tool using servo
//   */

//   scheduler.push([]() { analogWrite(SERVO, 210); return true; }); // ensures servo is in open position

//   scheduler.push([] () {wake_stepper; return true; });

//   scheduler.push([]() {digitalWrite(STEPPER_Y_DIR, LOW); return true; });

//   schedule_millis_reset(scheduler);

//   scheduler.push(
//     []() {
//       unsigned long current_millis = millis();
//       if (current_millis - prev_millis >= 2) {
//         prev_millis = current_millis;
//         stepper_y_state = (stepper_y_state == STEP_LOW) ? STEP_HIGH : STEP_LOW; // toggle step state
//         digitalWrite(STEPPER_Y_STEP, stepper_y_state); // toggle step pin
//       }
//       return toolhead_in_place();
//     }
//   );

//   scheduler.push([]() {digitalWrite(ELECTROMAGNET, HIGH); return true; });

//   schedule_millis_reset(scheduler);

//   scheduler.push(
//     []() {
//       unsigned long current_millis = millis();
//       if (current_millis - prev_millis >= 500) {
//         return true; // wait 500ms for electromagnet to fully engage
//       } else {
//         return false;
//       }
//     }
//   );

  
//   analogWrite(SERVO, 90);
//   delay(1000);

//   digitalWrite(STEPPER_Y_DIR, HIGH);

//   for (int i = 0; i < steps_per_rev*5; i++) {
//     digitalWrite(STEPPER_Y_STEP, LOW);
//     delay(2);
//     digitalWrite(STEPPER_Y_STEP, HIGH);
//     delay(2);
//   }
//}