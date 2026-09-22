#ifndef CAMERA_H
#define CAMERA_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include "config.h"

// Note: ESP32-CAM runs as a separate module with its own firmware
// and communicates over both WiFi (HTTP) and Bluetooth (BLE) protocols.

enum CameraConnectionMode {
  CAMERA_WIFI_ONLY,
  CAMERA_BLE_ONLY,
  CAMERA_DUAL_MODE  // Preferred: WiFi for streaming, BLE for control
};

enum CameraQuality {
  CAMERA_QUALITY_LOW = 0,     // 320x240
  CAMERA_QUALITY_MEDIUM = 1,  // 640x480
  CAMERA_QUALITY_HIGH = 2,    // 800x600
  CAMERA_QUALITY_MAX = 3      // 1024x768
};

enum CameraCommand {
  CAM_CMD_CAPTURE,
  CAM_CMD_START_STREAM,
  CAM_CMD_STOP_STREAM,
  CAM_CMD_SET_QUALITY,
  CAM_CMD_SET_BRIGHTNESS,
  CAM_CMD_SET_CONTRAST,
  CAM_CMD_ENABLE_FLASH,
  CAM_CMD_DISABLE_FLASH,
  CAM_CMD_GET_STATUS
};

struct CameraStatus {
  bool wifiConnected;
  bool bleConnected;
  bool streaming;
  int quality;
  int brightness;
  int contrast;
  bool flashEnabled;
  int freeHeap;
  String firmwareVersion;
};

class CameraModule {
private:
  String cameraIP;
  int cameraPort;
  bool wifiConnected;
  bool bleConnected;
  CameraConnectionMode connectionMode;
  CameraStatus status;
  unsigned long lastCheck;
  unsigned long checkInterval;
  
  // WiFi communication
  bool sendWiFiCommand(const String &endpoint, const String &payload = "");
  String getWiFiResponse(const String &endpoint);
  
public:
  CameraModule() {
    cameraIP = CAMERA_IP_ADDRESS;
    cameraPort = CAMERA_HTTP_PORT;
    wifiConnected = false;
    bleConnected = false;
    connectionMode = CAMERA_DUAL_MODE;
    lastCheck = 0;
    checkInterval = 10000; // Check every 10 seconds
    
    // Initialize status
    status.wifiConnected = false;
    status.bleConnected = false;
    status.streaming = false;
    status.quality = CAMERA_QUALITY_MEDIUM;
    status.brightness = 0;
    status.contrast = 0;
    status.flashEnabled = false;
    status.freeHeap = 0;
    status.firmwareVersion = "Unknown";
  }
  
  void begin() {
    Serial.println("Camera module interface initialized");
    Serial.println("Dual-mode camera: WiFi + Bluetooth connectivity");
    
    // Try to detect ESP32-CAM via WiFi
    checkWiFiConnection();
  }
  
  void update() {
    // Periodic connection check
    if (millis() - lastCheck >= checkInterval) {
      lastCheck = millis();
      checkWiFiConnection();
      if (wifiConnected) {
        updateStatus();
      }
    }
  }
  
  void checkWiFiConnection() {
    HTTPClient http;
    String url = "http://" + cameraIP + ":" + String(cameraPort) + "/status";
    
    http.begin(url);
    http.setTimeout(2000);
    
    int httpCode = http.GET();
    wifiConnected = (httpCode == HTTP_CODE_OK);
    status.wifiConnected = wifiConnected;
    
    if (wifiConnected) {
      Serial.println("[Camera] WiFi connected");
    } else {
      Serial.println("[Camera] WiFi not available");
    }
    
    http.end();
  }
  
  void updateStatus() {
    HTTPClient http;
    String url = "http://" + cameraIP + ":" + String(cameraPort) + "/api/status";
    
    http.begin(url);
    http.setTimeout(1500);
    
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      // Parse JSON status (simplified)
      status.streaming = payload.indexOf("\"streaming\":true") > 0;
      status.wifiConnected = true;
    }
    http.end();
  }
  
  // WiFi-based camera control
  bool capturePhoto() {
    if (!wifiConnected) {
      Serial.println("[Camera] WiFi not connected, cannot capture");
      return false;
    }
    
    String response = getWiFiResponse("/capture");
    Serial.println("[Camera] Photo captured via WiFi");
    return true;
  }
  
  bool startStreaming() {
    if (!wifiConnected) {
      Serial.println("[Camera] WiFi not connected, cannot start stream");
      return false;
    }
    
    bool success = sendWiFiCommand("/api/stream/start");
    if (success) {
      status.streaming = true;
      Serial.println("[Camera] Streaming started via WiFi");
    }
    return success;
  }
  
  bool stopStreaming() {
    if (!wifiConnected) return false;
    
    bool success = sendWiFiCommand("/api/stream/stop");
    if (success) {
      status.streaming = false;
      Serial.println("[Camera] Streaming stopped via WiFi");
    }
    return success;
  }
  
  bool setQuality(CameraQuality quality) {
    if (!wifiConnected) return false;
    
    String payload = "{\"quality\":" + String((int)quality) + "}";
    bool success = sendWiFiCommand("/api/quality", payload);
    if (success) {
      status.quality = (int)quality;
      Serial.printf("[Camera] Quality set to %d via WiFi\n", (int)quality);
    }
    return success;
  }
  
  bool setBrightness(int brightness) {
    if (!wifiConnected) return false;
    
    brightness = constrain(brightness, -2, 2);
    String payload = "{\"brightness\":" + String(brightness) + "}";
    bool success = sendWiFiCommand("/api/brightness", payload);
    if (success) {
      status.brightness = brightness;
      Serial.printf("[Camera] Brightness set to %d via WiFi\n", brightness);
    }
    return success;
  }
  
  bool setContrast(int contrast) {
    if (!wifiConnected) return false;
    
    contrast = constrain(contrast, -2, 2);
    String payload = "{\"contrast\":" + String(contrast) + "}";
    bool success = sendWiFiCommand("/api/contrast", payload);
    if (success) {
      status.contrast = contrast;
      Serial.printf("[Camera] Contrast set to %d via WiFi\n", contrast);
    }
    return success;
  }
  
  bool enableFlash(bool enable) {
    if (!wifiConnected) return false;
    
    String payload = "{\"flash\":" + String(enable ? "true" : "false") + "}";
    bool success = sendWiFiCommand("/api/flash", payload);
    if (success) {
      status.flashEnabled = enable;
      Serial.printf("[Camera] Flash %s via WiFi\n", enable ? "enabled" : "disabled");
    }
    return success;
  }
  
  // Bluetooth-based camera control (called from BLE handler)
  bool handleBluetoothCommand(CameraCommand cmd, int value = 0) {
    Serial.printf("[Camera] BLE command received: %d\n", cmd);
    
    // If WiFi is available, use it for better performance
    if (wifiConnected) {
      switch (cmd) {
        case CAM_CMD_CAPTURE:
          return capturePhoto();
        case CAM_CMD_START_STREAM:
          return startStreaming();
        case CAM_CMD_STOP_STREAM:
          return stopStreaming();
        case CAM_CMD_SET_QUALITY:
          return setQuality((CameraQuality)value);
        case CAM_CMD_SET_BRIGHTNESS:
          return setBrightness(value);
        case CAM_CMD_SET_CONTRAST:
          return setContrast(value);
        case CAM_CMD_ENABLE_FLASH:
          return enableFlash(true);
        case CAM_CMD_DISABLE_FLASH:
          return enableFlash(false);
        case CAM_CMD_GET_STATUS:
          updateStatus();
          return true;
        default:
          return false;
      }
    }
    
    // TODO: Implement direct BLE-to-camera communication
    // This would require ESP32-CAM to also expose BLE service
    Serial.println("[Camera] BLE-only mode not yet implemented");
    return false;
  }
  
  // Getters
  String getStreamURL() {
    if (!wifiConnected) {
      checkWiFiConnection();
    }
    return "http://" + cameraIP + ":" + String(cameraPort) + "/stream";
  }
  
  String getSnapshotURL() {
    return "http://" + cameraIP + ":" + String(cameraPort) + "/capture";
  }
  
  CameraStatus getStatus() {
    return status;
  }
  
  bool isWiFiConnected() {
    return wifiConnected;
  }
  
  bool isBluetoothConnected() {
    return bleConnected;
  }
  
  bool isConnected() {
    return wifiConnected || bleConnected;
  }
  
  // Setters
  void setCameraIP(String ip) {
    cameraIP = ip;
    wifiConnected = false;
  }
  
  void setConnectionMode(CameraConnectionMode mode) {
    connectionMode = mode;
    Serial.printf("[Camera] Connection mode set to: %d\n", mode);
  }
  
  void setBluetoothConnected(bool connected) {
    bleConnected = connected;
    status.bleConnected = connected;
  }

private:
  bool sendWiFiCommand(const String &endpoint, const String &payload) {
    HTTPClient http;
    String url = "http://" + cameraIP + ":" + String(cameraPort) + endpoint;
    
    http.begin(url);
    http.setTimeout(3000);
    http.addHeader("Content-Type", "application/json");
    
    int httpCode;
    if (payload.length() > 0) {
      httpCode = http.POST(payload);
    } else {
      httpCode = http.POST("");
    }
    
    bool success = (httpCode == HTTP_CODE_OK || httpCode == 200);
    http.end();
    
    return success;
  }
  
  String getWiFiResponse(const String &endpoint) {
    HTTPClient http;
    String url = "http://" + cameraIP + ":" + String(cameraPort) + endpoint;
    
    http.begin(url);
    http.setTimeout(3000);
    
    int httpCode = http.GET();
    String response = "";
    
    if (httpCode == HTTP_CODE_OK) {
      response = http.getString();
    }
    
    http.end();
    return response;
  }
};

#endif
