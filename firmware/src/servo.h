#ifndef SERVO_H
#define SERVO_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "config.h"

class ServoController {
private:
  Adafruit_PWMServoDriver pwm;
  bool initialized;
  
  // Current positions
  uint16_t basePos;
  uint16_t shoulderPos;
  uint16_t elbowPos;
  uint16_t gripperPos;
  
  // Smooth servo movement
  void moveServoSmooth(uint8_t channel, uint16_t targetPos, int delayMs = 10) {
    uint16_t currentPos = getCurrentPosition(channel);
    
    if (targetPos > currentPos) {
      for (uint16_t pos = currentPos; pos <= targetPos; pos++) {
        pwm.setPWM(channel, 0, pos);
        delay(delayMs);
      }
    } else {
      for (uint16_t pos = currentPos; pos >= targetPos; pos--) {
        pwm.setPWM(channel, 0, pos);
        delay(delayMs);
      }
    }
    
    updatePosition(channel, targetPos);
  }
  
  uint16_t getCurrentPosition(uint8_t channel) {
    switch (channel) {
      case SERVO_BASE: return basePos;
      case SERVO_SHOULDER: return shoulderPos;
      case SERVO_ELBOW: return elbowPos;
      case SERVO_GRIPPER: return gripperPos;
      default: return 0;
    }
  }
  
  void updatePosition(uint8_t channel, uint16_t pos) {
    switch (channel) {
      case SERVO_BASE: basePos = pos; break;
      case SERVO_SHOULDER: shoulderPos = pos; break;
      case SERVO_ELBOW: elbowPos = pos; break;
      case SERVO_GRIPPER: gripperPos = pos; break;
    }
  }
  
public:
  ServoController() : pwm(PCA9685_ADDRESS) {
    initialized = false;
    basePos = 330;
    shoulderPos = 150;
    elbowPos = 300;
    gripperPos = 410;
  }
  
  bool begin() {
    Wire.begin(PCA9685_SDA, PCA9685_SCL);
    
    pwm.begin();
    pwm.setPWMFreq(SERVO_FREQ);
    
    delay(100);
    
    // Set to home position
    pwm.setPWM(SERVO_BASE, 0, basePos);
    pwm.setPWM(SERVO_SHOULDER, 0, shoulderPos);
    pwm.setPWM(SERVO_ELBOW, 0, elbowPos);
    pwm.setPWM(SERVO_GRIPPER, 0, gripperPos);
    
    initialized = true;
    Serial.println("PCA9685 Servo controller initialized");
    
    return true;
  }
  
  bool isInitialized() {
    return initialized;
  }
  
  // Basic servo control
  void setServo(uint8_t channel, uint16_t position) {
    if (!initialized) {
      Serial.println("Servo controller not initialized");
      return;
    }
    
    pwm.setPWM(channel, 0, position);
    updatePosition(channel, position);
  }
  
  void setServoSmooth(uint8_t channel, uint16_t position, int speed = 10) {
    if (!initialized) {
      Serial.println("Servo controller not initialized");
      return;
    }
    
    moveServoSmooth(channel, position, speed);
  }
  
  // Pre-defined positions for drain operation
  void moveToHomePosition() {
    Serial.println("Moving to home position");
    setServoSmooth(SERVO_BASE, 330, 10);
    delay(100);
    setServoSmooth(SERVO_SHOULDER, 150, 10);
    delay(100);
    setServoSmooth(SERVO_ELBOW, 300, 10);
    delay(100);
    setServoSmooth(SERVO_GRIPPER, 410, 10);
  }
  
  void openDrainWithArm() {
    Serial.println("Opening drain with robotic arm");
    
    // Move base to position
    setServoSmooth(SERVO_BASE, 250, 10);
    delay(500);
    
    // Extend arm
    setServoSmooth(SERVO_SHOULDER, 380, 10);
    delay(500);
    setServoSmooth(SERVO_ELBOW, 380, 10);
    delay(500);
    
    // Open gripper
    setServoSmooth(SERVO_GRIPPER, 510, 10);
    delay(1000);
    
    Serial.println("Drain opened");
  }
  
  void closeDrainWithArm() {
    Serial.println("Closing drain with robotic arm");
    
    // Close gripper
    setServoSmooth(SERVO_GRIPPER, 410, 10);
    delay(500);
    
    // Retract arm
    setServoSmooth(SERVO_ELBOW, 300, 10);
    delay(500);
    setServoSmooth(SERVO_SHOULDER, 150, 10);
    delay(500);
    
    // Return base to home
    setServoSmooth(SERVO_BASE, 330, 10);
    delay(500);
    
    Serial.println("Drain closed");
  }
  
  // Demo sequence from reference code
  void runDemoSequence() {
    Serial.println("Running demo sequence");
    
    // Sequence 1: Lower and grab
    for (int pos = 330; pos >= 250; pos--) {
      setServo(SERVO_BASE, pos);
      delay(10);
    }
    
    for (int pos = 150; pos <= 380; pos++) {
      setServo(SERVO_SHOULDER, pos);
      delay(10);
    }
    
    for (int pos = 300; pos <= 380; pos++) {
      setServo(SERVO_ELBOW, pos);
      delay(10);
    }
    
    for (int pos = 410; pos <= 510; pos++) {
      setServo(SERVO_GRIPPER, pos);
      delay(10);
    }
    
    delay(2000);
    
    // Sequence 2: Release and retract
    for (int pos = 510; pos > 410; pos--) {
      setServo(SERVO_GRIPPER, pos);
      delay(10);
    }
    
    for (int pos = 380; pos > 300; pos--) {
      setServo(SERVO_ELBOW, pos);
      delay(10);
    }
    
    for (int pos = 380; pos > 150; pos--) {
      setServo(SERVO_SHOULDER, pos);
      delay(10);
    }
    
    for (int pos = 250; pos < 450; pos++) {
      setServo(SERVO_BASE, pos);
      delay(10);
    }
    
    // Continue sequence...
    for (int pos = 150; pos <= 380; pos++) {
      setServo(SERVO_SHOULDER, pos);
      delay(10);
    }
    
    for (int pos = 300; pos <= 380; pos++) {
      setServo(SERVO_ELBOW, pos);
      delay(10);
    }
    
    for (int pos = 410; pos <= 510; pos++) {
      setServo(SERVO_GRIPPER, pos);
      delay(10);
    }
    
    for (int pos = 510; pos > 410; pos--) {
      setServo(SERVO_GRIPPER, pos);
      delay(10);
    }
    
    for (int pos = 380; pos > 300; pos--) {
      setServo(SERVO_ELBOW, pos);
      delay(10);
    }
    
    for (int pos = 380; pos > 150; pos--) {
      setServo(SERVO_SHOULDER, pos);
      delay(10);
    }
    
    for (int pos = 450; pos > 330; pos--) {
      setServo(SERVO_BASE, pos);
      delay(10);
    }
    
    Serial.println("Demo sequence complete");
  }
  
  // Individual servo controls for API
  void setBase(uint16_t position) {
    position = constrain(position, SERVO_BASE_MIN, SERVO_BASE_MAX);
    setServoSmooth(SERVO_BASE, position);
  }
  
  void setShoulder(uint16_t position) {
    position = constrain(position, SERVO_SHOULDER_MIN, SERVO_SHOULDER_MAX);
    setServoSmooth(SERVO_SHOULDER, position);
  }
  
  void setElbow(uint16_t position) {
    position = constrain(position, SERVO_ELBOW_MIN, SERVO_ELBOW_MAX);
    setServoSmooth(SERVO_ELBOW, position);
  }
  
  void setGripper(uint16_t position) {
    position = constrain(position, SERVO_GRIPPER_MIN, SERVO_GRIPPER_MAX);
    setServoSmooth(SERVO_GRIPPER, position);
  }
  
  // Get current positions
  void getCurrentPositions(uint16_t &base, uint16_t &shoulder, uint16_t &elbow, uint16_t &gripper) {
    base = basePos;
    shoulder = shoulderPos;
    elbow = elbowPos;
    gripper = gripperPos;
  }
};

#endif
