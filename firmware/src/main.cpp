#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "config.h"
#include "sensors.h"
#include "motors.h"
#include "gps.h"
#include "sms.h"
#include "camera.h"
#include "servo.h"
#include "auto_mode.h"

WebServer server(80);
SensorManager sensors;
MotorController motors;
GPSModule gpsModule;
SMSModule smsModule;
CameraModule camera;
ServoController servoArm;
AutoModeController autoMode;

// System state
struct SystemState {
  float waterLevel;
  float distance;
  bool drainOpen;
  bool alertSent;
  GPSData gpsData;
  unsigned long lastUpdate;
} state;

// Thresholds
const float CRITICAL_LEVEL = 20.0; // cm
const float WARNING_LEVEL = 50.0;  // cm

void setup() {
  Serial.begin(115200);
  Serial.println("Drain Guard System Starting...");
  
  // Initialize WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  
  // Initialize components
  sensors.begin();
  motors.begin();
  gpsModule.begin();
  smsModule.begin();
  camera.begin();
  servoArm.begin();
  
  // Setup API endpoints
  setupAPIEndpoints();
  
  server.begin();
  Serial.println("HTTP server started");
  
  // Initialize state
  state.drainOpen = false;
  state.alertSent = false;
  state.lastUpdate = millis();
}

void loop() {
  server.handleClient();
  
  // Update sensor readings every 2 seconds
  if (millis() - state.lastUpdate > 2000) {
    updateSystemState();
    checkAlerts();
    sendTelemetry();
    
    // Check auto mode
    checkAutoMode();
    
    state.lastUpdate = millis();
  }
  
  // Update GPS data
  gpsModule.update();
  
  // Process SMS commands
  smsModule.checkMessages();
  
  delay(10);
}

void checkAutoMode() {
  if (autoMode.shouldOperate(state.distance)) {
    autoMode.startOperation();
    
    Serial.println("Auto mode: Running automatic sequence");
    
    // Open drain
    servoArm.openDrainWithArm();
    delay(3000);
    
    // Wait
    delay(2000);
    
    // Close drain
    servoArm.closeDrainWithArm();
    delay(3000);
    
    // Return home
    servoArm.moveToHomePosition();
    delay(2000);
    
    autoMode.completeOperation();
    Serial.println("Auto mode: Sequence finished");
  }
}

void updateSystemState() {
  state.distance = sensors.readUltrasonic();
  state.waterLevel = sensors.calculateWaterLevel(state.distance);
  state.gpsData = gpsModule.getLocation();
  
  Serial.printf("Water Level: %.2f cm, Distance: %.2f cm\n", 
                state.waterLevel, state.distance);
}

void checkAlerts() {
  if (state.distance < CRITICAL_LEVEL && !state.alertSent) {
    String alertMsg = "CRITICAL: Water level high at drain! ";
    alertMsg += "Location: " + String(state.gpsData.latitude, 6) + ", " 
              + String(state.gpsData.longitude, 6);
    
    smsModule.sendSMS(ALERT_PHONE_NUMBER, alertMsg);
    state.alertSent = true;
    
    // Auto-open drain if configured
    if (AUTO_OPEN_DRAIN) {
      motors.openDrain();
      state.drainOpen = true;
    }
  } else if (state.distance > WARNING_LEVEL) {
    state.alertSent = false;
  }
}

void sendTelemetry() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(API_ENDPOINT);
    http.addHeader("Content-Type", "application/json");
    
    StaticJsonDocument<512> doc;
    doc["device_id"] = DEVICE_ID;
    doc["water_level"] = state.waterLevel;
    doc["distance"] = state.distance;
    doc["drain_open"] = state.drainOpen;
    doc["latitude"] = state.gpsData.latitude;
    doc["longitude"] = state.gpsData.longitude;
    doc["satellites"] = state.gpsData.satellites;
    doc["timestamp"] = millis();
    
    String jsonData;
    serializeJson(doc, jsonData);
    
    int httpCode = http.POST(jsonData);
    if (httpCode > 0) {
      Serial.printf("Telemetry sent: %d\n", httpCode);
    }
    http.end();
  }
}

void setupAPIEndpoints() {
  // Get system status
  server.on("/api/status", HTTP_GET, []() {
    StaticJsonDocument<512> doc;
    doc["water_level"] = state.waterLevel;
    doc["distance"] = state.distance;
    doc["drain_open"] = state.drainOpen;
    doc["latitude"] = state.gpsData.latitude;
    doc["longitude"] = state.gpsData.longitude;
    doc["satellites"] = state.gpsData.satellites;
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  });
  
  // Control drain with motors
  server.on("/api/drain/open", HTTP_POST, []() {
    motors.openDrain();
    state.drainOpen = true;
    server.send(200, "application/json", "{\"status\":\"opened\",\"method\":\"motors\"}");
  });
  
  server.on("/api/drain/close", HTTP_POST, []() {
    motors.closeDrain();
    state.drainOpen = false;
    server.send(200, "application/json", "{\"status\":\"closed\",\"method\":\"motors\"}");
  });
  
  // Control drain with servo arm
  server.on("/api/arm/open", HTTP_POST, []() {
    servoArm.openDrainWithArm();
    state.drainOpen = true;
    server.send(200, "application/json", "{\"status\":\"opened\",\"method\":\"servo_arm\"}");
  });
  
  server.on("/api/arm/close", HTTP_POST, []() {
    servoArm.closeDrainWithArm();
    state.drainOpen = false;
    server.send(200, "application/json", "{\"status\":\"closed\",\"method\":\"servo_arm\"}");
  });
  
  server.on("/api/arm/home", HTTP_POST, []() {
    servoArm.moveToHomePosition();
    server.send(200, "application/json", "{\"status\":\"home\"}");
  });
  
  server.on("/api/arm/demo", HTTP_POST, []() {
    servoArm.runDemoSequence();
    server.send(200, "application/json", "{\"status\":\"demo_complete\"}");
  });
  
  // Individual servo control
  server.on("/api/servo/base", HTTP_POST, []() {
    if (server.hasArg("position")) {
      int pos = server.arg("position").toInt();
      servoArm.setBase(pos);
      server.send(200, "application/json", "{\"status\":\"moved\",\"servo\":\"base\",\"position\":" + String(pos) + "}");
    } else {
      server.send(400, "application/json", "{\"error\":\"missing position parameter\"}");
    }
  });
  
  server.on("/api/servo/shoulder", HTTP_POST, []() {
    if (server.hasArg("position")) {
      int pos = server.arg("position").toInt();
      servoArm.setShoulder(pos);
      server.send(200, "application/json", "{\"status\":\"moved\",\"servo\":\"shoulder\",\"position\":" + String(pos) + "}");
    } else {
      server.send(400, "application/json", "{\"error\":\"missing position parameter\"}");
    }
  });
  
  server.on("/api/servo/elbow", HTTP_POST, []() {
    if (server.hasArg("position")) {
      int pos = server.arg("position").toInt();
      servoArm.setElbow(pos);
      server.send(200, "application/json", "{\"status\":\"moved\",\"servo\":\"elbow\",\"position\":" + String(pos) + "}");
    } else {
      server.send(400, "application/json", "{\"error\":\"missing position parameter\"}");
    }
  });
  
  server.on("/api/servo/gripper", HTTP_POST, []() {
    if (server.hasArg("position")) {
      int pos = server.arg("position").toInt();
      servoArm.setGripper(pos);
      server.send(200, "application/json", "{\"status\":\"moved\",\"servo\":\"gripper\",\"position\":" + String(pos) + "}");
    } else {
      server.send(400, "application/json", "{\"error\":\"missing position parameter\"}");
    }
  });
  
  server.on("/api/servo/status", HTTP_GET, []() {
    uint16_t base, shoulder, elbow, gripper;
    servoArm.getCurrentPositions(base, shoulder, elbow, gripper);
    
    StaticJsonDocument<256> doc;
    doc["base"] = base;
    doc["shoulder"] = shoulder;
    doc["elbow"] = elbow;
    doc["gripper"] = gripper;
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  });
  
  // Auto mode control
  server.on("/api/auto/start", HTTP_POST, []() {
    autoMode.enable();
    server.send(200, "application/json", "{\"status\":\"started\"}");
  });
  
  server.on("/api/auto/stop", HTTP_POST, []() {
    autoMode.disable();
    server.send(200, "application/json", "{\"status\":\"stopped\"}");
  });
  
  server.on("/api/auto/status", HTTP_GET, []() {
    StaticJsonDocument<256> doc;
    doc["enabled"] = autoMode.isEnabled();
    doc["operating"] = autoMode.isCurrentlyOperating();
    doc["detection_range"] = autoMode.getDetectionRange();
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  });
  
  server.on("/api/auto/range", HTTP_POST, []() {
    if (server.hasArg("range")) {
      float range = server.arg("range").toFloat();
      autoMode.setDetectionRange(range);
      server.send(200, "application/json", "{\"status\":\"updated\"}");
    } else {
      server.send(400, "application/json", "{\"error\":\"missing range parameter\"}");
    }
  });
  
  // Get camera stream URL
  server.on("/api/camera/stream", HTTP_GET, []() {
    String streamUrl = camera.getStreamURL();
    server.send(200, "application/json", 
                "{\"stream_url\":\"" + streamUrl + "\"}");
  });
  
  // Get GPS location
  server.on("/api/gps", HTTP_GET, []() {
    StaticJsonDocument<256> doc;
    doc["latitude"] = state.gpsData.latitude;
    doc["longitude"] = state.gpsData.longitude;
    doc["altitude"] = state.gpsData.altitude;
    doc["satellites"] = state.gpsData.satellites;
    doc["valid"] = state.gpsData.valid;
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  });
}
