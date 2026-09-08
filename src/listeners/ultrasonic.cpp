#include "listeners/ultrasonic.hpp"
#include "eventScheduler.hpp"

#define NUM_ULTRASONIC_READINGS 200

int triggerPin;
int echoPin;
int ultrasonicTimeout;
float filterSuccessProportion = 0.98; // proportion of readings that need to be obstructed to return obstructed
unsigned long prevUltrasonic_ms = 0;
bool ultrasonicReadings[NUM_ULTRASONIC_READINGS]; // array to hold recent ultrasonic readings for smoothing


void initUltrasonic(int trigger, int echo, int timeout, float successProportion) {
  // triggerPin(trigger), echoPin(echo), prevUltrasonic_ms(0), timeout(timeout), filterSuccessProportion(successProportion) {
  triggerPin = trigger;
  echoPin = echo;
  ultrasonicTimeout = timeout;
  filterSuccessProportion = successProportion;

  pinMode(triggerPin, OUTPUT);
  pinMode(echoPin, INPUT);
}

bool check_area_obstructed() {
  if (millis() - prevUltrasonic_ms <= 1000) {
    // Serial.println("Ultrasonic reading did not occur");
    return false;
  }
  // Serial.println("Checking area obstruction with ultrasonic sensor...");

  for (int i = 0; i <= NUM_ULTRASONIC_READINGS; i++) {
      
    if (i == NUM_ULTRASONIC_READINGS) {
      // Serial.println("checking time");
      int trueCount = 0;
      for (int j = 0; j < NUM_ULTRASONIC_READINGS; j++) {
        if (ultrasonicReadings[j]) { // if any reading detects an object, consider the area obstructed
          trueCount++;
        }
      }
      Serial.print("trueCount: ");
      Serial.println(String(trueCount));
      if (trueCount >= int((NUM_ULTRASONIC_READINGS*filterSuccessProportion))) {
        prevUltrasonic_ms = millis();
        Serial.println("trueCount > threshold");
        return true;
      } else {
        // Serial.println("Area clear. Ultrasonic readings: " + String(trueCount));
        prevUltrasonic_ms = millis();
        return false;
      }
    }
    
    // send ultrasonic pulse
    digitalWrite(triggerPin, LOW);
    delayMicroseconds(5);
    digitalWrite(triggerPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(triggerPin, LOW);
    
    long duration_us = pulseIn(echoPin, HIGH, ultrasonicTimeout); // wait for echo or timeout

    ultrasonicReadings[i] = false; // no object detected
    double distance_cm = duration_us * 0.034 / 2;
    // Serial.println("Ultrasonic reading: " + String(distance_cm) + " cm, duration: " + String(duration_us) + " us");
    if (duration_us > 0 && distance_cm < 30) { // if we got a valid reading and it's less than 30cm, consider it an obstruction
      ultrasonicReadings[i] = true;
    } else {
      ultrasonicReadings[i] = false;
    }
  }
  prevUltrasonic_ms = millis();
  return false; // if we got here then the threshold was not met to consider there to be an obstruction
}