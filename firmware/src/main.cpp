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
#include "wifi_provisioning.h"

WebServer server(80);
SensorManager sensors;
MotorController motors;
GPSModule gpsModule;
SMSModule smsModule;
CameraModule camera;
ServoController servoArm;
AutoModeController autoMode;
WiFiProvisioningManager provisioning;
bool httpServerStarted = false;

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

void setupAPIEndpoints();
void updateSystemState();
void checkAlerts();
void sendTelemetry();
void checkAutoMode();
void processBleControllerCommand();

void setup() {
  Serial.begin(115200);
  Serial.println("Drain Guard System Starting...");
  
  // Start BLE provisioning and connect using saved credentials.
  // This is non-blocking, so invalid WiFi credentials never prevent the robot
  // hardware or provisioning service from starting.
  provisioning.begin(WIFI_SSID, WIFI_PASSWORD, API_ENDPOINT);
  
  // Initialize components
  sensors.begin();
  motors.begin();
  gpsModule.begin();
  smsModule.begin();
  camera.begin();
  servoArm.begin();
  
  // Setup API endpoints
  setupAPIEndpoints();
  // The private hotspot is available even when the optional station uplink is
  // not connected, so the local robot API must start unconditionally.
  server.begin();
  httpServerStarted = true;
  Serial.printf("HTTP server ready on hotspot: http://%s\n", provisioning.getHotspotIP().c_str());
  
  // Initialize state
  state.drainOpen = false;
  state.alertSent = false;
  state.lastUpdate = millis();
}

void loop() {
  provisioning.update();
  processBleControllerCommand();

  if (httpServerStarted) {
    server.handleClient();
  }
  
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

void processBleControllerCommand() {
  WiFiProvisioningManager::ControllerCommand command;
  if (!provisioning.nextControllerCommand(command)) return;

  switch (command.type) {
    case WiFiProvisioningManager::CONTROLLER_GET_STATUS:
      provisioning.notifyControllerStatus(
        command.requestId,
        state.waterLevel,
        state.distance,
        state.drainOpen,
        state.gpsData.latitude,
        state.gpsData.longitude,
        state.gpsData.satellites
      );
      break;

    case WiFiProvisioningManager::CONTROLLER_ARM_OPEN:
      // Acknowledge before the smooth arm sequence blocks the Arduino loop.
      provisioning.notifyControllerResult(command.requestId, true, "accepted");
      servoArm.openDrainWithArm();
      state.drainOpen = true;
      break;

    case WiFiProvisioningManager::CONTROLLER_ARM_CLOSE:
      provisioning.notifyControllerResult(command.requestId, true, "accepted");
      servoArm.closeDrainWithArm();
      state.drainOpen = false;
      break;

    case WiFiProvisioningManager::CONTROLLER_SERVO:
      switch (command.servo) {
        case WiFiProvisioningManager::CONTROLLER_SERVO_BASE:
          servoArm.setBaseImmediate(command.position);
          break;
        case WiFiProvisioningManager::CONTROLLER_SERVO_SHOULDER:
          servoArm.setShoulderImmediate(command.position);
          break;
        case WiFiProvisioningManager::CONTROLLER_SERVO_ELBOW:
          servoArm.setElbowImmediate(command.position);
          break;
        case WiFiProvisioningManager::CONTROLLER_SERVO_GRIPPER:
          servoArm.setGripperImmediate(command.position);
          break;
      }
      provisioning.notifyControllerResult(command.requestId, true, "moved");
      break;
  }
}

// Auto mode uses a state machine to avoid blocking the main loop
// with long delay() calls that would freeze the web server and BLE
enum AutoSequenceStep {
  AUTO_IDLE,
  AUTO_OPENING,
  AUTO_WAITING,
  AUTO_CLOSING,
  AUTO_HOMING,
  AUTO_COOLDOWN
};

AutoSequenceStep autoStep = AUTO_IDLE;
unsigned long autoStepTime = 0;

void checkAutoMode() {
  // State machine - no blocking delays
  unsigned long now = millis();

  switch (autoStep) {
    case AUTO_IDLE:
      if (autoMode.shouldOperate(state.distance)) {
        autoMode.startOperation();
        Serial.println("Auto mode: Opening drain");
        servoArm.openDrainWithArm();
        autoStep = AUTO_OPENING;
        autoStepTime = now;
      }
      break;

    case AUTO_OPENING:
      if (now - autoStepTime >= 3000) {
        Serial.println("Auto mode: Waiting");
        autoStep = AUTO_WAITING;
        autoStepTime = now;
      }
      break;

    case AUTO_WAITING:
      if (now - autoStepTime >= 2000) {
        Serial.println("Auto mode: Closing drain");
        servoArm.closeDrainWithArm();
        autoStep = AUTO_CLOSING;
        autoStepTime = now;
      }
      break;

    case AUTO_CLOSING:
      if (now - autoStepTime >= 3000) {
        Serial.println("Auto mode: Returning home");
        servoArm.moveToHomePosition();
        autoStep = AUTO_HOMING;
        autoStepTime = now;
      }
      break;

    case AUTO_HOMING:
      if (now - autoStepTime >= 2000) {
        autoMode.completeOperation();
        Serial.println("Auto mode: Sequence finished");
        autoStep = AUTO_COOLDOWN;
        autoStepTime = now;
      }
      break;

    case AUTO_COOLDOWN:
      if (now - autoStepTime >= 5000) {
        autoStep = AUTO_IDLE;
      }
      break;
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
  // Guard against invalid sensor readings (-1 means timeout/out-of-range)
  if (state.distance <= 0) {
    return;
  }
  
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
    http.begin(provisioning.getApiEndpoint());
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(3000); // 3 second timeout - prevent blocking loop
    
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
    } else {
      Serial.printf("Telemetry failed: %s\n", http.errorToString(httpCode).c_str());
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
    StaticJsonDocument<192> doc;
    doc["available"] = camera.isConnected();
    doc["stream_url"] = streamUrl;
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
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
