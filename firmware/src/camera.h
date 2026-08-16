#ifndef CAMERA_H
#define CAMERA_H

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"

// Note: ESP32-CAM runs as a separate module with its own firmware
// This class communicates with it via WiFi or serial

class CameraModule {
private:
  String cameraIP;
  int cameraPort;
  bool connected;
  
public:
  CameraModule() {
    cameraIP = "192.168.4.1"; // Default AP mode IP
    cameraPort = 80;
    connected = false;
  }
  
  void begin() {
    // ESP32-CAM should be running its own web server
    // This module just provides the stream URL
    Serial.println("Camera module interface initialized");
    Serial.println("Note: ESP32-CAM should run separate firmware");
    
    // Try to detect ESP32-CAM
    checkConnection();
  }
  
  void checkConnection() {
    // Ping ESP32-CAM to verify connection
    HTTPClient http;
    String url = "http://" + cameraIP + ":" + String(cameraPort) + "/";
    
    http.begin(url);
    http.setTimeout(2000);
    
    int httpCode = http.GET();
    connected = (httpCode > 0);
    
    if (connected) {
      Serial.println("ESP32-CAM connected");
    } else {
      Serial.println("ESP32-CAM not detected");
    }
    
    http.end();
  }
  
  String getStreamURL() {
    if (!connected) {
      checkConnection();
    }
    
    return "http://" + cameraIP + ":" + String(cameraPort) + "/stream";
  }
  
  String getSnapshotURL() {
    return "http://" + cameraIP + ":" + String(cameraPort) + "/capture";
  }
  
  void setCameraIP(String ip) {
    cameraIP = ip;
    connected = false;
  }
  
  bool isConnected() {
    return connected;
  }
};

#endif
