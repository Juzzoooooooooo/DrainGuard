// DrainGuard Robot - HOTSPOT-ONLY VERSION (NO BLUETOOTH)
// Phone connects to ESP32 WiFi hotspot → controls via HTTP API

// ============================================================================
// INCLUDES
// ============================================================================
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <HardwareSerial.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// ============================================================================
// CONFIGURATION
// ============================================================================

#define DEVICE_ID "DRAIN_GUARD_001"

// WiFi Hotspot — phone connects here
#define AP_SSID     "DrainGuard-Robot"
#define AP_PASSWORD "DrainGuard123"
#define AP_IP       "192.168.4.1"

// Optional ESP32-CAM on the hotspot network
#define CAMERA_IP               "192.168.4.50"
#define CAMERA_PORT             80
#define CAMERA_CHECK_TIMEOUT_MS 1500
#define CAMERA_CHECK_INTERVAL_MS 10000

// Alert SMS number
#define ALERT_PHONE    "+1234567890"
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

#define A9G_RX      33
#define A9G_TX      32

#define A7670_RX    16
#define A7670_TX    17

#define PCA9685_SDA  21
#define PCA9685_SCL  22
#define PCA9685_ADDR 0x40

#define LED_BUILTIN_PIN 2

// ── Timing ───────────────────────────────────────────────────────────────────

#define SENSOR_INTERVAL  2000
#define MOTOR_SPEED      200
#define MOTOR_WIFI_SPEED 100  // low power mode — safe for shared power supply
#define DRAIN_OPEN_MS   5000
#define DRAIN_CLOSE_MS  5000

// ── Ultrasonic ───────────────────────────────────────────────────────────────

#define MAX_DISTANCE 400
#define TANK_HEIGHT  200

// ── Servo channels (PCA9685) ─────────────────────────────────────────────────

#define SERVO_FREQ         60
#define SERVO_BASE          0   // servo1 — base rotation
#define SERVO_SHOULDER      1   // servo2 — shoulder
#define SERVO_ELBOW         2   // servo3 — elbow
#define SERVO_GRIPPER       3   // servo4 — gripper

// Exact values from reference Robot_arm.ino — adjusted for full range of motion
// MIN values extended below home so UP button works
// MAX values extended above reference to allow full DOWN movement
#define SERVO_BASE_MIN       250
#define SERVO_BASE_MAX       450
#define SERVO_SHOULDER_MIN   120   // raised from 80 — prevents hitting mechanical stop
#define SERVO_SHOULDER_MAX   450   // extended beyond 380 for full DOWN range
#define SERVO_ELBOW_MIN      250   // raised from 200 — prevents hitting mechanical stop
#define SERVO_ELBOW_MAX      450   // extended beyond 380 for full DOWN range
#define SERVO_GRIPPER_MIN    350   // extended below 410 for CLOSE
#define SERVO_GRIPPER_MAX    510   // home=410 is between MIN-MAX now

// Home positions (arm starts here on boot)
#define SERVO_BASE_HOME      330
#define SERVO_SHOULDER_HOME  150
#define SERVO_ELBOW_HOME     300
#define SERVO_GRIPPER_HOME   410   // open (middle of range)

// ── Alert thresholds ─────────────────────────────────────────────────────────

#define CRITICAL_DIST 20.0f
#define WARNING_DIST  50.0f

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

WebServer               server(80);
WebSocketsServer        wsServer(81);   // WebSocket on port 81
Adafruit_PWMServoDriver pwm(PCA9685_ADDR);
HardwareSerial          gpsSerial(1);
HardwareSerial          smsSerial(2);

// ============================================================================
// STATE
// ============================================================================

struct SystemState {
  float         distance         = -1;
  float         waterLevel       =  0;
  bool          drainOpen        = false;
  bool          alertSent        = false;
  unsigned long lastSensorUpdate =  0;
} state;

volatile uint16_t basePos = 330; // last commanded base position
volatile uint16_t shoulderPos = 150;
volatile uint16_t elbowPos    = 300;
volatile uint16_t gripperPos  = 410;
volatile uint16_t servoTargetPos[4] = {330, 150, 300, 410};
volatile unsigned long servoNextStepAt[4] = {0, 0, 0, 0};
volatile unsigned long servoManualStopAt[4] = {0, 0, 0, 0};
volatile bool armSequenceRunning = false;

enum AutoSequenceStep {
  AUTO_IDLE, AUTO_OPENING, AUTO_WAITING,
  AUTO_CLOSING, AUTO_HOMING, AUTO_COOLDOWN
};

AutoSequenceStep autoStep            = AUTO_IDLE;
bool             autoModeEnabled     = false;
bool             autoModeOperating   = false;
float            autoDetectionRange  = CRITICAL_DIST;
unsigned long    autoStepTime        = 0;
unsigned long    autoLastOperation   = 0;
const unsigned long AUTO_OPERATION_COOLDOWN_MS = 10000;

unsigned long ledTimer = 0;
bool          ledState = false;

String gpsBuffer;
float  gpsLat      = 0;
float  gpsLon      = 0;
int    gpsSatCount = 0;
bool   gpsValid    = false;

struct CameraState {
  bool available    = false;
  bool streaming    = false;
  int  quality      = 1;
  int  brightness   = 0;
  int  contrast     = 0;
  bool flashEnabled = false;
} cameraState;

bool          cameraAvailable = false;
unsigned long cameraLastCheck = 0;

enum SmsState { SMS_IDLE, SMS_CMGF, SMS_NUMBER, SMS_BODY, SMS_WAIT };
SmsState      smsState  = SMS_IDLE;
unsigned long smsTimer  = 0;
String        smsPending;
String        smsNumber;

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

void     initHotspot();
void     setupAPIEndpoints();
void     setupWebSocket();
void     onWebSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
float    readUltrasonic();
void     openDrain();
void     closeDrain();
void     stopMotors();
uint16_t getServoPosition(uint8_t ch);
uint16_t clampServoPosition(uint8_t ch, int pos);
void     setServo(uint8_t ch, uint16_t pos);
void     setServoImmediate(uint8_t ch, uint16_t pos);
void     startServoMove(uint8_t ch, int direction);
void     stopServoMove(uint8_t ch);
void     updateServoMoves();
void     waitForServoMove(uint8_t ch);
void     moveArmHome();
void     openDrainWithArm();
void     closeDrainWithArm();
void     updateAutoMode();
void     updateCameraStatus();
void     sendSMS(const String &number, const String &msg);
void     checkAlerts();
void     updateGPS();
void     updateSMS();
void     parseGGA(const String &sentence);
void     updateLED();

// ============================================================================
// HOTSPOT INIT
// ============================================================================

void initHotspot() {
  IPAddress ip, gw, sn;
  ip.fromString(AP_IP);
  gw = ip;
  sn.fromString("255.255.255.0");
  WiFi.softAPConfig(ip, gw, sn);
  bool ok = WiFi.softAP(AP_SSID, AP_PASSWORD, 1, false, 4);
  Serial.printf("[AP] Hotspot %s — %s  SSID: %s\n",
                ok ? "started" : "FAILED", AP_IP, AP_SSID);
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== DrainGuard (Hotspot-Only) Starting ===");

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(MOTOR_AIN1, OUTPUT); pinMode(MOTOR_AIN2, OUTPUT);
  pinMode(MOTOR_BIN1, OUTPUT); pinMode(MOTOR_BIN2, OUTPUT);
  pinMode(MOTOR_PWMA, OUTPUT); pinMode(MOTOR_PWMB, OUTPUT);
  pinMode(MOTOR_STBY, OUTPUT);
  stopMotors();

  pinMode(LED_BUILTIN_PIN, OUTPUT);
  digitalWrite(LED_BUILTIN_PIN, HIGH);

  Wire.begin(PCA9685_SDA, PCA9685_SCL);
  pwm.begin();
  pwm.setPWMFreq(SERVO_FREQ);
  delay(10);
  for (uint8_t ch = 0; ch < 4; ++ch) pwm.setPWM(ch, 0, 4096);

  gpsSerial.begin(115200, SERIAL_8N1, A9G_RX, A9G_TX);
  smsSerial.begin(115200, SERIAL_8N1, A7670_RX, A7670_TX);

  WiFi.mode(WIFI_AP);
  WiFi.persistent(false);

  initHotspot();
  setupWebSocket();
  setupAPIEndpoints();
  server.begin();

  Serial.printf("[HTTP] API ready at http://%s\n", AP_IP);
  Serial.printf("  Connect phone to WiFi: %s  /  Password: %s\n\n", AP_SSID, AP_PASSWORD);
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  updateServoMoves();
  updateLED();
  wsServer.loop();      // WebSocket — must be first, handles all real-time commands
  server.handleClient(); // HTTP — status, camera, etc.
  updateGPS();
  updateSMS();

  if (millis() - state.lastSensorUpdate >= SENSOR_INTERVAL) {
    state.distance   = readUltrasonic();
    state.waterLevel = (state.distance > 0) ? (TANK_HEIGHT - state.distance) : 0;
    Serial.printf("[Sensor] dist=%.1fcm  level=%.1fcm  gps=%s\n",
                  state.distance, state.waterLevel, gpsValid ? "fix" : "none");
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
    
    // Send SMS alert
    sendSMS(ALERT_PHONE, "CRITICAL: DrainGuard water level is high!");
    
    // Send push notification via WebSocket to all connected clients
    StaticJsonDocument<256> alert;
    alert["type"] = "alert";
    alert["level"] = "critical";
    alert["message"] = "High water level detected!";
    alert["distance"] = state.distance;
    alert["waterLevel"] = state.waterLevel;
    String alertMsg;
    serializeJson(alert, alertMsg);
    wsServer.broadcastTXT(alertMsg);
    Serial.println("[WS] Alert broadcast to all clients");
    
    if (AUTO_OPEN_DRAIN) openDrain();
  } else if (state.distance > WARNING_DIST) {
    state.alertSent = false;
  }
}

// ============================================================================
// MOTOR FUNCTIONS
// ============================================================================

// Run motor forward for DRAIN_OPEN_MS then stop — in background task
static void drainOpenTask(void *) {
  Serial.println("[Motor] Opening drain");
  digitalWrite(MOTOR_STBY, HIGH);
  digitalWrite(MOTOR_AIN1, HIGH); digitalWrite(MOTOR_AIN2, LOW);
  analogWrite(MOTOR_PWMA, MOTOR_SPEED);
  digitalWrite(MOTOR_BIN1, HIGH); digitalWrite(MOTOR_BIN2, LOW);
  analogWrite(MOTOR_PWMB, MOTOR_SPEED);
  vTaskDelay(pdMS_TO_TICKS(DRAIN_OPEN_MS));
  stopMotors();
  state.drainOpen = true;
  vTaskDelete(nullptr);
}

static void drainCloseTask(void *) {
  Serial.println("[Motor] Closing drain");
  digitalWrite(MOTOR_STBY, HIGH);
  digitalWrite(MOTOR_AIN1, LOW); digitalWrite(MOTOR_AIN2, HIGH);
  analogWrite(MOTOR_PWMA, MOTOR_SPEED);
  digitalWrite(MOTOR_BIN1, LOW); digitalWrite(MOTOR_BIN2, HIGH);
  analogWrite(MOTOR_PWMB, MOTOR_SPEED);
  vTaskDelay(pdMS_TO_TICKS(DRAIN_CLOSE_MS));
  stopMotors();
  state.drainOpen = false;
  vTaskDelete(nullptr);
}

void openDrain() {
  xTaskCreate(drainOpenTask, "drainOpen", 2048, nullptr, 1, nullptr);
}

void closeDrain() {
  xTaskCreate(drainCloseTask, "drainClose", 2048, nullptr, 1, nullptr);
}

void stopMotors() {
  digitalWrite(MOTOR_AIN1, LOW); digitalWrite(MOTOR_AIN2, LOW);
  digitalWrite(MOTOR_BIN1, LOW); digitalWrite(MOTOR_BIN2, LOW);
  analogWrite(MOTOR_PWMA, 0);    analogWrite(MOTOR_PWMB, 0);
  digitalWrite(MOTOR_STBY, LOW);
}

// ============================================================================
// SERVO FUNCTIONS
// All four joints follow Robot_arm.ino: one PWM count every 10 ms.
// Each manual command moves at most 20 counts (200 ms); stop can cancel the ramp.
// Positional PWM stays active afterward so each joint holds its position.
// ============================================================================

#define SERVO_STEP_INTERVAL_MS 10
#define SERVO_STEPS_PER_PRESS 20

// Base servo: small step per press (LEFT/RIGHT buttons)
#define SERVO_BASE_STEPS_PER_PRESS    10   // smaller step for base rotation
// Gripper servo: small step per press (OPEN/CLOSE buttons)
#define SERVO_GRIPPER_STEPS_PER_PRESS 10   // smaller step for claw open/close

// Per-channel speed multipliers (1 = normal, 2 = half speed, 3 = third speed, etc.)
// Increased all to prevent mechanical stress
#define SERVO_BASE_SPEED     2   // slower to protect base motor
#define SERVO_SHOULDER_SPEED 8   // SUPER SLOW — heavy arm section, prevents damage
#define SERVO_ELBOW_SPEED    3   // slow — prevents jerky motion
#define SERVO_GRIPPER_SPEED  2   // moderate — lighter load

void startServoMove(uint8_t ch, int direction) {
  if (ch > SERVO_GRIPPER || armSequenceRunning) return;
  
  // Base and Gripper use smaller steps for precise control
  int steps = SERVO_STEPS_PER_PRESS;
  if (ch == SERVO_BASE)    steps = SERVO_BASE_STEPS_PER_PRESS;
  if (ch == SERVO_GRIPPER) steps = SERVO_GRIPPER_STEPS_PER_PRESS;
  
  const int target = (int)getServoPosition(ch) +
    (direction < 0 ? -steps : steps);
  servoTargetPos[ch] = clampServoPosition(ch, target);
  servoNextStepAt[ch] = millis();
  // Apply per-channel speed to manual stop timeout
  uint8_t speed = 1;
  switch (ch) {
    case SERVO_BASE:     speed = SERVO_BASE_SPEED; break;
    case SERVO_SHOULDER: speed = SERVO_SHOULDER_SPEED; break;
    case SERVO_ELBOW:    speed = SERVO_ELBOW_SPEED; break;
    case SERVO_GRIPPER:  speed = SERVO_GRIPPER_SPEED; break;
  }
  servoManualStopAt[ch] = millis() + SERVO_STEP_INTERVAL_MS * steps * speed;
}

void stopServoMove(uint8_t ch) {
  if (ch > SERVO_GRIPPER || armSequenceRunning) return;
  servoTargetPos[ch] = getServoPosition(ch);
  servoManualStopAt[ch] = 0;
}

void updateServoMoves() {
  const unsigned long now = millis();
  for (uint8_t ch = SERVO_BASE; ch <= SERVO_GRIPPER; ch++) {
    if (servoManualStopAt[ch] && (long)(now - servoManualStopAt[ch]) >= 0)
      stopServoMove(ch);
    const uint16_t current = getServoPosition(ch);
    if (current == servoTargetPos[ch] || (long)(now - servoNextStepAt[ch]) < 0) continue;
    const uint16_t next = current + (servoTargetPos[ch] > current ? 1 : -1);
    pwm.setPWM(ch, 0, next);
    switch (ch) {
      case SERVO_BASE: basePos = next; break;
      case SERVO_SHOULDER: shoulderPos = next; break;
      case SERVO_ELBOW: elbowPos = next; break;
      case SERVO_GRIPPER: gripperPos = next; break;
    }
    // Apply per-channel speed multiplier
    uint8_t speed = 1;
    switch (ch) {
      case SERVO_BASE:     speed = SERVO_BASE_SPEED; break;
      case SERVO_SHOULDER: speed = SERVO_SHOULDER_SPEED; break;
      case SERVO_ELBOW:    speed = SERVO_ELBOW_SPEED; break;
      case SERVO_GRIPPER:  speed = SERVO_GRIPPER_SPEED; break;
    }
    servoNextStepAt[ch] = now + SERVO_STEP_INTERVAL_MS * speed;
  }
}

void waitForServoMove(uint8_t ch) {
  while (getServoPosition(ch) != servoTargetPos[ch])
    vTaskDelay(pdMS_TO_TICKS(SERVO_STEP_INTERVAL_MS));
}

uint16_t clampServoPosition(uint8_t ch, int pos) {
  switch (ch) {
    case SERVO_BASE:     return constrain(pos, SERVO_BASE_MIN, SERVO_BASE_MAX);
    case SERVO_SHOULDER: return constrain(pos, SERVO_SHOULDER_MIN, SERVO_SHOULDER_MAX);
    case SERVO_ELBOW:    return constrain(pos, SERVO_ELBOW_MIN, SERVO_ELBOW_MAX);
    case SERVO_GRIPPER:  return constrain(pos, SERVO_GRIPPER_MIN, SERVO_GRIPPER_MAX);
    default:             return 0;
  }
}

uint16_t getServoPosition(uint8_t ch) {
  switch (ch) {
    case SERVO_BASE:     return basePos;
    case SERVO_SHOULDER: return shoulderPos;
    case SERVO_ELBOW:    return elbowPos;
    case SERVO_GRIPPER:  return gripperPos;
    default:             return 0;
  }
}

void setServo(uint8_t ch, uint16_t val) {
  setServoImmediate(ch, val);
}

void setServoImmediate(uint8_t ch, uint16_t val) {
  if (ch > SERVO_GRIPPER) return;
  val = clampServoPosition(ch, val);
  servoManualStopAt[ch] = 0;
  servoTargetPos[ch] = val;
  servoNextStepAt[ch] = millis();
  if (val == getServoPosition(ch)) pwm.setPWM(ch, 0, val);
}

static void armHomeTask(void *) {
  Serial.println("[Arm] Home");
  setServoImmediate(SERVO_GRIPPER, 410);
  waitForServoMove(SERVO_GRIPPER);
  vTaskDelay(pdMS_TO_TICKS(500));
  setServoImmediate(SERVO_ELBOW, 300);
  waitForServoMove(SERVO_ELBOW);
  vTaskDelay(pdMS_TO_TICKS(500));
  setServoImmediate(SERVO_SHOULDER, 150);
  waitForServoMove(SERVO_SHOULDER);
  vTaskDelay(pdMS_TO_TICKS(500));
  setServoImmediate(SERVO_BASE, 330);
  waitForServoMove(SERVO_BASE);
  armSequenceRunning = false;
  vTaskDelete(nullptr);
}

static void armOpenTask(void *) {
  Serial.println("[Arm] Open drain");
  setServoImmediate(SERVO_BASE, 250);
  waitForServoMove(SERVO_BASE);
  vTaskDelay(pdMS_TO_TICKS(500));
  setServoImmediate(SERVO_SHOULDER, 380);
  waitForServoMove(SERVO_SHOULDER);
  vTaskDelay(pdMS_TO_TICKS(500));
  setServoImmediate(SERVO_ELBOW, 380);
  waitForServoMove(SERVO_ELBOW);
  vTaskDelay(pdMS_TO_TICKS(500));
  setServoImmediate(SERVO_GRIPPER, 510);
  waitForServoMove(SERVO_GRIPPER);
  state.drainOpen = true;
  armSequenceRunning = false;
  vTaskDelete(nullptr);
}

static void armCloseTask(void *) {
  Serial.println("[Arm] Close drain");
  setServoImmediate(SERVO_GRIPPER, 410);
  waitForServoMove(SERVO_GRIPPER);
  vTaskDelay(pdMS_TO_TICKS(500));
  setServoImmediate(SERVO_ELBOW, 300);
  waitForServoMove(SERVO_ELBOW);
  vTaskDelay(pdMS_TO_TICKS(500));
  setServoImmediate(SERVO_SHOULDER, 150);
  waitForServoMove(SERVO_SHOULDER);
  vTaskDelay(pdMS_TO_TICKS(500));
  setServoImmediate(SERVO_BASE, 330);
  waitForServoMove(SERVO_BASE);
  state.drainOpen = false;
  armSequenceRunning = false;
  vTaskDelete(nullptr);
}

void moveArmHome() {
  if (armSequenceRunning) return;
  armSequenceRunning = true;
  for (uint8_t ch = SERVO_BASE; ch <= SERVO_GRIPPER; ch++) {
    servoManualStopAt[ch] = 0;
    servoTargetPos[ch] = getServoPosition(ch);
  }
  if (xTaskCreate(armHomeTask, "armHome", 3072, nullptr, 1, nullptr) != pdPASS)
    armSequenceRunning = false;
}

void openDrainWithArm() {
  if (armSequenceRunning) return;
  armSequenceRunning = true;
  for (uint8_t ch = SERVO_BASE; ch <= SERVO_GRIPPER; ch++) {
    servoManualStopAt[ch] = 0;
    servoTargetPos[ch] = getServoPosition(ch);
  }
  if (xTaskCreate(armOpenTask, "armOpen", 3072, nullptr, 1, nullptr) != pdPASS)
    armSequenceRunning = false;
}

void closeDrainWithArm() {
  if (armSequenceRunning) return;
  armSequenceRunning = true;
  for (uint8_t ch = SERVO_BASE; ch <= SERVO_GRIPPER; ch++) {
    servoManualStopAt[ch] = 0;
    servoTargetPos[ch] = getServoPosition(ch);
  }
  if (xTaskCreate(armCloseTask, "armClose", 3072, nullptr, 1, nullptr) != pdPASS)
    armSequenceRunning = false;
}

// ============================================================================
// AUTO MODE
// ============================================================================

void updateAutoMode() {
  if (!autoModeEnabled) { autoModeOperating = false; autoStep = AUTO_IDLE; return; }
  unsigned long now = millis();
  switch (autoStep) {
    case AUTO_IDLE:
      if (!armSequenceRunning && state.distance > 0 &&
          state.distance <= autoDetectionRange &&
          now - autoLastOperation >= AUTO_OPERATION_COOLDOWN_MS) {
        autoModeOperating = true;
        autoLastOperation = now;
        openDrainWithArm();
        autoStep     = AUTO_OPENING;
        autoStepTime = millis();
      }
      break;
    case AUTO_OPENING:
      if (!armSequenceRunning) { autoStep = AUTO_WAITING; autoStepTime = now; }
      break;
    case AUTO_WAITING:
      if (now - autoStepTime >= 2000) { closeDrainWithArm(); autoStep = AUTO_CLOSING; autoStepTime = millis(); }
      break;
    case AUTO_CLOSING:
      if (!armSequenceRunning) { moveArmHome(); autoStep = AUTO_HOMING; autoStepTime = millis(); }
      break;
    case AUTO_HOMING:
      if (!armSequenceRunning) { autoModeOperating = false; autoStep = AUTO_COOLDOWN; autoStepTime = now; }
      break;
    case AUTO_COOLDOWN:
      if (now - autoStepTime >= 5000) { autoStep = AUTO_IDLE; }
      break;
  }
}

// ============================================================================
// CAMERA STATUS (background probe — runs in FreeRTOS task so HTTP doesn't block loop)
// ============================================================================

static void cameraProbeTask(void *) {
  HTTPClient http;
  String url = String("http://") + CAMERA_IP + ":" + CAMERA_PORT + "/status";
  http.begin(url);
  http.setTimeout(CAMERA_CHECK_TIMEOUT_MS);
  int code = http.GET();
  http.end();
  cameraAvailable       = (code == HTTP_CODE_OK);
  cameraState.available = cameraAvailable;
  vTaskDelete(nullptr);
}

void updateCameraStatus() {
  if (millis() - cameraLastCheck < CAMERA_CHECK_INTERVAL_MS) return;
  cameraLastCheck = millis();
  xTaskCreate(cameraProbeTask, "camProbe", 4096, nullptr, 1, nullptr);
}

// ============================================================================
// ULTRASONIC
// ============================================================================

float readUltrasonic() {
  digitalWrite(TRIG_PIN, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long dur = pulseIn(ECHO_PIN, HIGH, 30000);
  if (dur == 0) return -1;
  float d = dur * 0.034f / 2.0f;
  return (d < 2 || d > MAX_DISTANCE) ? -1 : d;
}

// ============================================================================
// GPS (A9G)
// ============================================================================

void parseGGA(const String &sentence) {
  int field = 0, start = 0;
  String fields[10];
  for (int i = 0; i <= (int)sentence.length() && field < 10; i++) {
    if (i == (int)sentence.length() || sentence[i] == ',') {
      fields[field++] = sentence.substring(start, i);
      start = i + 1;
    }
  }
  if (field < 8 || fields[6] == "0" || fields[6] == "") return;

  float rawLat = fields[2].toFloat();
  int   latDeg = (int)(rawLat / 100);
  gpsLat = latDeg + (rawLat - latDeg * 100) / 60.0f;
  if (fields[3] == "S") gpsLat = -gpsLat;

  float rawLon = fields[4].toFloat();
  int   lonDeg = (int)(rawLon / 100);
  gpsLon = lonDeg + (rawLon - lonDeg * 100) / 60.0f;
  if (fields[5] == "W") gpsLon = -gpsLon;

  gpsSatCount = fields[7].toInt();
  gpsValid    = true;
}

void updateGPS() {
  while (gpsSerial.available()) {
    char c = gpsSerial.read();
    if (c == '\n') {
      gpsBuffer.trim();
      if (gpsBuffer.startsWith("$GPGGA") || gpsBuffer.startsWith("$GNGGA"))
        parseGGA(gpsBuffer);
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
      smsState = SMS_NUMBER; smsTimer = millis(); break;
    case SMS_NUMBER:
      if (millis() - smsTimer < 300) break;
      smsSerial.print("AT+CMGS=\""); smsSerial.print(smsNumber); smsSerial.println("\"");
      smsState = SMS_BODY; smsTimer = millis(); break;
    case SMS_BODY:
      if (millis() - smsTimer < 300) break;
      smsSerial.print(smsPending); smsSerial.write(26);
      smsState = SMS_WAIT; smsTimer = millis(); break;
    case SMS_WAIT:
      if (millis() - smsTimer < 4000) break;
      Serial.printf("[SMS] Sent to %s\n", smsNumber.c_str());
      smsState = SMS_IDLE; break;
  }
}

// ============================================================================
// LED  (slow blink = hotspot active)
// ============================================================================

void updateLED() {
  unsigned long now = millis();
  if (now - ledTimer >= 1000) {
    ledTimer = now;
    ledState = !ledState;
    digitalWrite(LED_BUILTIN_PIN, ledState ? HIGH : LOW);
  }
}

// ============================================================================
// WEBSOCKET HANDLER — port 81, handles all real-time motor/servo commands
// ============================================================================

void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.printf("[WS] Client %u connected\n", num);
      // Send current status on connect
      {
        StaticJsonDocument<256> doc;
        doc["type"]       = "status";
        doc["water_level"] = state.waterLevel;
        doc["distance"]    = state.distance;
        doc["drain_open"]  = state.drainOpen;
        doc["base"]        = basePos;
        doc["shoulder"]    = shoulderPos;
        String out; serializeJson(doc, out);
        wsServer.sendTXT(num, out);
      }
      break;

    case WStype_DISCONNECTED:
      Serial.printf("[WS] Client %u disconnected\n", num);
      // Stop drive motors and any manual joint movement when a controller disconnects.
      stopMotors();
      for (uint8_t ch = SERVO_BASE; ch <= SERVO_GRIPPER; ch++) stopServoMove(ch);
      break;

    case WStype_TEXT: {
      StaticJsonDocument<256> doc;
      if (deserializeJson(doc, payload, length)) return;
      const char *cmd = doc["cmd"] | "";

      if (strcmp(cmd, "motor") == 0) {
        const char *dir = doc["dir"] | "";
        if (strcmp(dir, "forward") == 0) {
          // Continuous — stays on until "stop" is sent
          digitalWrite(MOTOR_STBY, HIGH);
          digitalWrite(MOTOR_AIN1, HIGH); digitalWrite(MOTOR_AIN2, LOW);
          analogWrite(MOTOR_PWMA, MOTOR_WIFI_SPEED);
          digitalWrite(MOTOR_BIN1, HIGH); digitalWrite(MOTOR_BIN2, LOW);
          analogWrite(MOTOR_PWMB, MOTOR_WIFI_SPEED);
        } else if (strcmp(dir, "backward") == 0) {
          // Continuous — stays on until "stop" is sent
          digitalWrite(MOTOR_STBY, HIGH);
          digitalWrite(MOTOR_AIN1, LOW); digitalWrite(MOTOR_AIN2, HIGH);
          analogWrite(MOTOR_PWMA, MOTOR_WIFI_SPEED);
          digitalWrite(MOTOR_BIN1, LOW); digitalWrite(MOTOR_BIN2, HIGH);
          analogWrite(MOTOR_PWMB, MOTOR_WIFI_SPEED);
        } else if (strcmp(dir, "left")  == 0) startMotorPulse(LOW,  HIGH, HIGH, LOW);
        else if   (strcmp(dir, "right") == 0) startMotorPulse(HIGH, LOW,  LOW,  HIGH);
        else if   (strcmp(dir, "stop")  == 0) stopMotors();
        wsServer.sendTXT(num, "{\"type\":\"ack\",\"cmd\":\"motor\"}");

      } else if (strcmp(cmd, "servo") == 0) {
        const char *joint = doc["joint"] | "";
        const char *dir   = doc["dir"]   | "";

        uint8_t ch = 255;
        if      (strcmp(joint, "base")     == 0) ch = SERVO_BASE;
        else if (strcmp(joint, "shoulder") == 0) ch = SERVO_SHOULDER;
        else if (strcmp(joint, "elbow")    == 0) ch = SERVO_ELBOW;
        else if (strcmp(joint, "gripper")  == 0) ch = SERVO_GRIPPER;
        if (ch == 255) return;

        if (strcmp(dir, "stop") == 0) stopServoMove(ch);
        else if (strcmp(dir, "fwd") == 0) startServoMove(ch, 1);
        else if (strcmp(dir, "rev") == 0) startServoMove(ch, -1);
        else return;
        wsServer.sendTXT(num, "{\"type\":\"ack\",\"cmd\":\"servo\"}");

      } else if (strcmp(cmd, "arm") == 0) {
        const char *action = doc["action"] | "";
        if      (strcmp(action, "open")  == 0) openDrainWithArm();
        else if (strcmp(action, "close") == 0) closeDrainWithArm();
        else if (strcmp(action, "home")  == 0) moveArmHome();
        wsServer.sendTXT(num, "{\"type\":\"ack\",\"cmd\":\"arm\"}");

      } else if (strcmp(cmd, "get_status") == 0) {
        StaticJsonDocument<256> resp;
        resp["type"]        = "status";
        resp["water_level"] = state.waterLevel;
        resp["distance"]    = state.distance;
        resp["drain_open"]  = state.drainOpen;
        resp["base"]        = basePos;
        resp["shoulder"]    = shoulderPos;
        String out; serializeJson(resp, out);
        wsServer.sendTXT(num, out);
      }
      break;
    }

    default: break;
  }
}

void setupWebSocket() {
  wsServer.begin();
  wsServer.onEvent(onWebSocketEvent);
  Serial.printf("[WS] WebSocket server started on port 81\n");
}

// ============================================================================
// MOTOR PULSE — runs motor for 350ms per tap, auto-stops, non-blocking
// ============================================================================

#define MOTOR_PULSE_MS   350  // ms per tap

struct MotorPulse {
  uint8_t ain1, ain2, bin1, bin2;
};

static void motorPulseTask(void *arg) {
  MotorPulse *p = (MotorPulse *)arg;
  // Wait 80ms so HTTP response finishes before we draw motor current
  vTaskDelay(pdMS_TO_TICKS(80));
  digitalWrite(MOTOR_STBY, HIGH);
  digitalWrite(MOTOR_AIN1, p->ain1); digitalWrite(MOTOR_AIN2, p->ain2);
  analogWrite(MOTOR_PWMA, MOTOR_WIFI_SPEED);
  digitalWrite(MOTOR_BIN1, p->bin1); digitalWrite(MOTOR_BIN2, p->bin2);
  analogWrite(MOTOR_PWMB, MOTOR_WIFI_SPEED);
  vTaskDelay(pdMS_TO_TICKS(MOTOR_PULSE_MS));
  stopMotors();
  // 80ms pause after stop — lets WiFi stack recover
  vTaskDelay(pdMS_TO_TICKS(80));
  free(p);
  vTaskDelete(nullptr);
}

static void startMotorPulse(uint8_t ain1, uint8_t ain2, uint8_t bin1, uint8_t bin2) {
  MotorPulse *p = (MotorPulse *)malloc(sizeof(MotorPulse));
  if (!p) return;
  p->ain1 = ain1; p->ain2 = ain2;
  p->bin1 = bin1; p->bin2 = bin2;
  xTaskCreate(motorPulseTask, "motorPulse", 2048, p, 1, nullptr);
}

// ============================================================================

void setupAPIEndpoints() {

  // CORS preflight
  server.onNotFound([]() {
    server.sendHeader("Access-Control-Allow-Origin",  "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    if (server.method() == HTTP_OPTIONS) server.send(204);
    else server.send(404, "text/plain", "Not found");
  });

  // GET /api/status
  server.on("/api/status", HTTP_GET, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    StaticJsonDocument<512> doc;
    doc["device_id"]         = DEVICE_ID;
    doc["water_level"]       = state.waterLevel;
    doc["distance"]          = state.distance;
    doc["drain_open"]        = state.drainOpen;
    doc["camera_available"]  = cameraAvailable;
    doc["camera_streaming"]  = cameraState.streaming;
    doc["camera_quality"]    = cameraState.quality;
    doc["latitude"]          = gpsLat;
    doc["longitude"]         = gpsLon;
    doc["satellites"]        = gpsSatCount;
    doc["gps_valid"]         = gpsValid;
    doc["uptime_s"]          = (unsigned long)(millis() / 1000);
    doc["free_heap"]         = (int)ESP.getFreeHeap();
    doc["clients_connected"] = (int)WiFi.softAPgetStationNum();
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

  // ── Motor / Wheel Control ────────────────────────────────────────────────

  // POST /api/motor/forward
  server.on("/api/motor/forward", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    startMotorPulse(HIGH, LOW, HIGH, LOW);
    server.send(200, "application/json", "{\"status\":\"forward\"}");
  });

  // POST /api/motor/backward
  server.on("/api/motor/backward", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    startMotorPulse(LOW, HIGH, LOW, HIGH);
    server.send(200, "application/json", "{\"status\":\"backward\"}");
  });

  // POST /api/motor/left
  server.on("/api/motor/left", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    startMotorPulse(LOW, HIGH, HIGH, LOW);
    server.send(200, "application/json", "{\"status\":\"left\"}");
  });

  // POST /api/motor/right
  server.on("/api/motor/right", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    startMotorPulse(HIGH, LOW, LOW, HIGH);
    server.send(200, "application/json", "{\"status\":\"right\"}");
  });

  // POST /api/motor/stop
  server.on("/api/motor/stop", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    stopMotors();
    server.send(200, "application/json", "{\"status\":\"stopped\"}");
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

  // All manual joints use the same 20-count (200 ms) ramp per press.
  server.on("/api/servo/step", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    const String joint = server.arg("joint");
    const String direction = server.arg("direction");
    uint8_t ch = 255;
    if (joint == "base") ch = SERVO_BASE;
    else if (joint == "shoulder") ch = SERVO_SHOULDER;
    else if (joint == "elbow") ch = SERVO_ELBOW;
    else if (joint == "gripper") ch = SERVO_GRIPPER;
    if (ch == 255 || (direction != "-1" && direction != "1")) {
      server.send(400, "application/json", "{\"error\":\"invalid joint or direction\"}"); return;
    }
    startServoMove(ch, direction == "-1" ? -1 : 1);
    server.send(200, "application/json", "{\"status\":\"moving\"}");
  });
  server.on("/api/servo/stop", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    const String joint = server.arg("joint");
    uint8_t ch = 255;
    if (joint == "base") ch = SERVO_BASE;
    else if (joint == "shoulder") ch = SERVO_SHOULDER;
    else if (joint == "elbow") ch = SERVO_ELBOW;
    else if (joint == "gripper") ch = SERVO_GRIPPER;
    if (ch == 255) {
      server.send(400, "application/json", "{\"error\":\"invalid joint\"}"); return;
    }
    stopServoMove(ch);
    server.send(200, "application/json", "{\"status\":\"stopped\"}");
  });

  // Keep the original base routes for older app builds.
  server.on("/api/servo/base/left", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    startServoMove(SERVO_BASE, 1);
    server.send(200, "application/json", "{\"status\":\"moving\"}");
  });
  server.on("/api/servo/base/right", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    startServoMove(SERVO_BASE, -1);
    server.send(200, "application/json", "{\"status\":\"moving\"}");
  });
  server.on("/api/servo/base/stop", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    stopServoMove(SERVO_BASE);
    server.send(200, "application/json", "{\"status\":\"stopped\"}");
  });

  // Legacy position endpoint: ramp to the requested positional PWM value.
  // POST /api/servo/base?position=330
  server.on("/api/servo/base", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    if (!server.hasArg("position")) {
      server.send(400, "application/json", "{\"error\":\"missing position\"}"); return;
    }
    int pos = constrain(server.arg("position").toInt(), SERVO_BASE_MIN, SERVO_BASE_MAX);
    setServoImmediate(SERVO_BASE, pos);
    server.send(200, "application/json", "{\"status\":\"moved\",\"position\":" + String(pos) + "}");
  });

  // POST /api/servo/shoulder?position=200
  server.on("/api/servo/shoulder", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    if (!server.hasArg("position")) {
      server.send(400, "application/json", "{\"error\":\"missing position\"}"); return;
    }
    int pos = constrain(server.arg("position").toInt(), SERVO_SHOULDER_MIN, SERVO_SHOULDER_MAX);
    setServoImmediate(SERVO_SHOULDER, pos);
    server.send(200, "application/json", "{\"status\":\"moved\",\"position\":" + String(pos) + "}");
  });

  // POST /api/servo/elbow?position=340
  server.on("/api/servo/elbow", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    if (!server.hasArg("position")) {
      server.send(400, "application/json", "{\"error\":\"missing position\"}"); return;
    }
    int pos = constrain(server.arg("position").toInt(), SERVO_ELBOW_MIN, SERVO_ELBOW_MAX);
    setServoImmediate(SERVO_ELBOW, pos);
    server.send(200, "application/json", "{\"status\":\"moved\",\"position\":" + String(pos) + "}");
  });

  // POST /api/servo/gripper?position=460
  server.on("/api/servo/gripper", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    if (!server.hasArg("position")) {
      server.send(400, "application/json", "{\"error\":\"missing position\"}"); return;
    }
    int pos = constrain(server.arg("position").toInt(), SERVO_GRIPPER_MIN, SERVO_GRIPPER_MAX);
    setServoImmediate(SERVO_GRIPPER, pos);
    server.send(200, "application/json", "{\"status\":\"moved\",\"position\":" + String(pos) + "}");
  });

  // GET /api/servo/status
  server.on("/api/servo/status", HTTP_GET, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    StaticJsonDocument<192> doc;
    doc["base"]     = basePos;
    doc["shoulder"] = shoulderPos;
    doc["elbow"]    = elbowPos;
    doc["gripper"]  = gripperPos;
    doc["arm_busy"] = armSequenceRunning;
    String out; serializeJson(doc, out);
    server.send(200, "application/json", out);
  });

  // POST /api/auto/start
  server.on("/api/auto/start", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    autoModeEnabled = true;
    server.send(200, "application/json", "{\"status\":\"started\"}");
  });

  // POST /api/auto/stop
  server.on("/api/auto/stop", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    autoModeEnabled   = false;
    autoModeOperating = false;
    autoStep          = AUTO_IDLE;
    server.send(200, "application/json", "{\"status\":\"stopped\"}");
  });

  // GET /api/auto/status
  server.on("/api/auto/status", HTTP_GET, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    StaticJsonDocument<160> doc;
    doc["enabled"]         = autoModeEnabled;
    doc["operating"]       = autoModeOperating;
    doc["detection_range"] = autoDetectionRange;
    String out; serializeJson(doc, out);
    server.send(200, "application/json", out);
  });

  // POST /api/auto/range?value=30.5
  server.on("/api/auto/range", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    if (!server.hasArg("value")) {
      server.send(400, "application/json", "{\"error\":\"missing value\"}"); return;
    }
    autoDetectionRange = server.arg("value").toFloat();
    server.send(200, "application/json",
      "{\"status\":\"updated\",\"range\":" + String(autoDetectionRange) + "}");
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
    doc["available"]  = cameraAvailable;
    doc["streaming"]  = cameraState.streaming;
    doc["quality"]    = cameraState.quality;
    doc["brightness"] = cameraState.brightness;
    doc["contrast"]   = cameraState.contrast;
    doc["flash"]      = cameraState.flashEnabled;
    String out; serializeJson(doc, out);
    server.send(200, "application/json", out);
  });

  // POST /api/camera/capture
  server.on("/api/camera/capture", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    if (!cameraAvailable) {
      server.send(503, "application/json", "{\"error\":\"camera not available\"}"); return;
    }
    server.send(200, "application/json", "{\"status\":\"captured\"}");
  });

  // POST /api/camera/stream/start
  server.on("/api/camera/stream/start", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    if (!cameraAvailable) {
      server.send(503, "application/json", "{\"error\":\"camera not available\"}"); return;
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
