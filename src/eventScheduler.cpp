#include "Arduino.h"
#include "eventScheduler.hpp"


void EventScheduler::push(EventHandler event, void* context) {
  if (eventCount < MAX_EVENTS) {
    handlers[eventCount] = event;
    contexts[eventCount] = context;
    eventCount++;
  } else {
    Serial.println("Event queue full");
  }
}

void EventScheduler::processNext() {
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

bool EventScheduler::isEmpty() { return eventCount == 0; }

void EventScheduler::initListeners() { 
  for (int i = 0; i < MAX_LISTENERS; i++) {
    listeners[i] = nullptr;
  }
}

int EventScheduler::addListener(bool* targetFlag, ListenerFunction listenerFunction, Listener* listener) {
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
  return -1; // indicates failure to add listener
}

void EventScheduler::removeListener(int id) {
  for (int i = 0; i < MAX_LISTENERS; i++) {
    Listener* listener = (Listener*)listeners[i];
    if (listener->id == id) {
      listeners[i] = nullptr;
      listenerCount--;
      break;
    }
  }
}

void EventScheduler::callListeners() {
  for (int i = 0; i < MAX_LISTENERS; i++) {
    if (listeners[i] != nullptr) {
      Listener* listener = (Listener*)listeners[i];

      // define currentConditionState here for readability and so that the listener function is only called once
      bool currentConditionState = listener->listenerFunction();

      // only update if condition has changed (prevents unecessary writes)
      if (currentConditionState != listener->lastConditionState) {
        *(listener->targetFlag) = currentConditionState; // update target flag to new state
        listener->lastConditionState = currentConditionState; // update last condition state
      }
    }
  }
}