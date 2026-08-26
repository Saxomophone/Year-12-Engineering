#pragma once

#include "Arduino.h"
#include "events/stepperEvents.hpp"
#include "listeners/listener.hpp"


class EventScheduler {
  private:
    typedef bool (*EventHandler)(void *context);
    
    static const int MAX_EVENTS = 30;
    static const int MAX_LISTENERS = 5;

    EventHandler handlers[MAX_EVENTS]; // Array to hold event handlers
    void* contexts[MAX_EVENTS]; // Array to hold contexts for each event
    int eventCount = 0; // Number of scheduled events

    void* listeners[MAX_LISTENERS]; // Array to hold contexts for listeners
    int listenerCount = 0; // Number of registered listeners

  public:
    void push(EventHandler event, void* context);

    void processNext();

    bool isEmpty();
    // Initialize all listeners to default state
    void initListeners();

    int addListener(bool* targetFlag, ListenerFunction listenerFunction);

    void removeListener(int id);

    void callListeners();
}; 