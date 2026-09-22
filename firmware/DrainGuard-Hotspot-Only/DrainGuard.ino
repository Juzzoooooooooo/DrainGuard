// DrainGuard Robot - HOTSPOT-ONLY VERSION (NO BLUETOOTH)
// Simplified architecture: WiFi Hotspot + HTTP API only
// Phone connects to ESP32 hotspot → uses HTTP API for all controls

// ============================================================================
// INCLUDES
// ============================================================================
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <HardwareSerial.h>

// ============================================================================
// CONFIGURATION
// ============================================================================

#define DEVICE_ID "DRAIN_GUARD_001"

// WiFi Hotspot — phone connects here
#define AP_SSID     "DrainGuard-Robot"
#define AP_PASSWORD "DrainGuard123"
#define AP_IP       "192.168.4.1"

// Optional ESP32-CAM on the private hotspot
#define CAMERA_IP   "192.168.4.50"
#define CAMERA_PORT 80
#define CAMERA_CHECK_TIMEOUT_MS 1500
#define CAMERA_CHECK_INTERVAL_MS 10000

// Alert SMS number
#define ALERT_PHONE "+1234567890"
#define AUTO_OPEN_DRAIN true

// ── Pin Definitions ──────────────────────────────────────────────────────────

#define TRIG_PIN    25
#define ECHO_PIN    34

#define MOTOR_AIN1  27
#define MOTOR_AIN2  14
#define MOTOR_BIN1  12
#define MOTOR_BIN2  23
#define MOTOR_PWMA  26
#define MOTOR_PWMB  13
#define MOTOR_STBY   4

#define A9G_RX      33   // GPS Serial1
#define A9G_TX      32

#define A7670_RX    16   // SMS Serial2
#define A7670_TX    17

#define PCA9685_SDA 21
#define PCA9685_SCL 22
#define PCA9685_ADDR 0x40

// LED indicator — GPIO 2 is the built-in blue LED
#define LED_BUILTIN_PIN  2

// ── System timing ─────────────────────────────────────────────────────────────

#define SENSOR_INTERVAL  2000   // ms between ultrasonic reads

// ── Motor timing ─────────────────────────────────────────────────────────────

#define MOTOR_SPEED     200     // 0-255
#define DRAIN_OPEN_MS  5000
#define DRAIN_CLOSE_MS 5000

// ── Ultrasonic ───────────────────────────────────────────────────────────────

#define MAX_DISTANCE 400        // cm
#define TANK_HEIGHT  200        // cm

// ── Servo channels on PCA9685 ────────────────────────────────────────────────

#define SERVO_FREQ         60
#define SERVO_BASE          0
#define SERVO_SHOULDER      1
#define SERVO_ELBOW         2
#define SERVO_GRIPPER       3

#define SERVO_BASE_MIN     250
#define SERVO_BASE_MAX     450
#define SERVO_SHOULDER_MIN 150
#define SERVO_SHOULDER_MAX 380
#define SERVO_ELBOW_MIN    300
#define SERVO_ELBOW_MAX    380
#define SERVO_GRIPPER_MIN  410
#define SERVO_GRIPPER_MAX  510

// Positional servos: pulse width selects joint angle and remains active to hold it.

// ── Alert thresholds ─────────────────────────────────────────────────────────

#define CRITICAL_DIST 20.0f
#define WARNING_DIST  50.0f

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

WebServer               server(80);
Adafruit_PWMServoDriver pwm(PCA9685_ADDR);
HardwareSerial          gpsSerial(1);
HardwareSerial          smsSerial(2);

// ============================================================================
// STATE
// ============================================================================

struct SystemState {
  float         distance        = -1;
  float         waterLevel      =  0;
  bool          drainOpen       = false;
  bool          alertSent       = false;
  unsigned long lastSensorUpdate = 0;
} state;

// Last commanded positions. The servos have internal feedback, not network telemetry.
uint16_t basePos     = 330;
uint16_t shoulderPos = 150;
uint16_t elbowPos    = 300;
uint16_t gripperPos  = 410;

// Automatic arm sequence
enum AutoSequenceStep {
  AUTO_IDLE,
  AUTO_OPENING,
  AUTO_WAITING,
  AUTO_CLOSING,
  AUTO_HOMING,
  AUTO_COOLDOWN
};

AutoSequenceStep autoStep = AUTO_IDLE;
bool          autoModeEnabled   = false;
bool          autoModeOperating = false;
float         autoDetectionRange = CRITICAL_DIST;
unsigned long autoStepTime       = 0;
unsigned long autoLastOperation  = 0;
const unsigned long AUTO_OPERATION_COOLDOWN_MS = 10000;

// LED state
unsigned long ledTimer    = 0;
bool          ledState    = false;

// GPS state
String  gpsBuffer;
float   gpsLat      = 0;
float   gpsLon      = 0;
int     gpsSatCount = 0;
bool    gpsValid    = false;

// Camera state
struct CameraModuleState {
  bool    available;
  bool    streaming;
  int     quality;
  int     brightness;
  int     contrast;
  bool    flashEnabled;
  unsigned long lastCheck;
} cameraState;

bool          cameraAvailable       = false;
unsigned long cameraLastCheck       = 0;

// SMS state machine
enum SmsState { SMS_IDLE, SMS_CMGF, SMS_NUMBER, SMS_BODY, SMS_WAIT };
SmsState      smsState  = SMS_IDLE;
unsigned long smsTimer  = 0;
String        smsPending;
String        smsNumber;

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

void  initHotspot();
void  setupAPIEndpoints();
float readUltrasonic();
void  openDrain();
void  closeDrain();
void  stopMotors();
uint16_t getServoPosition(uint8_t ch);
void  setServo(uint8_t ch, uint16_t pos);
void  setServoImmediate(uint8_t ch, uint16_t pos);
void  setServoSmooth(uint8_t ch, uint16_t target, int stepDelayMs = 10);
void  moveArmHome();
void  openDrainWithArm();
void  closeDrainWithArm();
void  updateAutoMode();
void  updateCameraStatus();
void  sendSMS(const String &number, const String &msg);
void  checkAlerts();
void  updateGPS();
void  updateSMS();
void  parseGGA(const String &sentence);
void  updateLED();

// ============================================================================
// HARDWARE INIT
// ============================================================================

void initHotspot() {
  IPAddress ip, gw, sn;
  ip.fromString(AP_IP);
  gw = ip;
  sn.fromString("255.255.255.0");
  WiFi.softAPConfig(ip, gw, sn);
  
  bool ok = WiFi.softAP(AP_SSID, AP_PASSWORD, 1, false, 4);
  Serial.printf("[AP] Hotspot %s — %s (SSID: %s)\n", 
                ok ? "started" : "FAILED", AP_IP, AP_SSID);
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== DrainGuard Starting (Hotspot-Only) ===");

  // Pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(MOTOR_AIN1, OUTPUT); pinMode(MOTOR_AIN2, OUTPUT);
  pinMode(MOTOR_BIN1, OUTPUT); pinMode(MOTOR_BIN2, OUTPUT);
  pinMode(MOTOR_PWMA, OUTPUT); pinMode(MOTOR_PWMB, OUTPUT);
  pinMode(MOTOR_STBY, OUTPUT);
  stopMotors();

  // LED
  pinMode(LED_BUILTIN_PIN, OUTPUT);
  digitalWrite(LED_BUILTIN_PIN, HIGH); // ON = ready

  // I2C + PCA9685
  Wire.begin(PCA9685_SDA, PCA9685_SCL);
  pwm.begin();
  pwm.setPWMFreq(SERVO_FREQ);
  delay(10);
  for (uint8_t ch = 0; ch < 4; ++ch) pwm.setPWM(ch, 0, 4096);

  // Serial modules
  gpsSerial.begin(115200, SERIAL_8N1, A9G_RX, A9G_TX);
  smsSerial.begin(115200, SERIAL_8N1, A7670_RX, A7670_TX);

  // WiFi: AP only (no station mode needed)
  WiFi.mode(WIFI_AP);
  WiFi.persistent(false);

  // Initialize camera state
  cameraState.available = false;
  cameraState.streaming = false;
  cameraState.quality = 1;
  cameraState.brightness = 0;
  cameraState.contrast = 0;
  cameraState.flashEnabled = false;
  cameraState.lastCheck = 0;

  initHotspot();

  setupAPIEndpoints();
  server.begin();
  Serial.printf("[HTTP] API ready at http://%s\n", AP_IP);
  Serial.println("\n📱 CONNECT YOUR PHONE:");
  Serial.printf("   WiFi SSID: %s\n", AP_SSID);
  Serial.printf("   Password:  %s\n", AP_PASSWORD);
  Serial.printf("   API URL:   http://%s/api/status\n\n", AP_IP);
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  // LED status indicator
  updateLED();

  // HTTP server
  server.handleClient();

  // GPS — read serial bytes
  updateGPS();

  // SMS state machine
  updateSMS();

  // Sensor + alert every SENSOR_INTERVAL ms
  if (millis() - state.lastSensorUpdate >= SENSOR_INTERVAL) {
    state.distance   = readUltrasonic();
    state.waterLevel = (state.distance > 0) ? (TANK_HEIGHT - state.distance) : 0;
    Serial.printf("[Sensor] Distance: %.1f cm  WaterLevel: %.1f cm  GPS: %s\n",
                  state.distance, state.waterLevel, gpsValid ? "fix" : "no fix");
    checkAlerts();
    state.lastSensorUpdate = millis();
  }

  updateAutoMode();
  updateCameraStatus();
}

// ============================================================================
// ALERT LOGIC
// ============================================================================

void checkAlerts() {
  if (state.distance <= 0) return;

  if (state.distance < CRITICAL_DIST && !state.alertSent) {
    Serial.println("[ALERT] Critical water level!");
    state.alertSent = true;
    sendSMS(ALERT_PHONE, "CRITICAL: DrainGuard water level is high!");
    if (AUTO_OPEN_DRAIN) openDrain();
  } else if (state.distance > WARNING_DIST) {
    state.alertSent = false;
  }
}

// ============================================================================
// MOTOR FUNCTIONS
// ============================================================================

void openDrain() {
  Serial.println("[Motor] Opening drain");
  digitalWrite(MOTOR_STBY, HIGH);
  digitalWrite(MOTOR_AIN1, HIGH); digitalWrite(MOTOR_AIN2, LOW);
  analogWrite(MOTOR_PWMA, MOTOR_SPEED);
  digitalWrite(MOTOR_BIN1, HIGH); digitalWrite(MOTOR_BIN2, LOW);
  analogWrite(MOTOR_PWMB, MOTOR_SPEED);
  delay(DRAIN_OPEN_MS);
  stopMotors();
  state.drainOpen = true;
}

void closeDrain() {
  Serial.println("[Motor] Closing drain");
  digitalWrite(MOTOR_STBY, HIGH);
  digitalWrite(MOTOR_AIN1, LOW); digitalWrite(MOTOR_AIN2, HIGH);
  analogWrite(MOTOR_PWMA, MOTOR_SPEED);
  digitalWrite(MOTOR_BIN1, LOW); digitalWrite(MOTOR_BIN2, HIGH);
  analogWrite(MOTOR_PWMB, MOTOR_SPEED);
  delay(DRAIN_CLOSE_MS);
  stopMotors();
  state.drainOpen = false;
}

void stopMotors() {
  digitalWrite(MOTOR_AIN1, LOW); digitalWrite(MOTOR_AIN2, LOW);
  digitalWrite(MOTOR_BIN1, LOW); digitalWrite(MOTOR_BIN2, LOW);
  analogWrite(MOTOR_PWMA, 0);    analogWrite(MOTOR_PWMB, 0);
  digitalWrite(MOTOR_STBY, LOW);
}

// ============================================================================
// SERVO FUNCTIONS
// ============================================================================

uint16_t getServoPosition(uint8_t ch) {
  switch (ch) {
    case SERVO_BASE:     return basePos;
    case SERVO_SHOULDER: return shoulderPos;
    case SERVO_ELBOW:    return elbowPos;
    case SERVO_GRIPPER:  return gripperPos;
    default:             return 0;
  }
}

void setServo(uint8_t ch, uint16_t pos) {
  if (ch > SERVO_GRIPPER) return;
  switch (ch) {
    case SERVO_BASE:     pos = constrain(pos, SERVO_BASE_MIN, SERVO_BASE_MAX); break;
    case SERVO_SHOULDER: pos = constrain(pos, SERVO_SHOULDER_MIN, SERVO_SHOULDER_MAX); break;
    case SERVO_ELBOW:    pos = constrain(pos, SERVO_ELBOW_MIN, SERVO_ELBOW_MAX); break;
    case SERVO_GRIPPER:  pos = constrain(pos, SERVO_GRIPPER_MIN, SERVO_GRIPPER_MAX); break;
  }
  pwm.setPWM(ch, 0, pos);
  switch (ch) {
    case SERVO_BASE:     basePos = pos; break;
    case SERVO_SHOULDER: shoulderPos = pos; break;
    case SERVO_ELBOW:    elbowPos = pos; break;
    case SERVO_GRIPPER:  gripperPos = pos; break;
  }
}

void setServoImmediate(uint8_t ch, uint16_t pos) {
  setServo(ch, pos);
}

void setServoSmooth(uint8_t ch, uint16_t target, int stepDelayMs) {
  (void)stepDelayMs;
  setServo(ch, target);
}

void moveArmHome() {
  Serial.println("[Arm] Returning home");
  setServoSmooth(SERVO_BASE, 330);
  delay(100);
  setServoSmooth(SERVO_SHOULDER, 150);
  delay(100);
  setServoSmooth(SERVO_ELBOW, 300);
  delay(100);
  setServoSmooth(SERVO_GRIPPER, 410);
}

void openDrainWithArm() {
  Serial.println("[Arm] Opening drain");
  setServoSmooth(SERVO_BASE, 250);
  delay(500);
  setServoSmooth(SERVO_SHOULDER, 380);
  delay(500);
  setServoSmooth(SERVO_ELBOW, 380);
  delay(500);
  setServoSmooth(SERVO_GRIPPER, 510);
  delay(1000);
  state.drainOpen = true;
}

void closeDrainWithArm() {
  Serial.println("[Arm] Closing drain");
  setServoSmooth(SERVO_GRIPPER, 410);
  delay(500);
  setServoSmooth(SERVO_ELBOW, 300);
  delay(500);
  setServoSmooth(SERVO_SHOULDER, 150);
  delay(500);
  setServoSmooth(SERVO_BASE, 330);
  delay(500);
  state.drainOpen = false;
}

void updateAutoMode() {
  if (!autoModeEnabled) {
    autoModeOperating = false;
    autoStep = AUTO_IDLE;
    return;
  }

  unsigned long now = millis();
  switch (autoStep) {
    case AUTO_IDLE:
      if (
        state.distance > 0 &&
        state.distance <= autoDetectionRange &&
        now - autoLastOperation >= AUTO_OPERATION_COOLDOWN_MS
      ) {
        autoModeOperating = true;
        autoLastOperation = now;
        openDrainWithArm();
        autoStep = AUTO_OPENING;
        autoStepTime = millis();
      }
      break;

    case AUTO_OPENING:
      if (now - autoStepTime >= 3000) {
        autoStep = AUTO_WAITING;
        autoStepTime = now;
      }
      break;

    case AUTO_WAITING:
      if (now - autoStepTime >= 2000) {
        closeDrainWithArm();
        autoStep = AUTO_CLOSING;
        autoStepTime = millis();
      }
      break;

    case AUTO_CLOSING:
      if (now - autoStepTime >= 3000) {
        moveArmHome();
        autoStep = AUTO_HOMING;
        autoStepTime = millis();
      }
      break;

    case AUTO_HOMING:
      if (now - autoStepTime >= 2000) {
        autoModeOperating = false;
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

void updateCameraStatus() {
  if (millis() - cameraLastCheck < CAMERA_CHECK_INTERVAL_MS) return;
  cameraLastCheck = millis();

  HTTPClient http;
  String url = String("http://") + CAMERA_IP + ":" + String(CAMERA_PORT) + "/status";
  http.begin(url);
  http.setTimeout(CAMERA_CHECK_TIMEOUT_MS);
  int code = http.GET();
  http.end();
  
  cameraAvailable = (code == HTTP_CODE_OK);
  cameraState.available = cameraAvailable;
  cameraState.lastCheck = millis();
}

// ============================================================================
// ULTRASONIC SENSOR
// ============================================================================

float readUltrasonic() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long dur = pulseIn(ECHO_PIN, HIGH, 30000);
  if (dur == 0) return -1;
  float dist = dur * 0.034f / 2.0f;
  return (dist < 2 || dist > MAX_DISTANCE) ? -1 : dist;
}

// ============================================================================
// GPS (A9G)
// ============================================================================

void parseGGA(const String &sentence) {
  int field = 0;
  int start = 0;
  String fields[10];

  for (int i = 0; i <= sentence.length() && field < 10; i++) {
    if (i == sentence.length() || sentence[i] == ',') {
      fields[field++] = sentence.substring(start, i);
      start = i + 1;
    }
  }

  if (field < 8) return;
  if (fields[6] == "0" || fields[6] == "") return;

  float rawLat = fields[2].toFloat();
  int   latDeg = (int)(rawLat / 100);
  float latMin = rawLat - latDeg * 100;
  gpsLat = latDeg + latMin / 60.0f;
  if (fields[3] == "S") gpsLat = -gpsLat;

  float rawLon = fields[4].toFloat();
  int   lonDeg = (int)(rawLon / 100);
  float lonMin = rawLon - lonDeg * 100;
  gpsLon = lonDeg + lonMin / 60.0f;
  if (fields[5] == "W") gpsLon = -gpsLon;

  gpsSatCount = fields[7].toInt();
  gpsValid    = true;
}

void updateGPS() {
  while (gpsSerial.available()) {
    char c = gpsSerial.read();
    if (c == '\n') {
      gpsBuffer.trim();
      if (gpsBuffer.startsWith("$GPGGA") || gpsBuffer.startsWith("$GNGGA")) {
        parseGGA(gpsBuffer);
      }
      gpsBuffer = "";
    } else if (c != '\r') {
      gpsBuffer += c;
      if (gpsBuffer.length() > 120) gpsBuffer = "";
    }
  }
}

// ============================================================================
// SMS (A7670)
// ============================================================================

void sendSMS(const String &number, const String &msg) {
  if (smsState != SMS_IDLE) return;
  smsNumber  = number;
  smsPending = msg;
  smsState   = SMS_CMGF;
  smsTimer   = millis();
  Serial.printf("[SMS] Queued to %s\n", number.c_str());
}

void updateSMS() {
  switch (smsState) {
    case SMS_IDLE: break;

    case SMS_CMGF:
      smsSerial.println("AT+CMGF=1");
      smsState = SMS_NUMBER;
      smsTimer = millis();
      break;

    case SMS_NUMBER:
      if (millis() - smsTimer < 300) break;
      smsSerial.print("AT+CMGS=\"");
      smsSerial.print(smsNumber);
      smsSerial.println("\"");
      smsState = SMS_BODY;
      smsTimer = millis();
      break;

    case SMS_BODY:
      if (millis() - smsTimer < 300) break;
      smsSerial.print(smsPending);
      smsSerial.write(26);
      smsState = SMS_WAIT;
      smsTimer = millis();
      break;

    case SMS_WAIT:
      if (millis() - smsTimer < 4000) break;
      Serial.printf("[SMS] Sent to %s\n", smsNumber.c_str());
      smsState = SMS_IDLE;
      break;
  }
}

// ============================================================================
// LED STATUS INDICATOR
// ============================================================================

void updateLED() {
  // Slow blink = hotspot active, waiting for connections
  unsigned long now = millis();
  if (now - ledTimer >= 1000) {
    ledTimer = now;
    ledState = !ledState;
    digitalWrite(LED_BUILTIN_PIN, ledState ? HIGH : LOW);
  }
}

// ============================================================================
// HTTP API ENDPOINTS
// ============================================================================

void setupAPIEndpoints() {
  // CORS preflight
  server.onNotFound([]() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    if (server.method() == HTTP_OPTIONS) server.send(204);
    else server.send(404, "text/plain", "Not found");
  });

  // GET /api/status
  server.on("/api/status", HTTP_GET, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    StaticJsonDocument<512> doc;
    doc["device_id"]        = DEVICE_ID;
    doc["water_level"]      = state.waterLevel;
    doc["distance"]         = state.distance;
    doc["drain_open"]       = state.drainOpen;
    doc["camera_available"] = cameraAvailable;
    doc["camera_streaming"] = cameraState.streaming;
    doc["camera_quality"]   = cameraState.quality;
    doc["latitude"]         = gpsLat;
    doc["longitude"]        = gpsLon;
    doc["satellites"]       = gpsSatCount;
    doc["gps_valid"]        = gpsValid;
    doc["uptime_s"]         = (unsigned long)(millis() / 1000);
    doc["free_heap"]        = (int)ESP.getFreeHeap();
    doc["clients_connected"] = WiFi.softAPgetStationNum();
    String out; serializeJson(doc, out);
    server.send(200, "application/json", out);
  });

  // POST /api/drain/open
  server.on("/api/drain/open", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    openDrain();
    server.send(200, "application/json", "{\"status\":\"opened\"}");
  });

  // POST /api/drain/close
  server.on("/api/drain/close", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    closeDrain();
    server.send(200, "application/json", "{\"status\":\"closed\"}");
  });

  // POST /api/arm/open
  server.on("/api/arm/open", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    openDrainWithArm();
    server.send(200, "application/json", "{\"status\":\"opened\"}");
  });

  // POST /api/arm/close
  server.on("/api/arm/close", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    closeDrainWithArm();
    server.send(200, "application/json", "{\"status\":\"closed\"}");
  });

  // POST /api/arm/home
  server.on("/api/arm/home", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    moveArmHome();
    server.send(200, "application/json", "{\"status\":\"home\"}");
  });

  // POST /api/servo/base?position=330
  server.on("/api/servo/base", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    if (!server.hasArg("position")) {
      server.send(400, "application/json", "{\"error\":\"missing position\"}");
      return;
    }
    int pos = constrain(server.arg("position").toInt(), SERVO_BASE_MIN, SERVO_BASE_MAX);
    setServoImmediate(SERVO_BASE, pos);
    server.send(200, "application/json", "{\"status\":\"moved\",\"position\":" + String(pos) + "}");
  });

  // POST /api/servo/shoulder?position=200
  server.on("/api/servo/shoulder", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    if (!server.hasArg("position")) {
      server.send(400, "application/json", "{\"error\":\"missing position\"}");
      return;
    }
    int pos = constrain(server.arg("position").toInt(), SERVO_SHOULDER_MIN, SERVO_SHOULDER_MAX);
    setServoImmediate(SERVO_SHOULDER, pos);
    server.send(200, "application/json", "{\"status\":\"moved\",\"position\":" + String(pos) + "}");
  });

  // POST /api/servo/elbow?position=340
  server.on("/api/servo/elbow", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    if (!server.hasArg("position")) {
      server.send(400, "application/json", "{\"error\":\"missing position\"}");
      return;
    }
    int pos = constrain(server.arg("position").toInt(), SERVO_ELBOW_MIN, SERVO_ELBOW_MAX);
    setServoImmediate(SERVO_ELBOW, pos);
    server.send(200, "application/json", "{\"status\":\"moved\",\"position\":" + String(pos) + "}");
  });

  // POST /api/servo/gripper?position=460
  server.on("/api/servo/gripper", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    if (!server.hasArg("position")) {
      server.send(400, "application/json", "{\"error\":\"missing position\"}");
      return;
    }
    int pos = constrain(server.arg("position").toInt(), SERVO_GRIPPER_MIN, SERVO_GRIPPER_MAX);
    setServoImmediate(SERVO_GRIPPER, pos);
    server.send(200, "application/json", "{\"status\":\"moved\",\"position\":" + String(pos) + "}");
  });

  // GET /api/servo/status
  server.on("/api/servo/status", HTTP_GET, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    StaticJsonDocument<192> doc;
    doc["base"] = basePos;
    doc["shoulder"] = shoulderPos;
    doc["elbow"] = elbowPos;
    doc["gripper"] = gripperPos;
    String out;
    serializeJson(doc, out);
    server.send(200, "application/json", out);
  });

  // POST /api/auto/start
  server.on("/api/auto/start", HTTP_POST, []() {
    autoModeEnabled = true;
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", "{\"status\":\"started\"}");
  });

  // POST /api/auto/stop
  server.on("/api/auto/stop", HTTP_POST, []() {
    autoModeEnabled = false;
    autoModeOperating = false;
    autoStep = AUTO_IDLE;
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", "{\"status\":\"stopped\"}");
  });

  // GET /api/auto/status
  server.on("/api/auto/status", HTTP_GET, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    StaticJsonDocument<160> doc;
    doc["enabled"] = autoModeEnabled;
    doc["operating"] = autoModeOperating;
    doc["detection_range"] = autoDetectionRange;
    String out;
    serializeJson(doc, out);
    server.send(200, "application/json", out);
  });

  // POST /api/auto/range?value=30.5
  server.on("/api/auto/range", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    if (!server.hasArg("value")) {
      server.send(400, "application/json", "{\"error\":\"missing value\"}");
      return;
    }
    autoDetectionRange = server.arg("value").toFloat();
    server.send(200, "application/json", "{\"status\":\"updated\",\"range\":" + String(autoDetectionRange) + "}");
  });

  // GET /api/gps
  server.on("/api/gps", HTTP_GET, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    StaticJsonDocument<128> doc;
    doc["latitude"]   = gpsLat;
    doc["longitude"]  = gpsLon;
    doc["satellites"] = gpsSatCount;
    doc["valid"]      = gpsValid;
    String out; serializeJson(doc, out);
    server.send(200, "application/json", out);
  });

  // GET /api/camera/status
  server.on("/api/camera/status", HTTP_GET, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    StaticJsonDocument<256> doc;
    doc["available"] = cameraAvailable;
    doc["streaming"] = cameraState.streaming;
    doc["quality"] = cameraState.quality;
    doc["brightness"] = cameraState.brightness;
    doc["contrast"] = cameraState.contrast;
    doc["flash"] = cameraState.flashEnabled;
    String out;
    serializeJson(doc, out);
    server.send(200, "application/json", out);
  });

  // POST /api/camera/capture
  server.on("/api/camera/capture", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    if (!cameraAvailable) {
      server.send(503, "application/json", "{\"error\":\"camera not available\"}");
      return;
    }
    server.send(200, "application/json", "{\"status\":\"captured\"}");
  });

  // POST /api/camera/stream/start
  server.on("/api/camera/stream/start", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    if (!cameraAvailable) {
      server.send(503, "application/json", "{\"error\":\"camera not available\"}");
      return;
    }
    cameraState.streaming = true;
    server.send(200, "application/json", "{\"status\":\"streaming\"}");
  });

  // POST /api/camera/stream/stop
  server.on("/api/camera/stream/stop", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    cameraState.streaming = false;
    server.send(200, "application/json", "{\"status\":\"stopped\"}");
  });
}
