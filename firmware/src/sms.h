#ifndef SMS_H
#define SMS_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include "config.h"

class SMSModule {
private:
  HardwareSerial smsSerial;
  
  String sendATCommand(String cmd, int timeout = 1000) {
    String response = "";
    smsSerial.println(cmd);
    
    unsigned long start = millis();
    while (millis() - start < timeout) {
      while (smsSerial.available()) {
        char c = smsSerial.read();
        response += c;
      }
      if (response.indexOf("OK") != -1 || response.indexOf("ERROR") != -1) {
        break;
      }
    }
    
    Serial.print("SMS Response: ");
    Serial.println(response);
    return response;
  }
  
public:
  SMSModule() : smsSerial(2) {}
  
  void begin() {
    pinMode(A7670_POWER, OUTPUT);
    digitalWrite(A7670_POWER, HIGH);
    
    smsSerial.begin(115200, SERIAL_8N1, A7670_RX, A7670_TX);
    delay(2000);
    
    // Initialize A7670 module
    sendATCommand("AT", 2000);
    delay(500);
    sendATCommand("AT+CMGF=1", 2000); // Text mode
    delay(500);
    sendATCommand("AT+CNMI=2,2,0,0,0", 2000); // SMS notification
    
    Serial.println("A7670 SMS module initialized");
  }
  
  bool sendSMS(String phoneNumber, String message) {
    Serial.print("Sending SMS to ");
    Serial.println(phoneNumber);
    
    smsSerial.print("AT+CMGS=\"");
    smsSerial.print(phoneNumber);
    smsSerial.println("\"");
    delay(500);
    
    smsSerial.print(message);
    delay(100);
    smsSerial.write(26); // Ctrl+Z
    
    delay(5000); // Wait for send
    
    String response = "";
    while (smsSerial.available()) {
      response += (char)smsSerial.read();
    }
    
    bool success = response.indexOf("OK") != -1;
    Serial.println(success ? "SMS sent successfully" : "SMS failed");
    
    return success;
  }
  
  void checkMessages() {
    if (smsSerial.available()) {
      String message = "";
      while (smsSerial.available()) {
        message += (char)smsSerial.read();
      }
      
      Serial.print("SMS Received: ");
      Serial.println(message);
      
      // Parse SMS commands
      if (message.indexOf("OPEN") != -1) {
        Serial.println("SMS Command: Open drain");
        // Trigger open drain action via callback or global flag
      } else if (message.indexOf("CLOSE") != -1) {
        Serial.println("SMS Command: Close drain");
        // Trigger close drain action
      } else if (message.indexOf("STATUS") != -1) {
        Serial.println("SMS Command: Request status");
        // Send status SMS
      }
    }
  }
};

#endif
