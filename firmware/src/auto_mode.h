#ifndef AUTO_MODE_H
#define AUTO_MODE_H

#include <Arduino.h>
#include "config.h"

class AutoModeController {
private:
  bool enabled;
  bool isOperating;
  float detectionRange;
  unsigned long lastOperation;
  unsigned long operationCooldown;
  
public:
  AutoModeController() {
    enabled = false;
    isOperating = false;
    detectionRange = 20.0; // cm
    lastOperation = 0;
    operationCooldown = 10000; // 10 seconds between operations
  }
  
  void enable() {
    enabled = true;
    isOperating = false;
    Serial.println("Auto mode: ENABLED");
  }
  
  void disable() {
    enabled = false;
    isOperating = false;
    Serial.println("Auto mode: DISABLED");
  }
  
  bool isEnabled() {
    return enabled;
  }
  
  bool isCurrentlyOperating() {
    return isOperating;
  }
  
  void setDetectionRange(float range) {
    detectionRange = range;
    Serial.printf("Auto mode: Detection range set to %.1f cm\n", range);
  }
  
  float getDetectionRange() {
    return detectionRange;
  }
  
  bool shouldOperate(float currentDistance) {
    if (!enabled) return false;
    if (isOperating) return false;
    if (currentDistance <= 0) return false;
    
    // Check cooldown period
    if (millis() - lastOperation < operationCooldown) {
      return false;
    }
    
    // Check if object detected within range
    if (currentDistance <= detectionRange) {
      Serial.printf("Auto mode: Object detected at %.1f cm\n", currentDistance);
      return true;
    }
    
    return false;
  }
  
  void startOperation() {
    isOperating = true;
    lastOperation = millis();
    Serial.println("Auto mode: Starting automatic operation");
  }
  
  void completeOperation() {
    isOperating = false;
    Serial.println("Auto mode: Operation completed");
  }
};

#endif
