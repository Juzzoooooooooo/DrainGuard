#ifndef MOTORS_H
#define MOTORS_H

#include <Arduino.h>
#include "config.h"

class MotorController {
private:
  bool isOpen;
  
  void setMotor(int in1, int in2, int pwmPin, int speed, bool forward) {
    digitalWrite(MOTOR_STBY, HIGH); // Enable motors
    
    if (speed == 0) {
      digitalWrite(in1, LOW);
      digitalWrite(in2, LOW);
    } else {
      if (forward) {
        digitalWrite(in1, HIGH);
        digitalWrite(in2, LOW);
      } else {
        digitalWrite(in1, LOW);
        digitalWrite(in2, HIGH);
      }
      analogWrite(pwmPin, speed);
    }
  }
  
public:
  void begin() {
    pinMode(MOTOR_AIN1, OUTPUT);
    pinMode(MOTOR_AIN2, OUTPUT);
    pinMode(MOTOR_BIN1, OUTPUT);
    pinMode(MOTOR_BIN2, OUTPUT);
    pinMode(MOTOR_PWMA, OUTPUT);
    pinMode(MOTOR_PWMB, OUTPUT);
    pinMode(MOTOR_STBY, OUTPUT);
    
    digitalWrite(MOTOR_STBY, LOW); // Standby mode initially
    isOpen = false;
    
    Serial.println("Motor controller initialized");
  }
  
  void openDrain() {
    if (isOpen) {
      Serial.println("Drain already open");
      return;
    }
    
    Serial.println("Opening drain...");
    
    // Run motors forward
    setMotor(MOTOR_AIN1, MOTOR_AIN2, MOTOR_PWMA, MOTOR_SPEED, true);
    setMotor(MOTOR_BIN1, MOTOR_BIN2, MOTOR_PWMB, MOTOR_SPEED, true);
    
    delay(DRAIN_OPEN_TIME);
    
    // Stop motors
    stopMotors();
    isOpen = true;
    
    Serial.println("Drain opened");
  }
  
  void closeDrain() {
    if (!isOpen) {
      Serial.println("Drain already closed");
      return;
    }
    
    Serial.println("Closing drain...");
    
    // Run motors backward
    setMotor(MOTOR_AIN1, MOTOR_AIN2, MOTOR_PWMA, MOTOR_SPEED, false);
    setMotor(MOTOR_BIN1, MOTOR_BIN2, MOTOR_PWMB, MOTOR_SPEED, false);
    
    delay(DRAIN_CLOSE_TIME);
    
    // Stop motors
    stopMotors();
    isOpen = false;
    
    Serial.println("Drain closed");
  }
  
  void stopMotors() {
    setMotor(MOTOR_AIN1, MOTOR_AIN2, MOTOR_PWMA, 0, true);
    setMotor(MOTOR_BIN1, MOTOR_BIN2, MOTOR_PWMB, 0, true);
    digitalWrite(MOTOR_STBY, LOW);
  }
  
  bool getDrainState() {
    return isOpen;
  }
};

#endif
