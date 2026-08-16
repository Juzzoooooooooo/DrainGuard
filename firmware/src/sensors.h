#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include "config.h"

class SensorManager {
private:
  long duration;
  float distance;
  
public:
  void begin() {
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    Serial.println("Ultrasonic sensor initialized");
  }
  
  float readUltrasonic() {
    // Clear trigger
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    
    // Send pulse
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    // Read echo
    duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout
    
    if (duration == 0) {
      Serial.println("Ultrasonic sensor timeout");
      return -1;
    }
    
    // Calculate distance in cm
    distance = duration * 0.034 / 2;
    
    // Validate reading
    if (distance > MAX_DISTANCE || distance < 2) {
      Serial.println("Ultrasonic reading out of range");
      return -1;
    }
    
    return distance;
  }
  
  float calculateWaterLevel(float distance) {
    if (distance < 0) return 0;
    
    // Water level = tank height - distance from sensor
    float waterLevel = TANK_HEIGHT - distance;
    
    // Clamp between 0 and tank height
    if (waterLevel < 0) waterLevel = 0;
    if (waterLevel > TANK_HEIGHT) waterLevel = TANK_HEIGHT;
    
    return waterLevel;
  }
  
  float getWaterPercentage(float waterLevel) {
    return (waterLevel / TANK_HEIGHT) * 100.0;
  }
};

#endif
