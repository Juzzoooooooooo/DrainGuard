#ifndef WIFI_PROVISIONING_H
#define WIFI_PROVISIONING_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <Preferences.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// Custom DrainGuard provisioning service. Keep these UUIDs synchronized with
// mobile-app/src/services/bleProvisioning.js.
#define DRAINGUARD_PROVISIONING_SERVICE_UUID "7b0d1001-5f6b-4c4f-9a7e-2f3b4d5e6f70"
#define DRAINGUARD_PROVISIONING_RX_UUID      "7b0d1002-5f6b-4c4f-9a7e-2f3b4d5e6f70"
#define DRAINGUARD_PROVISIONING_TX_UUID      "7b0d1003-5f6b-4c4f-9a7e-2f3b4d5e6f70"

class WiFiProvisioningManager {
public:
  WiFiProvisioningManager();

  void begin(const char *defaultSsid, const char *defaultPassword, const char *defaultApiEndpoint);
  void update();

  bool isWifiConnected() const;
  bool isHotspotActive() const;
  String getApiEndpoint() const;
  String getDeviceName() const;
  String getHotspotIP() const;

  // Called by BLE callbacks. They are public so the small adapter callback
  // classes do not need access to the rest of the manager's internal state.
  void onBleConnected();
  void onBleDisconnected();
  void onBleWrite(const String &chunk);

private:
  enum CommandType : uint8_t {
    COMMAND_SET_WIFI,
    COMMAND_SCAN_WIFI,
  };

  enum ConnectionState : uint8_t {
    WIFI_IDLE,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    WIFI_FAILED,
  };

  struct ProvisioningCommand {
    CommandType type;
    char ssid[33];
    char password[65];
    char apiEndpoint[193];
  };

  void initializeBle();
  void initializeHotspot();
  void loadStoredConfiguration();
  bool saveConfiguration();
  void enqueuePayload(const String &payload);
  void processCommand(const ProvisioningCommand &command);
  void startWifiConnection(
    const String &ssid,
    const String &password,
    const String &apiEndpoint,
    bool persistOnSuccess
  );
  void updateWifiConnection();
  void startWifiScan();
  void updateWifiScan();
  void restartAdvertisingIfNeeded();
  void notifyReady();
  void notifyStatus(const String &status, const String &message = "");
  const char *wifiFailureReason(wl_status_t status) const;

  Preferences preferences;
  QueueHandle_t commandQueue;
  BLEServer *bleServer;
  BLECharacteristic *statusCharacteristic;

  String deviceName;
  String receiveBuffer;
  String activeSsid;
  String activePassword;
  String activeApiEndpoint;
  String configuredApiEndpoint;

  ConnectionState connectionState;
  unsigned long connectionStartedAt;
  unsigned long bleDisconnectedAt;
  bool persistPendingCredentials;
  bool hasStoredConfiguration;
  bool bleClientConnected;
  bool advertisingRestartPending;
  bool wifiScanInProgress;
  bool hotspotActive;
};

#endif
