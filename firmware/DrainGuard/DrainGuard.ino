// DrainGuard Robot - Single File Version for Arduino IDE
// ESP32-CAM is NOT required. Camera tab in web app will show "not available".

// ============================================================================
// INCLUDES
// ============================================================================
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <HardwareSerial.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Preferences.h>

// ============================================================================
// CONFIGURATION
// ============================================================================

// Fallback WiFi — only used if no credentials saved in NVS.
// Use BLE provisioning from your phone to set the real network.
#define DEFAULT_WIFI_SSID     ""
#define DEFAULT_WIFI_PASSWORD ""

#define DEVICE_ID "DRAIN_GUARD_001"

// BLE provisioning — unique name uses last 4 hex digits of chip ID
#define BLE_DEVICE_PREFIX "DrainGuard"

// BLE service / characteristic UUIDs
#define PROV_SERVICE_UUID "7b0d1001-5f6b-4c4f-9a7e-2f3b4d5e6f70"
#define PROV_RX_UUID      "7b0d1002-5f6b-4c4f-9a7e-2f3b4d5e6f70"  // phone → ESP32
#define PROV_TX_UUID      "7b0d1003-5f6b-4c4f-9a7e-2f3b4d5e6f70"  // ESP32 → phone

// Private hotspot — always on, web app connects here
#define AP_SSID     "DrainGuard-Robot"
#define AP_PASSWORD "DrainGuard123"
#define AP_IP       "192.168.4.1"

// Alert SMS number
#define ALERT_PHONE "+1234567890"
#define AUTO_OPEN_DRAIN true

// WiFi connection timeout
#define WIFI_CONNECT_TIMEOUT_MS 20000

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

// LED indicators — GPIO 2 is the built-in blue LED on ESP32 DevKit V1
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

#define SERVO_BASE_MIN     150
#define SERVO_BASE_MAX     450
#define SERVO_SHOULDER_MIN 150
#define SERVO_SHOULDER_MAX 380
#define SERVO_ELBOW_MIN    300
#define SERVO_ELBOW_MAX    380
#define SERVO_GRIPPER_MIN  410
#define SERVO_GRIPPER_MAX  510

// ── Alert thresholds (sensor distance in cm) ─────────────────────────────────

#define CRITICAL_DIST 20.0f
#define WARNING_DIST  50.0f

// ── NVS keys ─────────────────────────────────────────────────────────────────

#define NVS_NAMESPACE "drainguard"
#define NVS_KEY_SSID  "ssid"
#define NVS_KEY_PASS  "password"
#define NVS_KEY_SAVED "configured"

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

WebServer               server(80);
Adafruit_PWMServoDriver pwm(PCA9685_ADDR);
HardwareSerial          gpsSerial(1);   // Serial1 — A9G GPS
HardwareSerial          smsSerial(2);   // Serial2 — A7670 SMS
Preferences             prefs;

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

// ============================================================================
// BLE PROVISIONING STATE
// ============================================================================

BLEServer         *bleServer     = nullptr;
BLECharacteristic *bleTx         = nullptr;
bool               bleConnected  = false;
bool               bleAdvRestart = false;
unsigned long      bleDisconnAt  = 0;

// Non-blocking WiFi state machine
enum WifiConnState { WCS_IDLE, WCS_CONNECTING, WCS_CONNECTED, WCS_FAILED };
WifiConnState wifiConnState   = WCS_IDLE;
unsigned long wifiConnStartAt = 0;
String        pendingSsid;
String        pendingPass;
bool          saveOnSuccess   = false;

bool          wifiScanInProgress = false;
String        deviceName;

// ── LED state ─────────────────────────────────────────────────────────────
unsigned long ledTimer    = 0;
bool          ledState    = false;

// ── GPS state (populated by updateGPS in loop) ────────────────────────────
String  gpsBuffer;
float   gpsLat      = 0;
float   gpsLon      = 0;
int     gpsSatCount = 0;
bool    gpsValid    = false;

// ── SMS state machine ─────────────────────────────────────────────────────
enum SmsState { SMS_IDLE, SMS_CMGF, SMS_NUMBER, SMS_BODY, SMS_WAIT };
SmsState      smsState  = SMS_IDLE;
unsigned long smsTimer  = 0;
String        smsPending;
String        smsNumber;

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

void  initHotspot();
void  initBLE();
void  handleBleWrite(const String &json);
void  startWifiConnection(const String &ssid, const String &pass, bool save);
void  updateWifiState();
void  updateWifiScan();
void  restartBleAdv();
void  bleNotify(const String &json);
void  loadSavedCredentials(String &ssid, String &pass);
bool  saveCredentials(const String &ssid, const String &pass);
void  setupAPIEndpoints();
float readUltrasonic();
void  openDrain();
void  closeDrain();
void  stopMotors();
void  setServo(uint8_t ch, uint16_t pos);
void  sendSMS(const String &number, const String &msg);
void  checkAlerts();
void  updateGPS();
void  updateSMS();
void  parseGGA(const String &sentence);
void  updateLED();

// ============================================================================
// BLE CALLBACKS
// ============================================================================

class BleServerCB : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) override {
    bleConnected  = true;
    bleAdvRestart = false;
    Serial.println("[BLE] Phone connected");

    // Stop advertising while connected — reduces interference
    BLEDevice::getAdvertising()->stop();

    // DO NOT notify immediately here — Android needs a moment
    // to complete service discovery before it can receive notifications.
    // The mobile app should READ the TX characteristic after connecting
    // to get the initial status.
  }

  void onDisconnect(BLEServer *pServer) override {
    bleConnected  = false;
    bleAdvRestart = true;
    bleDisconnAt  = millis();
    Serial.println("[BLE] Phone disconnected");
  }
};

class BleWriteCB : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *ch) override {
    String raw = String(ch->getValue().c_str());
    ch->setValue("");
    if (raw.length() > 0) handleBleWrite(raw);
  }
};

// ============================================================================
// BLE PROVISIONING LOGIC
// ============================================================================

// Commands phone sends (JSON written to RX characteristic):
//   {"command":"scan_wifi"}
//   {"command":"set_wifi","ssid":"MyNet","password":"secret"}
//
// Responses ESP32 notifies back (TX characteristic):
//   {"status":"ready","device":"DrainGuard-XXXX","hotspot_ip":"192.168.4.1"}
//   {"status":"scanning_wifi"}
//   {"status":"network","ssid":"MyNet","rssi":-62,"secure":true}
//   {"status":"scan_complete","count":5}
//   {"status":"connecting","ssid":"MyNet"}
//   {"status":"connected","ssid":"MyNet","ip":"192.168.1.55","saved":true}
//   {"status":"failed","reason":"authentication_failed"}

void handleBleWrite(const String &json) {
  Serial.printf("[BLE] Received: %s\n", json.c_str());

  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, json) != DeserializationError::Ok) {
    bleNotify("{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
    return;
  }

  const char *cmd = doc["command"] | "";

  // scan_wifi — return list of visible 2.4 GHz networks
  if (strcmp(cmd, "scan_wifi") == 0) {
    if (wifiScanInProgress) {
      bleNotify("{\"status\":\"scanning_wifi\",\"message\":\"Already scanning\"}");
      return;
    }
    WiFi.mode(WIFI_AP_STA);
    if (WiFi.scanNetworks(true, false) == WIFI_SCAN_FAILED) {
      bleNotify("{\"status\":\"scan_failed\",\"message\":\"Could not start scan\"}");
      return;
    }
    wifiScanInProgress = true;
    bleNotify("{\"status\":\"scanning_wifi\"}");
    return;
  }

  // set_wifi — connect ESP32 to the given network and save credentials
  if (strcmp(cmd, "set_wifi") == 0) {
    const char *ssid = doc["ssid"] | "";
    const char *pass = doc["password"] | "";

    if (strlen(ssid) == 0 || strlen(ssid) > 32) {
      bleNotify("{\"status\":\"error\",\"message\":\"SSID must be 1-32 characters\"}");
      return;
    }
    if (strlen(pass) > 63) {
      bleNotify("{\"status\":\"error\",\"message\":\"Password too long (max 63)\"}");
      return;
    }
    startWifiConnection(String(ssid), String(pass), true);
    return;
  }

  bleNotify("{\"status\":\"error\",\"message\":\"Unknown command\"}");
}

void startWifiConnection(const String &ssid, const String &pass, bool save) {
  pendingSsid     = ssid;
  pendingPass     = pass;
  saveOnSuccess   = save;
  wifiConnState   = WCS_CONNECTING;
  wifiConnStartAt = millis();

  WiFi.disconnect(false, false);
  WiFi.mode(WIFI_AP_STA);         // keep hotspot alive while connecting
  WiFi.begin(ssid.c_str(), pass.c_str());

  StaticJsonDocument<128> doc;
  doc["status"] = "connecting";
  doc["ssid"]   = ssid;
  String out; serializeJson(doc, out);
  bleNotify(out);

  Serial.printf("[WiFi] Connecting to: %s\n", ssid.c_str());
}

void updateWifiState() {
  if (wifiConnState != WCS_CONNECTING) return;

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnState = WCS_CONNECTED;
    bool saved = false;
    if (saveOnSuccess) saved = saveCredentials(pendingSsid, pendingPass);
    pendingPass = "";

    StaticJsonDocument<256> doc;
    doc["status"] = "connected";
    doc["ssid"]   = pendingSsid;
    doc["ip"]     = WiFi.localIP().toString();
    doc["saved"]  = saved;
    String out; serializeJson(doc, out);
    bleNotify(out);

    Serial.printf("[WiFi] Connected! IP: %s  saved: %s\n",
                  WiFi.localIP().toString().c_str(), saved ? "yes" : "no");
    return;
  }

  if (millis() - wifiConnStartAt >= WIFI_CONNECT_TIMEOUT_MS) {
    wl_status_t s = WiFi.status();
    wifiConnState = WCS_FAILED;
    pendingPass   = "";
    WiFi.disconnect(false, false);

    const char *reason = "timeout";
    if      (s == WL_NO_SSID_AVAIL)  reason = "network_not_found";
    else if (s == WL_CONNECT_FAILED)  reason = "authentication_failed";
    else if (s == WL_CONNECTION_LOST) reason = "connection_lost";

    StaticJsonDocument<192> doc;
    doc["status"]  = "failed";
    doc["reason"]  = reason;
    doc["message"] = "Could not connect within 20 seconds";
    String out; serializeJson(doc, out);
    bleNotify(out);

    Serial.printf("[WiFi] Failed: %s\n", reason);
  }
}

void updateWifiScan() {
  if (!wifiScanInProgress) return;

  int n = WiFi.scanComplete();
  if (n == WIFI_SCAN_RUNNING) return;

  if (n < 0) {
    wifiScanInProgress = false;
    bleNotify("{\"status\":\"scan_failed\"}");
    return;
  }

  for (int i = 0; i < n; i++) {
    if (WiFi.SSID(i).length() == 0) continue;
    StaticJsonDocument<192> doc;
    doc["status"] = "network";
    doc["ssid"]   = WiFi.SSID(i);
    doc["rssi"]   = WiFi.RSSI(i);
    doc["secure"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
    String out; serializeJson(doc, out);
    bleNotify(out);
    delay(15);
  }

  WiFi.scanDelete();
  wifiScanInProgress = false;

  StaticJsonDocument<64> doc;
  doc["status"] = "scan_complete";
  doc["count"]  = n;
  String out; serializeJson(doc, out);
  bleNotify(out);
}

void restartBleAdv() {
  if (!bleAdvRestart) return;
  if (millis() - bleDisconnAt < 500) return;  // wait 500ms after disconnect
  bleAdvRestart = false;
  BLEDevice::startAdvertising();
  Serial.println("[BLE] Advertising restarted");
}

void bleNotify(const String &json) {
  if (bleTx == nullptr) return;
  bleTx->setValue(json.c_str());
  if (bleConnected) bleTx->notify();
}

// ============================================================================
// NVS CREDENTIAL STORAGE
// ============================================================================

void loadSavedCredentials(String &ssid, String &pass) {
  if (!prefs.begin(NVS_NAMESPACE, true)) return;
  bool configured = prefs.getBool(NVS_KEY_SAVED, false);
  if (configured) {
    ssid = prefs.getString(NVS_KEY_SSID, "");
    pass = prefs.getString(NVS_KEY_PASS, "");
  }
  prefs.end();
  if (configured && ssid.length() > 0)
    Serial.printf("[NVS] Loaded SSID: %s\n", ssid.c_str());
}

bool saveCredentials(const String &ssid, const String &pass) {
  if (!prefs.begin(NVS_NAMESPACE, false)) return false;
  bool ok = prefs.putString(NVS_KEY_SSID, ssid) > 0
         && prefs.putString(NVS_KEY_PASS, pass) > 0
         && prefs.putBool(NVS_KEY_SAVED, true)  > 0;
  prefs.end();
  if (ok) Serial.printf("[NVS] Saved SSID: %s\n", ssid.c_str());
  return ok;
}

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
  Serial.printf("[AP] Hotspot %s — %s\n", ok ? "started" : "FAILED", AP_IP);
}

void initBLE() {
  uint64_t chipId = ESP.getEfuseMac();
  char suffix[5];
  snprintf(suffix, sizeof(suffix), "%04X", (uint16_t)(chipId & 0xFFFF));
  deviceName = String(BLE_DEVICE_PREFIX) + "-" + suffix;

  BLEDevice::init(deviceName.c_str());

  // Create server first, THEN set MTU
  bleServer = BLEDevice::createServer();
  bleServer->setCallbacks(new BleServerCB());

  // Request larger MTU so longer JSON payloads fit in one packet
  BLEDevice::setMTU(185);

  BLEService *svc = bleServer->createService(PROV_SERVICE_UUID);

  // TX — ESP32 notifies phone (status updates)
  bleTx = svc->createCharacteristic(
    PROV_TX_UUID,
    BLECharacteristic::PROPERTY_READ   |
    BLECharacteristic::PROPERTY_NOTIFY |
    BLECharacteristic::PROPERTY_INDICATE
  );
  bleTx->addDescriptor(new BLE2902());

  // RX — phone writes WiFi credentials/commands
  // WRITE_NR removed — Android uses WRITE_TYPE_DEFAULT which requires a response
  BLECharacteristic *bleRx = svc->createCharacteristic(
    PROV_RX_UUID,
    BLECharacteristic::PROPERTY_WRITE
  );
  bleRx->setCallbacks(new BleWriteCB());

  svc->start();

  // Set a short initial value so READ works immediately on connect
  bleTx->setValue("{\"status\":\"booting\"}");

  BLEAdvertising *adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(PROV_SERVICE_UUID);
  adv->setScanResponse(true);
  adv->setMinPreferred(0x06);
  adv->setMaxPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.printf("[BLE] Advertising as: %s\n", deviceName.c_str());
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== DrainGuard Starting ===");

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
  digitalWrite(LED_BUILTIN_PIN, LOW);

  // I2C + PCA9685
  Wire.begin(PCA9685_SDA, PCA9685_SCL);
  pwm.begin();
  pwm.setPWMFreq(SERVO_FREQ);
  delay(10);

  // Serial modules
  gpsSerial.begin(115200, SERIAL_8N1, A9G_RX, A9G_TX);
  smsSerial.begin(115200, SERIAL_8N1, A7670_RX, A7670_TX);

  // WiFi: AP + STA (hotspot always on, station connects to internet)
  WiFi.mode(WIFI_AP_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);

  initHotspot();
  initBLE();

  // Try saved credentials → fallback to compile-time default → wait for BLE
  String savedSsid, savedPass;
  loadSavedCredentials(savedSsid, savedPass);

  if (savedSsid.length() > 0) {
    startWifiConnection(savedSsid, savedPass, false);
  } else {
    Serial.println("[WiFi] No credentials — open BLE app and send WiFi details");
  }

  setupAPIEndpoints();
  server.begin();
  Serial.printf("[HTTP] API ready at http://%s\n", AP_IP);
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  // BLE provisioning tasks (all non-blocking)
  updateWifiState();
  updateWifiScan();
  restartBleAdv();

  // LED status indicator
  updateLED();

  // HTTP server
  server.handleClient();

  // GPS — read serial bytes whenever available
  updateGPS();

  // SMS state machine — non-blocking send
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
}

// ============================================================================
// ALERT LOGIC
// ============================================================================

void checkAlerts() {
  if (state.distance <= 0) return;   // invalid reading

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

void setServo(uint8_t ch, uint16_t pos) {
  pwm.setPWM(ch, 0, pos);
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
// GPS (A9G) — reads NMEA sentences from Serial1
// ============================================================================

// Parse $GPGGA or $GNGGA sentence for lat/lon/satellites
void parseGGA(const String &sentence) {
  // Field indices: $GPGGA,time,lat,NS,lon,EW,fix,sats,...
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
  if (fields[6] == "0" || fields[6] == "") return; // no fix

  // Latitude: DDMM.MMMM
  float rawLat = fields[2].toFloat();
  int   latDeg = (int)(rawLat / 100);
  float latMin = rawLat - latDeg * 100;
  gpsLat = latDeg + latMin / 60.0f;
  if (fields[3] == "S") gpsLat = -gpsLat;

  // Longitude: DDDMM.MMMM
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
      if (gpsBuffer.length() > 120) gpsBuffer = ""; // overflow guard
    }
  }
}

// ============================================================================
// SMS (A7670) — non-blocking send via state machine
// ============================================================================

// Queue one SMS — call this instead of sending inline
void sendSMS(const String &number, const String &msg) {
  if (smsState != SMS_IDLE) return; // already sending — drop duplicate
  smsNumber  = number;
  smsPending = msg;
  smsState   = SMS_CMGF;
  smsTimer   = millis();
  Serial.printf("[SMS] Queued to %s\n", number.c_str());
}

// Call from loop() — advances the SMS send state machine without blocking
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
      smsSerial.write(26); // Ctrl+Z
      smsState = SMS_WAIT;
      smsTimer = millis();
      break;

    case SMS_WAIT:
      if (millis() - smsTimer < 4000) break; // wait for modem response
      Serial.printf("[SMS] Sent to %s\n", smsNumber.c_str());
      smsState = SMS_IDLE;
      break;
  }
}

// ============================================================================
// LED STATUS INDICATOR (GPIO 2 — built-in blue LED)
// ============================================================================
//
//  Pattern               Meaning
//  ─────────────────     ──────────────────────────────────────────
//  Slow blink (1s)       Waiting — no BLE and no WiFi
//  Fast blink (200ms)    BLE advertising — waiting for phone to connect
//  SOLID ON              BLE phone connected ✅
//  Double blink          WiFi connected to internet ✅
//
void updateLED() {
  unsigned long now = millis();

  if (bleConnected) {
    // SOLID ON — phone is connected via BLE
    digitalWrite(LED_BUILTIN_PIN, HIGH);
    ledState = true;

  } else if (WiFi.status() == WL_CONNECTED) {
    // Double blink every 1.5s — WiFi connected to internet
    unsigned long t = now % 1500;
    bool on = (t < 100) || (t > 200 && t < 300);
    if (on != ledState) {
      ledState = on;
      digitalWrite(LED_BUILTIN_PIN, on ? HIGH : LOW);
    }

  } else if (wifiConnState == WCS_CONNECTING) {
    // Fast blink 100ms — connecting to WiFi
    if (now - ledTimer >= 100) {
      ledTimer = now;
      ledState = !ledState;
      digitalWrite(LED_BUILTIN_PIN, ledState ? HIGH : LOW);
    }

  } else {
    // Slow blink 800ms — idle, advertising BLE, waiting for provisioning
    if (now - ledTimer >= 800) {
      ledTimer = now;
      ledState = !ledState;
      digitalWrite(LED_BUILTIN_PIN, ledState ? HIGH : LOW);
    }
  }
}

// ============================================================================
// HTTP API ENDPOINTS
// ============================================================================

void setupAPIEndpoints() {
  // CORS preflight — required for browsers
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
    StaticJsonDocument<320> doc;
    doc["device_id"]        = DEVICE_ID;
    doc["water_level"]      = state.waterLevel;
    doc["distance"]         = state.distance;
    doc["drain_open"]       = state.drainOpen;
    doc["wifi_connected"]   = (WiFi.status() == WL_CONNECTED);
    doc["wifi_ip"]          = (WiFi.status() == WL_CONNECTED)
                              ? WiFi.localIP().toString() : "";
    doc["camera_available"] = false;
    doc["latitude"]         = gpsLat;
    doc["longitude"]        = gpsLon;
    doc["satellites"]       = gpsSatCount;
    doc["gps_valid"]        = gpsValid;
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

  // POST /api/servo/base?position=330
  server.on("/api/servo/base", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    if (!server.hasArg("position")) { server.send(400, "application/json", "{\"error\":\"missing position\"}"); return; }
    int pos = constrain(server.arg("position").toInt(), SERVO_BASE_MIN, SERVO_BASE_MAX);
    setServo(SERVO_BASE, pos);
    server.send(200, "application/json", "{\"status\":\"moved\",\"position\":" + String(pos) + "}");
  });

  // POST /api/servo/shoulder?position=200
  server.on("/api/servo/shoulder", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    if (!server.hasArg("position")) { server.send(400, "application/json", "{\"error\":\"missing position\"}"); return; }
    int pos = constrain(server.arg("position").toInt(), SERVO_SHOULDER_MIN, SERVO_SHOULDER_MAX);
    setServo(SERVO_SHOULDER, pos);
    server.send(200, "application/json", "{\"status\":\"moved\",\"position\":" + String(pos) + "}");
  });

  // POST /api/servo/elbow?position=340
  server.on("/api/servo/elbow", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    if (!server.hasArg("position")) { server.send(400, "application/json", "{\"error\":\"missing position\"}"); return; }
    int pos = constrain(server.arg("position").toInt(), SERVO_ELBOW_MIN, SERVO_ELBOW_MAX);
    setServo(SERVO_ELBOW, pos);
    server.send(200, "application/json", "{\"status\":\"moved\",\"position\":" + String(pos) + "}");
  });

  // POST /api/servo/gripper?position=460
  server.on("/api/servo/gripper", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    if (!server.hasArg("position")) { server.send(400, "application/json", "{\"error\":\"missing position\"}"); return; }
    int pos = constrain(server.arg("position").toInt(), SERVO_GRIPPER_MIN, SERVO_GRIPPER_MAX);
    setServo(SERVO_GRIPPER, pos);
    server.send(200, "application/json", "{\"status\":\"moved\",\"position\":" + String(pos) + "}");
  });

  // GET /api/camera/stream — no camera, return not_available
  server.on("/api/camera/stream", HTTP_GET, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json",
      "{\"available\":false,\"message\":\"ESP32-CAM not connected\"}");
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
}
