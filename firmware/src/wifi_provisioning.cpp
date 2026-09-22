#include "wifi_provisioning.h"

#include <ArduinoJson.h>
#include <BLE2902.h>
#include <BLESecurity.h>
#include <WiFi.h>
#include "config.h"

namespace {
constexpr size_t MAX_PROVISIONING_PAYLOAD = 768;
constexpr char PREFERENCES_NAMESPACE[] = "drainguard";

// Arduino-ESP32 moved setEncryptionLevel() from BLEDevice to BLESecurity.
// Select the available API at compile time so both older PlatformIO cores and
// newer Arduino IDE cores remain supported.
template <typename DeviceType, typename SecurityType>
auto setBleEncryptionLevel(esp_ble_sec_act_t level, int)
  -> decltype(DeviceType::setEncryptionLevel(level), void()) {
  DeviceType::setEncryptionLevel(level);
}

template <typename DeviceType, typename SecurityType>
auto setBleEncryptionLevel(esp_ble_sec_act_t level, long)
  -> decltype(SecurityType::setEncryptionLevel(level), void()) {
  SecurityType::setEncryptionLevel(level);
}

class ProvisioningSecurityCallbacks : public BLESecurityCallbacks {
public:
  uint32_t onPassKeyRequest() override {
    // NoInputNoOutput uses Just Works, so a passkey is never expected.
    return 0;
  }

  void onPassKeyNotify(uint32_t passKey) override {
    (void)passKey;
  }

  bool onSecurityRequest() override {
    return true;
  }

  void onAuthenticationComplete(esp_ble_auth_cmpl_t result) override {
    if (result.success) {
      Serial.println("BLE link encrypted and bonded");
    } else {
      Serial.printf(
        "BLE pairing failed (reason 0x%02X); forget the device on the phone and retry\n",
        result.fail_reason
      );
    }
  }

  bool onConfirmPIN(uint32_t pin) override {
    (void)pin;
    return true;
  }
};

class ProvisioningServerCallbacks : public BLEServerCallbacks {
public:
  explicit ProvisioningServerCallbacks(WiFiProvisioningManager *manager) : manager(manager) {}

  void onConnect(BLEServer *server) override {
    (void)server;
    manager->onBleConnected();
  }

  void onDisconnect(BLEServer *server) override {
    (void)server;
    manager->onBleDisconnected();
  }

private:
  WiFiProvisioningManager *manager;
};

class ProvisioningWriteCallbacks : public BLECharacteristicCallbacks {
public:
  explicit ProvisioningWriteCallbacks(WiFiProvisioningManager *manager) : manager(manager) {}

  void onWrite(BLECharacteristic *characteristic) override {
    auto value = characteristic->getValue();
    manager->onBleWrite(String(value.c_str()));
    characteristic->setValue("");
  }

private:
  WiFiProvisioningManager *manager;
};
} // namespace

WiFiProvisioningManager::WiFiProvisioningManager()
  : commandQueue(nullptr),
    controllerCommandQueue(nullptr),
    bleServer(nullptr),
    statusCharacteristic(nullptr),
    connectionState(WIFI_IDLE),
    connectionStartedAt(0),
    bleDisconnectedAt(0),
    persistPendingCredentials(false),
    hasStoredConfiguration(false),
    bleClientConnected(false),
    advertisingRestartPending(false),
    wifiScanInProgress(false),
    hotspotActive(false) {}

void WiFiProvisioningManager::begin(
  const char *defaultSsid,
  const char *defaultPassword,
  const char *defaultApiEndpoint
) {
  configuredApiEndpoint = defaultApiEndpoint;
  commandQueue = xQueueCreate(3, sizeof(ProvisioningCommand));
  controllerCommandQueue = xQueueCreate(8, sizeof(ControllerCommand));

  uint64_t chipId = ESP.getEfuseMac();
  char suffix[5];
  snprintf(suffix, sizeof(suffix), "%04X", static_cast<unsigned int>(chipId & 0xFFFF));
  deviceName = "DrainGuard-" + String(suffix);

  loadStoredConfiguration();

  WiFi.persistent(false);
  WiFi.mode(WIFI_AP_STA);
  WiFi.setAutoReconnect(true);
  initializeHotspot();
  initializeBle();

  if (hasStoredConfiguration) {
    startWifiConnection(activeSsid, activePassword, configuredApiEndpoint, false);
  } else if (
    strlen(defaultSsid) > 0 &&
    String(defaultSsid) != "YourWiFiSSID"
  ) {
    startWifiConnection(defaultSsid, defaultPassword, defaultApiEndpoint, false);
  } else {
    Serial.println("No WiFi credentials configured; BLE provisioning is ready");
    notifyReady();
  }
}

void WiFiProvisioningManager::initializeHotspot() {
  const IPAddress hotspotIp(192, 168, 4, 1);
  const IPAddress hotspotGateway(192, 168, 4, 1);
  const IPAddress hotspotSubnet(255, 255, 255, 0);

  if (!WiFi.softAPConfig(hotspotIp, hotspotGateway, hotspotSubnet)) {
    Serial.println("Failed to configure DrainGuard hotspot network");
  }

  hotspotActive = WiFi.softAP(
    DRAINGUARD_AP_SSID,
    DRAINGUARD_AP_PASSWORD,
    DRAINGUARD_AP_CHANNEL,
    false,
    DRAINGUARD_AP_MAX_CLIENTS
  );

  if (hotspotActive) {
    Serial.printf(
      "DrainGuard hotspot ready: %s at http://%s\n",
      DRAINGUARD_AP_SSID,
      WiFi.softAPIP().toString().c_str()
    );
    Serial.printf("ESP32-CAM expected at http://%s:%d\n", CAMERA_IP_ADDRESS, CAMERA_HTTP_PORT);
  } else {
    Serial.println("Failed to start DrainGuard hotspot");
  }
}

void WiFiProvisioningManager::initializeBle() {
  BLEDevice::init(deviceName.c_str());
  
  // Set MTU (optional, can fail on some devices)
  esp_err_t mtuResult = BLEDevice::setMTU(185);
  if (mtuResult != ESP_OK) {
    Serial.printf("Unable to set BLE MTU (error %d); using default\n", mtuResult);
  }

  // Controller commands and WiFi credentials share this service, so require
  // an encrypted, bonded link. With no display or keypad the ESP32 uses the
  // Bluetooth "Just Works" association model.
  BLEDevice::setSecurityCallbacks(new ProvisioningSecurityCallbacks());
  BLESecurity *security = new BLESecurity();
  security->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
  security->setCapability(ESP_IO_CAP_NONE);
  security->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
  security->setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
  setBleEncryptionLevel<BLEDevice, BLESecurity>(ESP_BLE_SEC_ENCRYPT, 0);

  bleServer = BLEDevice::createServer();
  bleServer->setCallbacks(new ProvisioningServerCallbacks(this));

  BLEService *service = bleServer->createService(DRAINGUARD_PROVISIONING_SERVICE_UUID);

  // Status characteristic (read/notify) over the encrypted link.
  statusCharacteristic = service->createCharacteristic(
    DRAINGUARD_PROVISIONING_TX_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  statusCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED);
  statusCharacteristic->addDescriptor(new BLE2902());

  // Provisioning and controller commands require an encrypted write.
  BLECharacteristic *commandCharacteristic = service->createCharacteristic(
    DRAINGUARD_PROVISIONING_RX_UUID,
    BLECharacteristic::PROPERTY_WRITE
  );
  commandCharacteristic->setAccessPermissions(ESP_GATT_PERM_WRITE_ENCRYPTED);
  commandCharacteristic->setCallbacks(new ProvisioningWriteCallbacks(this));

  service->start();

  // Configure advertising for maximum compatibility
  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(DRAINGUARD_PROVISIONING_SERVICE_UUID);
  advertising->setScanResponse(true);
  advertising->setMinPreferred(0x06);
  advertising->setMaxPreferred(0x12);
  
  BLEDevice::startAdvertising();

  notifyReady();
  Serial.printf("Secure BLE controller active as %s\n", deviceName.c_str());
}

void WiFiProvisioningManager::loadStoredConfiguration() {
  if (!preferences.begin(PREFERENCES_NAMESPACE, true)) {
    Serial.println("Failed to open provisioning NVS namespace");
    return;
  }

  hasStoredConfiguration = preferences.getBool("configured", false);
  if (hasStoredConfiguration) {
    activeSsid = preferences.getString("ssid", "");
    activePassword = preferences.getString("password", "");
    configuredApiEndpoint = preferences.getString("api_endpoint", configuredApiEndpoint);
    hasStoredConfiguration = activeSsid.length() > 0;
  }
  preferences.end();

  if (hasStoredConfiguration) {
    Serial.printf("Loaded WiFi configuration for SSID: %s\n", activeSsid.c_str());
  }
}

bool WiFiProvisioningManager::saveConfiguration() {
  if (!preferences.begin(PREFERENCES_NAMESPACE, false)) {
    return false;
  }

  // putString() returns zero for a valid empty value. Do not treat an empty
  // password as a write failure because open WiFi networks are supported.
  bool savedSsid = preferences.putString("ssid", activeSsid) == activeSsid.length();
  preferences.putString("password", activePassword);
  bool savedEndpoint =
    preferences.putString("api_endpoint", activeApiEndpoint) == activeApiEndpoint.length();
  bool savedFlag = preferences.putBool("configured", true) > 0;
  bool saved = savedSsid && savedEndpoint && savedFlag;

  preferences.end();
  hasStoredConfiguration = saved;
  return saved;
}

bool WiFiProvisioningManager::clearStoredConfiguration() {
  if (!preferences.begin(PREFERENCES_NAMESPACE, false)) {
    return false;
  }

  bool cleared = preferences.clear();
  preferences.end();
  return cleared;
}

void WiFiProvisioningManager::update() {
  ProvisioningCommand command;
  if (commandQueue != nullptr && xQueueReceive(commandQueue, &command, 0) == pdTRUE) {
    processCommand(command);
  }

  updateWifiConnection();
  updateWifiScan();
  restartAdvertisingIfNeeded();
}

void WiFiProvisioningManager::onBleConnected() {
  bleClientConnected = true;
  advertisingRestartPending = false;
  receiveBuffer = "";
  Serial.println("BLE provisioning client connected");
  // Keep the prepared status value available for the app's first encrypted
  // read. Notifying an encrypted characteristic here races Android's explicit
  // bonding request and can make otherwise valid connections fail during
  // security setup.
}

void WiFiProvisioningManager::onBleDisconnected() {
  bleClientConnected = false;
  receiveBuffer = "";
  bleDisconnectedAt = millis();
  advertisingRestartPending = true;
  Serial.println("BLE provisioning client disconnected");
}

void WiFiProvisioningManager::restartAdvertisingIfNeeded() {
  if (
    advertisingRestartPending &&
    millis() - bleDisconnectedAt >= 300 &&
    (BLE_PROVISIONING_STAY_ACTIVE || WiFi.status() != WL_CONNECTED)
  ) {
    bleServer->startAdvertising();
    advertisingRestartPending = false;
    Serial.println("BLE provisioning advertising restarted");
  }
}

void WiFiProvisioningManager::onBleWrite(const String &chunk) {
  if (chunk == "RESET" || chunk == "RESET\n") {
    receiveBuffer = "";
    return;
  }

  receiveBuffer += chunk;
  if (receiveBuffer.length() > MAX_PROVISIONING_PAYLOAD) {
    receiveBuffer = "";
    notifyStatus("invalid", "Provisioning payload is too large");
    return;
  }

  int delimiterIndex = receiveBuffer.indexOf('\n');
  while (delimiterIndex >= 0) {
    String payload = receiveBuffer.substring(0, delimiterIndex);
    receiveBuffer.remove(0, delimiterIndex + 1);
    payload.trim();

    if (payload.length() > 0) {
      enqueuePayload(payload);
    }
    delimiterIndex = receiveBuffer.indexOf('\n');
  }
}

void WiFiProvisioningManager::enqueuePayload(const String &payload) {
  StaticJsonDocument<768> document;
  DeserializationError error = deserializeJson(document, payload);
  if (error) {
    notifyStatus("invalid", "Malformed provisioning JSON");
    return;
  }

  const char *commandName = document["command"] | "";
  ProvisioningCommand command = {};

  if (
    strcmp(commandName, "get_status") == 0 ||
    strcmp(commandName, "arm") == 0 ||
    strcmp(commandName, "servo") == 0
  ) {
    ControllerCommand controllerCommand = {};
    controllerCommand.requestId = document["id"] | 0;
    if (controllerCommand.requestId == 0) {
      notifyStatus("invalid", "Controller command is missing its request id");
      return;
    }

    if (strcmp(commandName, "get_status") == 0) {
      controllerCommand.type = CONTROLLER_GET_STATUS;
    } else if (strcmp(commandName, "arm") == 0) {
      const char *action = document["action"] | "";
      if (strcmp(action, "open") == 0) {
        controllerCommand.type = CONTROLLER_ARM_OPEN;
      } else if (strcmp(action, "close") == 0) {
        controllerCommand.type = CONTROLLER_ARM_CLOSE;
      } else {
        notifyControllerResult(controllerCommand.requestId, false, "Unsupported arm action");
        return;
      }
    } else {
      const char *servoName = document["servo"] | "";
      if (!document["position"].is<int>()) {
        notifyControllerResult(controllerCommand.requestId, false, "Servo position is required");
        return;
      }

      int position = document["position"].as<int>();
      if (position < 0 || position > 4095) {
        notifyControllerResult(controllerCommand.requestId, false, "Servo position is invalid");
        return;
      }

      controllerCommand.type = CONTROLLER_SERVO;
      controllerCommand.position = static_cast<uint16_t>(position);
      if (strcmp(servoName, "base") == 0) {
        controllerCommand.servo = CONTROLLER_SERVO_BASE;
      } else if (strcmp(servoName, "shoulder") == 0) {
        controllerCommand.servo = CONTROLLER_SERVO_SHOULDER;
      } else if (strcmp(servoName, "elbow") == 0) {
        controllerCommand.servo = CONTROLLER_SERVO_ELBOW;
      } else if (strcmp(servoName, "gripper") == 0) {
        controllerCommand.servo = CONTROLLER_SERVO_GRIPPER;
      } else {
        notifyControllerResult(controllerCommand.requestId, false, "Unsupported servo");
        return;
      }
    }

    if (
      controllerCommandQueue == nullptr ||
      xQueueSend(controllerCommandQueue, &controllerCommand, 0) != pdTRUE
    ) {
      notifyControllerResult(controllerCommand.requestId, false, "Controller command queue is busy");
    }
    return;
  }

  if (strcmp(commandName, "scan_wifi") == 0) {
    command.type = COMMAND_SCAN_WIFI;
  } else if (strcmp(commandName, "forget_wifi") == 0) {
    command.type = COMMAND_FORGET_WIFI;
  } else if (strcmp(commandName, "set_wifi") == 0) {
    const char *ssid = document["ssid"] | "";
    const char *password = document["password"] | "";
    const char *receivedApiEndpoint = document["api_endpoint"] | "";
    String apiEndpoint = strlen(receivedApiEndpoint) > 0
      ? String(receivedApiEndpoint)
      : configuredApiEndpoint;

    size_t ssidLength = strlen(ssid);
    size_t passwordLength = strlen(password);
    size_t endpointLength = apiEndpoint.length();
    if (
      ssidLength == 0 || ssidLength > 32 ||
      passwordLength > 63 ||
      endpointLength == 0 || endpointLength > 192
    ) {
      notifyStatus("invalid", "SSID, password, or API endpoint has an invalid length");
      return;
    }
    if (!apiEndpoint.startsWith("http://") && !apiEndpoint.startsWith("https://")) {
      notifyStatus("invalid", "API endpoint must use HTTP or HTTPS");
      return;
    }

    command.type = COMMAND_SET_WIFI;
    strlcpy(command.ssid, ssid, sizeof(command.ssid));
    strlcpy(command.password, password, sizeof(command.password));
    strlcpy(command.apiEndpoint, apiEndpoint.c_str(), sizeof(command.apiEndpoint));
  } else {
    notifyStatus("invalid", "Unsupported provisioning command");
    return;
  }

  if (commandQueue == nullptr || xQueueSend(commandQueue, &command, 0) != pdTRUE) {
    notifyStatus("busy", "Provisioning command queue is busy");
  }
}

void WiFiProvisioningManager::processCommand(const ProvisioningCommand &command) {
  if (command.type == COMMAND_SCAN_WIFI) {
    startWifiScan();
    return;
  }

  if (command.type == COMMAND_FORGET_WIFI) {
    forgetWifi();
    return;
  }

  startWifiConnection(
    String(command.ssid),
    String(command.password),
    String(command.apiEndpoint),
    true
  );
}

void WiFiProvisioningManager::startWifiConnection(
  const String &ssid,
  const String &password,
  const String &apiEndpoint,
  bool persistOnSuccess
) {
  if (wifiScanInProgress) {
    WiFi.scanDelete();
    wifiScanInProgress = false;
  }

  activeSsid = ssid;
  activePassword = password;
  activeApiEndpoint = apiEndpoint.length() > 0 ? apiEndpoint : configuredApiEndpoint;
  persistPendingCredentials = persistOnSuccess;
  connectionState = WIFI_CONNECTING;
  connectionStartedAt = millis();

  WiFi.setAutoReconnect(true);
  WiFi.disconnect(false, false);
  // AP+STA keeps the private robot hotspot online while optionally connecting
  // the station interface to a provisioned internet network.
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(activeSsid.c_str(), activePassword.c_str());

  StaticJsonDocument<160> document;
  document["status"] = "connecting";
  document["ssid"] = activeSsid;
  String payload;
  serializeJson(document, payload);
  notifyPayload(payload);

  Serial.printf("Connecting to provisioned WiFi SSID: %s\n", activeSsid.c_str());
}

void WiFiProvisioningManager::updateWifiConnection() {
  if (connectionState != WIFI_CONNECTING) return;

  if (WiFi.status() == WL_CONNECTED) {
    connectionState = WIFI_CONNECTED;
    configuredApiEndpoint = activeApiEndpoint;
    WiFi.setAutoReconnect(true);

    bool saved = true;
    if (persistPendingCredentials) {
      saved = saveConfiguration();
    }
    persistPendingCredentials = false;
    activePassword = "";

    StaticJsonDocument<320> document;
    document["status"] = "connected";
    document["ssid"] = activeSsid;
    document["ip"] = WiFi.localIP().toString();
    document["hotspot_ip"] = WiFi.softAPIP().toString();
    document["saved"] = saved;
    String payload;
    serializeJson(document, payload);
    notifyPayload(payload);

    Serial.printf("WiFi connected. IP address: %s\n", WiFi.localIP().toString().c_str());
    if (!BLE_PROVISIONING_STAY_ACTIVE) {
      BLEDevice::getAdvertising()->stop();
    }
    return;
  }

  if (millis() - connectionStartedAt >= WIFI_PROVISIONING_TIMEOUT_MS) {
    wl_status_t finalStatus = WiFi.status();
    connectionState = WIFI_FAILED;
    persistPendingCredentials = false;
    activePassword = "";
    WiFi.disconnect(false, false);

    StaticJsonDocument<192> document;
    document["status"] = "failed";
    document["reason"] = wifiFailureReason(finalStatus);
    document["message"] = "Unable to join WiFi within 30 seconds";
    String payload;
    serializeJson(document, payload);
    notifyPayload(payload);

    Serial.printf("WiFi connection failed: %s\n", wifiFailureReason(finalStatus));
  }
}

void WiFiProvisioningManager::forgetWifi() {
  if (wifiScanInProgress) {
    WiFi.scanDelete();
    wifiScanInProgress = false;
  }

  connectionState = WIFI_IDLE;
  persistPendingCredentials = false;
  WiFi.setAutoReconnect(false);
  // Disconnect only the station interface and erase the Arduino WiFi stack's
  // remembered AP. WIFI_AP_STA keeps the private robot hotspot online.
  WiFi.disconnect(false, true);
  WiFi.mode(WIFI_AP_STA);

  bool cleared = clearStoredConfiguration();
  hasStoredConfiguration = false;
  activeSsid = "";
  activePassword = "";
  activeApiEndpoint = "";

  if (!cleared) {
    notifyStatus("clear_failed", "Unable to clear saved WiFi credentials");
    Serial.println("Failed to clear saved WiFi credentials");
    return;
  }

  StaticJsonDocument<192> document;
  document["status"] = "forgotten";
  document["configured"] = false;
  document["hotspot_ip"] = WiFi.softAPIP().toString();
  String payload;
  serializeJson(document, payload);
  notifyPayload(payload);

  Serial.println("Saved WiFi credentials cleared; private hotspot remains active");
}

void WiFiProvisioningManager::startWifiScan() {
  if (wifiScanInProgress) {
    notifyStatus("scanning_wifi", "WiFi scan already in progress");
    return;
  }

  // A provisioning scan intentionally interrupts a pending connection attempt
  // so users can recover from invalid stored credentials immediately.
  if (connectionState == WIFI_CONNECTING) {
    WiFi.disconnect(false, false);
    connectionState = WIFI_IDLE;
    persistPendingCredentials = false;
  }

  WiFi.mode(WIFI_AP_STA);
  int result = WiFi.scanNetworks(true, false);
  if (result == WIFI_SCAN_FAILED) {
    notifyStatus("scan_failed", "Unable to start WiFi scan");
    return;
  }

  wifiScanInProgress = true;
  notifyStatus("scanning_wifi", "Scanning nearby 2.4 GHz networks");
}

void WiFiProvisioningManager::updateWifiScan() {
  if (!wifiScanInProgress) return;

  int networkCount = WiFi.scanComplete();
  if (networkCount == WIFI_SCAN_RUNNING) return;

  if (networkCount < 0) {
    wifiScanInProgress = false;
    notifyStatus("scan_failed", "WiFi scan failed");
    return;
  }

  for (int index = 0; index < networkCount; index++) {
    if (WiFi.SSID(index).length() == 0) continue;

    StaticJsonDocument<192> document;
    document["status"] = "network";
    document["ssid"] = WiFi.SSID(index);
    document["rssi"] = WiFi.RSSI(index);
    document["secure"] = WiFi.encryptionType(index) != WIFI_AUTH_OPEN;
    String payload;
    serializeJson(document, payload);
    notifyPayload(payload);
  }

  WiFi.scanDelete();
  wifiScanInProgress = false;

  StaticJsonDocument<96> document;
  document["status"] = "scan_complete";
  document["count"] = networkCount;
  String payload;
  serializeJson(document, payload);
  notifyPayload(payload);
}

bool WiFiProvisioningManager::nextControllerCommand(ControllerCommand &command) {
  return
    controllerCommandQueue != nullptr &&
    xQueueReceive(controllerCommandQueue, &command, 0) == pdTRUE;
}

void WiFiProvisioningManager::notifyControllerStatus(
  uint32_t requestId,
  float waterLevel,
  float distance,
  bool drainOpen,
  float latitude,
  float longitude,
  int satellites
) {
  StaticJsonDocument<256> document;
  document["status"] = "controller_status";
  document["id"] = requestId;
  document["wl"] = waterLevel;
  document["d"] = distance;
  document["o"] = drainOpen;
  document["lat"] = latitude;
  document["lon"] = longitude;
  document["sat"] = satellites;

  String payload;
  serializeJson(document, payload);
  notifyPayload(payload);
}

void WiFiProvisioningManager::notifyControllerResult(
  uint32_t requestId,
  bool ok,
  const String &message
) {
  StaticJsonDocument<160> document;
  document["status"] = "command_result";
  document["id"] = requestId;
  document["ok"] = ok;
  if (message.length() > 0) document["message"] = message;

  String payload;
  serializeJson(document, payload);
  notifyPayload(payload);
}

void WiFiProvisioningManager::notifyPayload(const String &payload) {
  if (statusCharacteristic == nullptr) return;

  if (!bleClientConnected) {
    statusCharacteristic->setValue(payload.c_str());
    return;
  }

  String framedPayload = payload + "\n";
  // The server API exposes the configured local MTU, not reliably the peer's
  // negotiated MTU on every Arduino-ESP32 release. Twenty-byte notifications
  // therefore keep responses compatible even when an Android stack leaves the
  // connection at the default 23-byte ATT MTU.
  constexpr size_t chunkSize = 20;

  for (size_t offset = 0; offset < framedPayload.length(); offset += chunkSize) {
    String chunk = framedPayload.substring(
      offset,
      min(offset + chunkSize, framedPayload.length())
    );
    statusCharacteristic->setValue(chunk.c_str());
    statusCharacteristic->notify();
    delay(15);
  }
}

void WiFiProvisioningManager::notifyReady() {
  if (statusCharacteristic == nullptr) return;

  StaticJsonDocument<384> document;
  document["status"] = WiFi.status() == WL_CONNECTED ? "connected" : "ready";
  document["device"] = deviceName;
  document["configured"] = hasStoredConfiguration;
  document["hotspot_ip"] = WiFi.softAPIP().toString();
  if (WiFi.status() == WL_CONNECTED) {
    document["ssid"] = WiFi.SSID();
    document["ip"] = WiFi.localIP().toString();
  }

  String payload;
  serializeJson(document, payload);
  notifyPayload(payload);
}

void WiFiProvisioningManager::notifyStatus(const String &status, const String &message) {
  if (statusCharacteristic == nullptr) return;

  StaticJsonDocument<192> document;
  document["status"] = status;
  if (message.length() > 0) document["message"] = message;
  String payload;
  serializeJson(document, payload);
  notifyPayload(payload);
}

const char *WiFiProvisioningManager::wifiFailureReason(wl_status_t status) const {
  switch (status) {
    case WL_NO_SSID_AVAIL:
      return "network_not_found";
    case WL_CONNECT_FAILED:
      return "authentication_failed";
    case WL_CONNECTION_LOST:
      return "connection_lost";
    case WL_DISCONNECTED:
      return "disconnected";
    default:
      return "timeout";
  }
}

bool WiFiProvisioningManager::isWifiConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

bool WiFiProvisioningManager::isHotspotActive() const {
  return hotspotActive;
}

String WiFiProvisioningManager::getApiEndpoint() const {
  return configuredApiEndpoint;
}

String WiFiProvisioningManager::getDeviceName() const {
  return deviceName;
}

String WiFiProvisioningManager::getHotspotIP() const {
  return WiFi.softAPIP().toString();
}
