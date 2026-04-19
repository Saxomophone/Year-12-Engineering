#include <Arduino.h>

#define THERMISTOR A0
#define ELECTROMAGNET A4
#define GREEN_LED A3
#define SERVO 11
#define STEPPER_Y_DIR 2
#define STEPPER_Y_STEP 3
#define STEPPER_Y_SLEEP 4
#define TOOLHEAD_SWITCH A5

int steps_per_rev = 200; // 360/1.8
unsigned long prev_millis = 0;

void panic();
bool off(void*);
bool toolhead_in_place();
void sleep_stepper();
bool wake_stepper(void*);
bool handleDelay(void* context);
bool handleStepper(void* context);
bool resetDelay(void* context);
bool resetStepper(void* context);
void activateTrigger(void* context);

// Define a type for a function taking no args and returning void
typedef bool (*EventHandler)(void *context);

// Having id not be part of context means that the context does not need to be unpacked to access the id. 
// The id will be accessed a lot so this is good for both speed and readability.
typedef bool (*ListenerFunction)();

struct Listener {
  int id;
  bool* targetFlag; // pointer the global flag which the listener will toggle
  ListenerFunction listenerFunction; // function pointer to the condition check function which will determine whether the listener should toggle the flag
  bool lastConditionState; // stores the last state of the condition to detect changes
};

class EventScheduler {
  private:
    static const int MAX_EVENTS = 20;
    static const int MAX_LISTENERS = 5;

    EventHandler handlers[MAX_EVENTS]; // Array to hold event handlers
    void* contexts[MAX_EVENTS]; // Array to hold contexts for each event
    int eventCount = 0; // Number of scheduled events

    void* listeners[MAX_LISTENERS]; // Array to hold contexts for listeners
    int listenerCount = 0; // Number of registered listeners

  public:
    void push(EventHandler event, void* context) {
      if (eventCount < MAX_EVENTS) {
        handlers[eventCount] = event;
        contexts[eventCount] = context;
        eventCount++;
      } else {
        Serial.println("Event queue full");
      }
    }

    void processNext() {
      callListeners(); // Check listeners before processing the next event
      
      if (eventCount > 0) {
        bool isFinished = handlers[0](contexts[0]); // Call the first event handler with its context

        if (isFinished) { // If the event signals it's finished, remove it from the queue

          // Shift remaining events forward
          for (int i = 1; i < eventCount; i++) {
            handlers[i - 1] = handlers[i];
            contexts[i - 1] = contexts[i];
          }
          eventCount--;
        }
      }
    }

    bool isEmpty() { return eventCount == 0; }

    // Initialize all listeners to default state
    void initListeners() { 
      for (int i = 0; i < MAX_LISTENERS; i++) {
        listeners[i] = nullptr;
      }
    }

    int addListener(bool* targetFlag, ListenerFunction listenerFunction, Listener* listener) {
      if (listenerCount < MAX_LISTENERS) {
        for (int i = 0; i < MAX_LISTENERS; i++) {
          if (listeners[i] == nullptr) {

            // setup listener by assigning values to state struct
            listener->id = i; // assign id to state
            listener->listenerFunction = listenerFunction; 
            *targetFlag = listenerFunction();
            listener->targetFlag = targetFlag;
            listener->lastConditionState = targetFlag;

            // add listener to array
            listeners[i] = listener;
            listenerCount++;
            return i; // return id of listener for reference
          }
        }
      } else {
        Serial.println("Listener limit reached");
      }
    }

    void removeListener(int id) {
      for (int i = 0; i < MAX_LISTENERS; i++) {
        Listener* listener = (Listener*)listeners[i];
        if (listener->id == id) {
          listeners[i] = nullptr;
          listenerCount--;
          break;
        }
      }
    }

    void callListeners() {
      for (int i = 0; i < MAX_LISTENERS; i++) {
        if (listeners[i] != nullptr) {
          Listener* listener = (Listener*)listeners[i];

          // define currentConditionState here for readability and so that the listener function is only called once
          bool currentConditionState = listener->listenerFunction();

          // only update if condition has changed (prevents unecessary writes)
          if (currentConditionState != listener->lastConditionState) {
            *(listener->targetFlag) = currentConditionState; // update target flag to new state
            listener->lastConditionState = currentConditionState; // update last condition state

            // Optional: Print state change for debugging
            Serial.print("Listener ");
            Serial.print(listener->id);
            Serial.print(": ");
            Serial.println(listener->targetFlag ? "True" : "False");
          }
        }
      }
    }
}; 

struct DelayState { unsigned long startTime; unsigned long duration; };

enum StepperStopCondition { TIME, STEPS, TRIGGER };
enum PinState { PIN_LOW, PIN_HIGH };

struct StepperState {
  // 4 values needed to run the stepper
  int pin;
  PinState pinState;
  unsigned long lastToggleTime;
  unsigned int interval; // ms between toggles

  // stop condition
  StepperStopCondition stopCondition;

  // values for stop conditions
  unsigned long startTime = millis(); // for TIME condition
  unsigned long duration; // for TIME condition

  int stepsTaken = 0; // for STEPS condition
  int targetSteps; // for STEPS condition

  bool triggerMet = false; // for TRIGGER condition
};


EventScheduler scheduler;
StepperState stepperYState;
bool stepperOverheated = false;
bool toolheadAttached = false;
bool areaObstructed = false;

void setup() {
  // setup pins
  pinMode(THERMISTOR, INPUT);
  pinMode(ELECTROMAGNET, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(SERVO, OUTPUT);
  pinMode(STEPPER_Y_DIR, OUTPUT);
  pinMode(STEPPER_Y_STEP, OUTPUT);
  pinMode(STEPPER_Y_SLEEP, OUTPUT);
  pinMode(TOOLHEAD_SWITCH, INPUT_PULLUP);

  Serial.begin(9600);

  // init
  sleep_stepper();
  analogWrite(SERVO, 210); // ensures servo is in open position
  digitalWrite(ELECTROMAGNET, LOW);
  digitalWrite(GREEN_LED, HIGH);

  // define stepper states
  stepperYState.pin = STEPPER_Y_STEP;
  stepperYState.pinState = PIN_LOW;


  // initialise stepper motor
  scheduler.push(wake_stepper, nullptr);
  scheduler.push([](void*) {digitalWrite(STEPPER_Y_DIR, LOW); return true;}, nullptr);

  stepperYState.stopCondition = STEPS;
  stepperYState.interval = 2; // 2ms between steps
  stepperYState.targetSteps = steps_per_rev*10; // 5 revolutions
  scheduler.push(handleStepper, &stepperYState);

  
  DelayState delayState1 = { 0, 5000 }; // 2 second delay
  scheduler.push(resetDelay, &delayState1); // resets the timer for the delay handler
  scheduler.push(handleDelay, &delayState1);

  scheduler.push([](void*) {digitalWrite(STEPPER_Y_DIR, HIGH); return true;}, nullptr);
  scheduler.push(resetStepper, &stepperYState); // reset steps taken for next run
  scheduler.push(handleStepper, &stepperYState); //interval and distance same so just no need to redo

  scheduler.push(off, nullptr);
}


void loop() {
  // int thermistor_read = analogRead(THERMISTOR);
  // Serial.println(thermistor_read);
  // delay(500);

  // digitalWrite(ELECTROMAGNET, HIGH);
  // delay(5000);
  // digitalWrite(ELECTROMAGNET, LOW);
  // delay(5000);

  // analogWrite(SERVO, 90);
  // delay(500);
  // analogWrite(SERVO, 210);
  // delay(500);

  // wake_stepper();

  // Serial.println(toolhead_is_attached());
  // delay(500);
  // pulse_electromagnet();
  // delay(5000);



  scheduler.processNext(); // Call this repeatedly in the loop to process events
  if (scheduler.isEmpty()) {
    Serial.println("All events completed");
    while (true) {} // Stop further processing
  };
}

void panic() {
  digitalWrite(ELECTROMAGNET, LOW);
  digitalWrite(GREEN_LED, HIGH);
  Serial.println("panic");
  while (true) {} // halts program
}

bool off(void*) {
  digitalWrite(ELECTROMAGNET, LOW);
  digitalWrite(GREEN_LED, LOW);
  sleep_stepper();
  return true;
}

bool toolhead_in_place() {
  return digitalRead(TOOLHEAD_SWITCH) == LOW;
}

bool overheated() {
  int thermistor_read = analogRead(THERMISTOR);
  return thermistor_read > 600;
}

void sleep_stepper() {
  digitalWrite(STEPPER_Y_SLEEP, LOW); // sleep pin is active low
}

bool wake_stepper(void*) {
  digitalWrite(STEPPER_Y_SLEEP, HIGH); // sleep pin is active low
  delay(1); // I don't like using a blocking function but I need to block the stepper and its only a short delay called right at the start. I also don't want to bother cause it really doesn't matter.
  return true;
}

// Handler for Delay
bool handleDelay(void* context) {
  DelayState* state = (DelayState*)context; // state is the context given cast to a DelayState struct
  Serial.println(millis() - state->startTime);
  
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

bool resetStepper(void* context) {
  StepperState* state = (StepperState*)context;
  state->stepsTaken = 0;
  state->lastToggleTime = millis();
  state->triggerMet = false;
  digitalWrite(state->pin, LOW); // ensure pin starts low
  state->pinState = PIN_LOW;
  return true;
}

// Handler for Stepper
bool handleStepper(void* context) {
  StepperState* state = (StepperState*)context;
  unsigned long currentTime = millis();

  if (currentTime - state->lastToggleTime >= state->interval) {
    // Toggle pin
    digitalWrite(state->pin, !digitalRead(state->pin));
    state->lastToggleTime = currentTime;
    
    state->pinState = (state->pinState == PIN_LOW) ? PIN_HIGH : PIN_LOW; // switches between HIGH and LOW
    
    if (state->pinState == PIN_HIGH) { // only count steps on one edge (e.g. rising)
      state->stepsTaken++;
    }
  }

  switch (state->stopCondition) {
    case TIME:
      if (currentTime - state->startTime >= state->duration) {
        return true; // Done
      }
      break;

    case STEPS:
      if (state->stepsTaken >= state->targetSteps) {
        return true; // Done
      }
      break;

    case TRIGGER:
      return state->triggerMet; // if true the trigger is met and the function returns with as completed otherwise false and it continues
  }

  return false; // Keep waiting
}

void activateTrigger(void* context) {
  StepperState* state = (StepperState*)context;
  state->triggerMet = true;
}

//function for demonstration
void scheduleGetToolhead(EventScheduler scheduler) {
  /*  Move toolhead away from y stepper
      until switch is triggered
      then turn on electromagnet to grab toolhead
      then move toolhead towards y stepper
      then activate tool using servo
  */
  scheduler.push([](void*) { analogWrite(SERVO, 210); return true; }, nullptr); // ensures servo is in up position

  
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
// }