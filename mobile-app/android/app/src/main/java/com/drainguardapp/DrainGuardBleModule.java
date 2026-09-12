package com.drainguardapp;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.BluetoothStatusCodes;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanRecord;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.ParcelUuid;
import android.provider.Settings;

import com.facebook.react.bridge.Arguments;
import com.facebook.react.bridge.Promise;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.ReactContextBaseJavaModule;
import com.facebook.react.bridge.ReactMethod;
import com.facebook.react.bridge.WritableMap;
import com.facebook.react.modules.core.DeviceEventManagerModule;

import org.json.JSONException;
import org.json.JSONObject;

import java.nio.charset.StandardCharsets;
import java.util.ArrayDeque;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.UUID;

public class DrainGuardBleModule extends ReactContextBaseJavaModule {
  private static final UUID SERVICE_UUID =
      UUID.fromString("7b0d1001-5f6b-4c4f-9a7e-2f3b4d5e6f70");
  private static final UUID RX_UUID =
      UUID.fromString("7b0d1002-5f6b-4c4f-9a7e-2f3b4d5e6f70");
  private static final UUID TX_UUID =
      UUID.fromString("7b0d1003-5f6b-4c4f-9a7e-2f3b4d5e6f70");
  private static final UUID CLIENT_CONFIGURATION_UUID =
      UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");
  private static final int SAFE_CHUNK_SIZE = 182;  // MTU 185 minus 3 bytes ATT overhead
  private static final int REQUESTED_MTU = 185;
  private static final long SCAN_DURATION_MS = 12000;
  private static final long CONNECTION_TIMEOUT_MS = 45000;
  private static final long SERVICE_DISCOVERY_FALLBACK_MS = 1500;
  private static final long RECONNECT_DELAY_MS = 1000;
  private static final long WRITE_RETRY_DELAY_MS = 200;
  private static final int MAX_CONNECTION_ATTEMPTS = 3;
  private static final int MAX_WRITE_START_ATTEMPTS = 8;

  private final ReactApplicationContext reactContext;
  private final BluetoothAdapter bluetoothAdapter;
  private final Handler handler = new Handler(Looper.getMainLooper());
  private final Runnable scanTimeout = this::stopScanInternal;
  private final Set<String> discoveredDevices = new HashSet<>();
  private final ArrayDeque<byte[]> writeQueue = new ArrayDeque<>();

  private BluetoothLeScanner scanner;
  private BluetoothGatt bluetoothGatt;
  private BluetoothGattCharacteristic commandCharacteristic;
  private BluetoothGattCharacteristic statusCharacteristic;
  private BluetoothDevice connectingDevice;
  private Promise connectionPromise;
  private Promise writePromise;
  private int connectionAttempt;
  private int writeStartAttempt;
  private boolean scanning;
  private boolean ready;
  private boolean serviceDiscoveryStarted;
  private boolean bondingRequested;
  private boolean notificationSetupStarted;
  private final Runnable serviceDiscoveryFallback =
      () -> {
        if (bluetoothGatt != null) discoverServices(bluetoothGatt);
      };
  private final Runnable reconnectGatt =
      () -> {
        if (connectionPromise != null && connectingDevice != null && bluetoothGatt == null) {
          startGattConnection(connectingDevice);
        }
      };
  private final Runnable writeRetry = this::writeNextChunk;
  private final Runnable connectionTimeout =
      () -> {
        if (connectionPromise != null) {
          rejectConnection("CONNECT_TIMEOUT", "Bluetooth connection or pairing timed out.");
          closeGatt();
        }
      };

  DrainGuardBleModule(ReactApplicationContext context) {
    super(context);
    reactContext = context;
    BluetoothManager manager =
        (BluetoothManager) context.getSystemService(Context.BLUETOOTH_SERVICE);
    bluetoothAdapter = manager == null ? null : manager.getAdapter();

    IntentFilter bondFilter = new IntentFilter(BluetoothDevice.ACTION_BOND_STATE_CHANGED);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
      context.registerReceiver(bondReceiver, bondFilter, Context.RECEIVER_EXPORTED);
    } else {
      context.registerReceiver(bondReceiver, bondFilter);
    }
  }

  @Override
  public String getName() {
    return "DrainGuardBle";
  }

  @ReactMethod
  public void addListener(String eventName) {
    // Required by React Native's NativeEventEmitter contract.
  }

  @ReactMethod
  public void removeListeners(double count) {
    // Required by React Native's NativeEventEmitter contract.
  }

  @ReactMethod
  public void isSupported(Promise promise) {
    promise.resolve(bluetoothAdapter != null);
  }

  @SuppressLint("MissingPermission")
  @ReactMethod
  public void startScan(Promise promise) {
    if (bluetoothAdapter == null) {
      promise.reject("BLE_UNAVAILABLE", "Bluetooth Low Energy is unavailable on this phone.");
      return;
    }
    if (!bluetoothAdapter.isEnabled()) {
      promise.reject("BLUETOOTH_DISABLED", "Turn on Bluetooth, then scan again.");
      return;
    }

    stopScanInternal();
    scanner = bluetoothAdapter.getBluetoothLeScanner();
    if (scanner == null) {
      promise.reject("SCAN_UNAVAILABLE", "Bluetooth scanning is unavailable.");
      return;
    }

    discoveredDevices.clear();
    ScanSettings settings =
        new ScanSettings.Builder()
            .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
            .build();
    scanning = true;
    scanner.startScan(null, settings, scanCallback);
    emitState("scanning", "Looking for DrainGuard controllers");
    handler.removeCallbacks(scanTimeout);
    handler.postDelayed(scanTimeout, SCAN_DURATION_MS);
    promise.resolve(null);
  }

  @SuppressLint("MissingPermission")
  @ReactMethod
  public void stopScan(Promise promise) {
    stopScanInternal();
    promise.resolve(null);
  }

  @SuppressLint("MissingPermission")
  @ReactMethod
  public void connect(String deviceId, Promise promise) {
    if (bluetoothAdapter == null || !bluetoothAdapter.isEnabled()) {
      promise.reject("BLUETOOTH_DISABLED", "Turn on Bluetooth before connecting.");
      return;
    }
    if (connectionPromise != null) {
      promise.reject("CONNECT_BUSY", "A Bluetooth connection is already in progress.");
      return;
    }

    stopScanInternal();
    closeGatt();
    BluetoothDevice device;
    try {
      device = bluetoothAdapter.getRemoteDevice(deviceId);
    } catch (IllegalArgumentException error) {
      promise.reject("INVALID_DEVICE", "The selected DrainGuard device is invalid.", error);
      return;
    }

    connectionPromise = promise;
    connectingDevice = device;
    connectionAttempt = 0;
    handler.removeCallbacks(connectionTimeout);
    handler.postDelayed(connectionTimeout, CONNECTION_TIMEOUT_MS);
    startGattConnection(device);
  }

  @ReactMethod
  public void disconnect(Promise promise) {
    stopScanInternal();
    rejectConnection("DISCONNECTED", "DrainGuard Bluetooth disconnected.");
    closeGatt();
    emitState("disconnected", "Bluetooth disconnected");
    promise.resolve(null);
  }

  @ReactMethod
  public void openBluetoothSettings(Promise promise) {
    Activity activity = getCurrentActivity();
    if (activity == null) {
      promise.reject("NO_ACTIVITY", "Unable to open Bluetooth settings.");
      return;
    }
    activity.startActivity(new Intent(Settings.ACTION_BLUETOOTH_SETTINGS));
    promise.resolve(null);
  }

  @ReactMethod
  public void scanWifi(Promise promise) {
    try {
      JSONObject payload = new JSONObject();
      payload.put("command", "scan_wifi");
      sendPayload(payload.toString(), promise);
    } catch (JSONException error) {
      promise.reject("COMMAND_ERROR", "Unable to create the Wi-Fi scan command.", error);
    }
  }

  @ReactMethod
  public void provisionWifi(
      String ssid,
      String password,
      String apiEndpoint,
      Promise promise) {
    if (ssid == null || ssid.trim().isEmpty() || ssid.length() > 32) {
      promise.reject("INVALID_SSID", "Wi-Fi name must contain 1 to 32 characters.");
      return;
    }
    if (password != null && password.length() > 63) {
      promise.reject("INVALID_PASSWORD", "Wi-Fi password cannot exceed 63 characters.");
      return;
    }

    try {
      JSONObject payload = new JSONObject();
      payload.put("command", "set_wifi");
      payload.put("ssid", ssid.trim());
      payload.put("password", password == null ? "" : password);
      payload.put("api_endpoint", apiEndpoint == null ? "" : apiEndpoint.trim());
      sendPayload(payload.toString(), promise);
    } catch (JSONException error) {
      promise.reject("COMMAND_ERROR", "Unable to create the Wi-Fi setup command.", error);
    }
  }

  @SuppressLint("MissingPermission")
  private void stopScanInternal() {
    handler.removeCallbacks(scanTimeout);
    if (scanning && scanner != null) {
      try {
        scanner.stopScan(scanCallback);
      } catch (RuntimeException ignored) {
        // Android can revoke Bluetooth access while a scan is ending.
      }
    }
    scanning = false;
  }

  @SuppressLint("MissingPermission")
  private String safeDeviceName(BluetoothDevice device) {
    String name = device.getName();
    return name == null || name.trim().isEmpty() ? "DrainGuard" : name;
  }

  private boolean advertisesDrainGuard(ScanResult result) {
    ScanRecord record = result.getScanRecord();
    if (record == null) return false;

    String advertisedName = record.getDeviceName();
    if (advertisedName != null && advertisedName.startsWith("DrainGuard-")) {
      return true;
    }

    List<ParcelUuid> serviceUuids = record.getServiceUuids();
    if (serviceUuids == null) return false;
    for (ParcelUuid uuid : serviceUuids) {
      if (SERVICE_UUID.equals(uuid.getUuid())) return true;
    }
    return false;
  }

  @SuppressLint("MissingPermission")
  private void emitDevice(ScanResult result) {
    BluetoothDevice device = result.getDevice();
    if (!discoveredDevices.add(device.getAddress())) return;

    ScanRecord record = result.getScanRecord();
    String advertisedName = record == null ? null : record.getDeviceName();
    WritableMap event = Arguments.createMap();
    event.putString("id", device.getAddress());
    event.putString(
        "name",
        advertisedName == null || advertisedName.trim().isEmpty()
            ? safeDeviceName(device)
            : advertisedName);
    event.putInt("rssi", result.getRssi());
    emit("DrainGuardBleDevice", event);
  }

  private final ScanCallback scanCallback =
      new ScanCallback() {
        @Override
        public void onScanResult(int callbackType, ScanResult result) {
          if (advertisesDrainGuard(result)) emitDevice(result);
        }

        @Override
        public void onScanFailed(int errorCode) {
          scanning = false;
          WritableMap event = Arguments.createMap();
          event.putString("state", "error");
          event.putString("message", "Bluetooth scan failed (code " + errorCode + ").");
          emit("DrainGuardBleState", event);
        }
      };

  @SuppressLint("MissingPermission")
  private final BroadcastReceiver bondReceiver =
      new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
          if (!BluetoothDevice.ACTION_BOND_STATE_CHANGED.equals(intent.getAction())) return;
          BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
          if (device == null || bluetoothGatt == null) return;
          if (!device.getAddress().equals(bluetoothGatt.getDevice().getAddress())) return;

          int state = intent.getIntExtra(BluetoothDevice.EXTRA_BOND_STATE, BluetoothDevice.ERROR);
          if (state == BluetoothDevice.BOND_BONDED) {
            bondingRequested = false;
            // Pairing can finish either before or after service discovery. Resume
            // from the correct stage instead of depending on callback ordering,
            // which differs between Android Bluetooth stacks.
            if (statusCharacteristic == null) {
              prepareGatt(bluetoothGatt);
            } else {
              notificationSetupStarted = false;
              enableStatusNotifications(bluetoothGatt);
            }
          } else if (state == BluetoothDevice.BOND_BONDING) {
            bondingRequested = true;
          } else if (state == BluetoothDevice.BOND_NONE && bondingRequested) {
            rejectConnection(
                "PAIRING_FAILED",
                "Secure Bluetooth pairing failed. Forget DrainGuard in Android Bluetooth settings, then retry.");
            closeGatt();
          }
        }
      };

  @SuppressLint("MissingPermission")
  private final BluetoothGattCallback gattCallback =
      new BluetoothGattCallback() {
        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
          if (gatt != bluetoothGatt) {
            gatt.close();
            return;
          }
          if (status != BluetoothGatt.GATT_SUCCESS || newState == BluetoothProfile.STATE_DISCONNECTED) {
            ready = false;
            if (connectionPromise != null && connectionAttempt < MAX_CONNECTION_ATTEMPTS) {
              closeGattSession(gatt);
              emitState("reconnecting", "Bluetooth was interrupted; retrying connection");
              handler.removeCallbacks(reconnectGatt);
              handler.postDelayed(reconnectGatt, RECONNECT_DELAY_MS);
              return;
            }
            rejectConnection(
                "CONNECT_FAILED",
                "DrainGuard Bluetooth connection failed (GATT " + status + "). Move closer and retry.");
            emitState("disconnected", "DrainGuard disconnected");
            closeGatt();
            return;
          }

          if (newState == BluetoothProfile.STATE_CONNECTED) {
            // The current firmware exposes unencrypted provisioning
            // characteristics. Do not force an unnecessary Android bond here;
            // encrypted firmware is still supported by the authentication
            // recovery path used while enabling notifications and reading status.
            prepareGatt(gatt);
          }
        }

        @Override
        public void onMtuChanged(BluetoothGatt gatt, int mtu, int status) {
          if (gatt != bluetoothGatt) return;
          handler.removeCallbacks(serviceDiscoveryFallback);
          discoverServices(gatt);
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
          if (gatt != bluetoothGatt) return;
          if (status != BluetoothGatt.GATT_SUCCESS) {
            rejectConnection("SERVICE_ERROR", "Unable to read DrainGuard Bluetooth services.");
            closeGatt();
            return;
          }

          BluetoothGattService service = gatt.getService(SERVICE_UUID);
          if (service == null) {
            rejectConnection("SERVICE_MISSING", "This is not a compatible DrainGuard controller.");
            closeGatt();
            return;
          }
          commandCharacteristic = service.getCharacteristic(RX_UUID);
          statusCharacteristic = service.getCharacteristic(TX_UUID);
          if (commandCharacteristic == null || statusCharacteristic == null) {
            rejectConnection("SERVICE_INCOMPLETE", "DrainGuard provisioning service is incomplete.");
            closeGatt();
            return;
          }
          enableStatusNotifications(gatt);
        }

        @Override
        public void onDescriptorWrite(BluetoothGatt gatt, BluetoothGattDescriptor descriptor, int status) {
          if (gatt != bluetoothGatt) return;
          if (!CLIENT_CONFIGURATION_UUID.equals(descriptor.getUuid())) return;
          if (status != BluetoothGatt.GATT_SUCCESS) {
            if (!recoverOrRejectGattOperation(
                    status,
                    "NOTIFY_ERROR",
                    "Unable to enable DrainGuard status notifications")) {
              closeGatt();
            }
            return;
          }

          // The status value itself requires encryption. Reading it before
          // resolving connect verifies that Android and the ESP32 share valid
          // bond keys instead of reporting a false successful connection.
          if (!gatt.readCharacteristic(statusCharacteristic)) {
            rejectConnection("READ_ERROR", "Unable to verify the secure DrainGuard connection.");
            closeGatt();
          }
        }

        @Override
        public void onCharacteristicRead(
            BluetoothGatt gatt,
            BluetoothGattCharacteristic characteristic,
            int status) {
          handleStatusRead(gatt, characteristic, characteristic.getValue(), status);
        }

        @Override
        public void onCharacteristicRead(
            BluetoothGatt gatt,
            BluetoothGattCharacteristic characteristic,
            byte[] value,
            int status) {
          handleStatusRead(gatt, characteristic, value, status);
        }

        @Override
        public void onCharacteristicChanged(
            BluetoothGatt gatt,
            BluetoothGattCharacteristic characteristic) {
          if (TX_UUID.equals(characteristic.getUuid())) {
            emitStatus(characteristic.getValue());
          }
        }

        @Override
        public void onCharacteristicChanged(
            BluetoothGatt gatt,
            BluetoothGattCharacteristic characteristic,
            byte[] value) {
          if (TX_UUID.equals(characteristic.getUuid())) {
            emitStatus(value);
          }
        }

        @Override
        public void onCharacteristicWrite(
            BluetoothGatt gatt,
            BluetoothGattCharacteristic characteristic,
            int status) {
          if (!RX_UUID.equals(characteristic.getUuid())) return;
          if (status != BluetoothGatt.GATT_SUCCESS) {
            rejectWrite("WRITE_FAILED", "DrainGuard did not accept the setup command.");
            return;
          }
          // Start the next write after this callback returns. Some Android BLE
          // stacks keep the GATT client marked busy for the callback's lifetime.
          handler.post(writeRetry);
        }
      };

  @SuppressLint("MissingPermission")
  private void startGattConnection(BluetoothDevice device) {
    if (connectionPromise == null) return;

    connectionAttempt++;
    resetGattState();
    emitState(
        connectionAttempt == 1 ? "connecting" : "reconnecting",
        (connectionAttempt == 1 ? "Connecting to " : "Retrying ") + safeDeviceName(device));

    try {
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
        bluetoothGatt =
            device.connectGatt(
                reactContext,
                false,
                gattCallback,
                BluetoothDevice.TRANSPORT_LE,
                BluetoothDevice.PHY_LE_1M_MASK,
                handler);
      } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
        bluetoothGatt =
            device.connectGatt(
                reactContext, false, gattCallback, BluetoothDevice.TRANSPORT_LE);
      } else {
        bluetoothGatt = device.connectGatt(reactContext, false, gattCallback);
      }
    } catch (RuntimeException error) {
      rejectConnection("CONNECT_FAILED", "Android could not start the Bluetooth connection.");
      closeGatt();
      return;
    }

    if (bluetoothGatt == null) {
      rejectConnection("CONNECT_FAILED", "Android could not create a Bluetooth connection.");
      closeGatt();
    }
  }

  @SuppressLint("MissingPermission")
  private void prepareGatt(BluetoothGatt gatt) {
    if (gatt != bluetoothGatt || serviceDiscoveryStarted) return;
    emitState("discovering", "Preparing secure Wi-Fi setup");
    if (!gatt.requestMtu(REQUESTED_MTU)) {
      discoverServices(gatt);
      return;
    }
    // A few Android stacks accept requestMtu() but omit its callback. Keep
    // connection setup moving instead of waiting until the global timeout.
    handler.removeCallbacks(serviceDiscoveryFallback);
    handler.postDelayed(serviceDiscoveryFallback, SERVICE_DISCOVERY_FALLBACK_MS);
  }

  @SuppressLint("MissingPermission")
  private void discoverServices(BluetoothGatt gatt) {
    if (gatt != bluetoothGatt || serviceDiscoveryStarted) return;
    handler.removeCallbacks(serviceDiscoveryFallback);
    serviceDiscoveryStarted = true;
    if (!gatt.discoverServices()) {
      rejectConnection("SERVICE_ERROR", "Unable to start DrainGuard service discovery.");
      closeGatt();
    }
  }

  @SuppressLint("MissingPermission")
  private void enableStatusNotifications(BluetoothGatt gatt) {
    if (
        gatt != bluetoothGatt ||
        notificationSetupStarted ||
        statusCharacteristic == null
    ) {
      return;
    }
    notificationSetupStarted = true;
    emitState("discovering", "Pairing complete; enabling DrainGuard status");

    if (!gatt.setCharacteristicNotification(statusCharacteristic, true)) {
      rejectConnection("NOTIFY_ERROR", "Unable to monitor DrainGuard setup status.");
      closeGatt();
      return;
    }
    BluetoothGattDescriptor descriptor =
        statusCharacteristic.getDescriptor(CLIENT_CONFIGURATION_UUID);
    if (descriptor == null) {
      rejectConnection("NOTIFY_MISSING", "DrainGuard status notifications are unavailable.");
      closeGatt();
      return;
    }
    descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
    if (!gatt.writeDescriptor(descriptor)) {
      rejectConnection("NOTIFY_ERROR", "Unable to enable DrainGuard status notifications.");
      closeGatt();
    }
  }

  private void handleStatusRead(
      BluetoothGatt gatt,
      BluetoothGattCharacteristic characteristic,
      byte[] value,
      int status) {
    if (gatt != bluetoothGatt || !TX_UUID.equals(characteristic.getUuid())) return;
    if (status != BluetoothGatt.GATT_SUCCESS) {
      if (!recoverOrRejectGattOperation(
              status,
              "READ_ERROR",
              "Unable to verify the secure DrainGuard connection")) {
        closeGatt();
      }
      return;
    }

    emitStatus(value);
    completeConnection(gatt);
  }

  @SuppressLint("MissingPermission")
  private void completeConnection(BluetoothGatt gatt) {
    if (gatt != bluetoothGatt || connectionPromise == null) return;

    ready = true;
    bondingRequested = false;
    handler.removeCallbacks(connectionTimeout);
    handler.removeCallbacks(reconnectGatt);
    emitState("connected", "DrainGuard Bluetooth connected");

    WritableMap result = Arguments.createMap();
    result.putString("id", gatt.getDevice().getAddress());
    result.putString("name", safeDeviceName(gatt.getDevice()));
    connectionPromise.resolve(result);
    connectionPromise = null;
    connectingDevice = null;
    connectionAttempt = 0;
  }

  @SuppressLint("MissingPermission")
  private boolean recoverOrRejectGattOperation(int status, String code, String operation) {
    if (
        status == BluetoothGatt.GATT_INSUFFICIENT_AUTHENTICATION ||
        status == BluetoothGatt.GATT_INSUFFICIENT_ENCRYPTION
    ) {
      if (
          bluetoothGatt != null &&
          bluetoothGatt.getDevice().getBondState() != BluetoothDevice.BOND_BONDED
      ) {
        bondingRequested = true;
        notificationSetupStarted = false;
        emitState("pairing", "Completing secure pairing with DrainGuard");
        try {
          if (
              bluetoothGatt.getDevice().getBondState() == BluetoothDevice.BOND_BONDING ||
              bluetoothGatt.getDevice().createBond()
          ) {
            return true;
          }
        } catch (RuntimeException ignored) {
          // Report the actionable pairing error below.
        }
      }
      rejectConnection(
          "PAIRING_STALE",
          "The saved DrainGuard pairing is no longer valid. Forget DrainGuard in Android Bluetooth settings, then retry.");
      return false;
    }
    rejectConnection(code, operation + " (GATT " + status + ").");
    return false;
  }

  private void sendPayload(String payload, Promise promise) {
    if (!ready || bluetoothGatt == null || commandCharacteristic == null) {
      promise.reject("NOT_CONNECTED", "Connect to DrainGuard Bluetooth first.");
      return;
    }
    if (writePromise != null) {
      promise.reject("WRITE_BUSY", "Another DrainGuard command is still being sent.");
      return;
    }

    byte[] bytes = (payload + "\n").getBytes(StandardCharsets.UTF_8);
    writeQueue.clear();
    for (int offset = 0; offset < bytes.length; offset += SAFE_CHUNK_SIZE) {
      int length = Math.min(SAFE_CHUNK_SIZE, bytes.length - offset);
      byte[] chunk = new byte[length];
      System.arraycopy(bytes, offset, chunk, 0, length);
      writeQueue.add(chunk);
    }
    writePromise = promise;
    writeStartAttempt = 0;
    // Give Android a callback turn to release any GATT operation used to
    // finish connection setup before starting the first command write.
    handler.post(writeRetry);
  }

  @SuppressLint("MissingPermission")
  private void writeNextChunk() {
    if (writePromise == null) return;
    byte[] chunk = writeQueue.poll();
    if (chunk == null) {
      writePromise.resolve(null);
      writePromise = null;
      return;
    }

    BluetoothGatt gatt = bluetoothGatt;
    BluetoothGattCharacteristic characteristic = commandCharacteristic;
    if (!ready || gatt == null || characteristic == null) {
      rejectWrite("DISCONNECTED", "DrainGuard Bluetooth disconnected.");
      return;
    }

    boolean started;
    try {
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
        started =
            gatt.writeCharacteristic(
                    characteristic,
                    chunk,
                    BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT)
                == BluetoothStatusCodes.SUCCESS;
      } else {
        characteristic.setWriteType(BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT);
        characteristic.setValue(chunk);
        started = gatt.writeCharacteristic(characteristic);
      }
    } catch (RuntimeException error) {
      started = false;
    }

    if (started) {
      writeStartAttempt = 0;
      return;
    }

    // A false return usually means Android still considers a previous GATT
    // operation busy. Keep the chunk and retry instead of dropping it and
    // immediately failing setup.
    writeQueue.addFirst(chunk);
    writeStartAttempt++;
    if (writeStartAttempt <= MAX_WRITE_START_ATTEMPTS) {
      handler.removeCallbacks(writeRetry);
      handler.postDelayed(writeRetry, WRITE_RETRY_DELAY_MS);
    } else {
      rejectWrite(
          "WRITE_FAILED",
          "Android Bluetooth stayed busy while sending the setup command. Reconnect to DrainGuard and retry.");
    }
  }

  private void emitStatus(byte[] value) {
    if (value == null || value.length == 0) return;
    WritableMap event = Arguments.createMap();
    event.putString("payload", new String(value, StandardCharsets.UTF_8));
    emit("DrainGuardBleStatus", event);
  }

  private void emitState(String state, String message) {
    WritableMap event = Arguments.createMap();
    event.putString("state", state);
    event.putString("message", message);
    emit("DrainGuardBleState", event);
  }

  private void emit(String eventName, WritableMap event) {
    if (!reactContext.hasActiveCatalystInstance()) return;
    reactContext
        .getJSModule(DeviceEventManagerModule.RCTDeviceEventEmitter.class)
        .emit(eventName, event);
  }

  private void rejectConnection(String code, String message) {
    handler.removeCallbacks(connectionTimeout);
    if (connectionPromise != null) {
      connectionPromise.reject(code, message);
      connectionPromise = null;
    }
  }

  private void rejectWrite(String code, String message) {
    handler.removeCallbacks(writeRetry);
    writeQueue.clear();
    writeStartAttempt = 0;
    if (writePromise != null) {
      writePromise.reject(code, message);
      writePromise = null;
    }
  }

  @SuppressLint("MissingPermission")
  private void closeGatt() {
    handler.removeCallbacks(connectionTimeout);
    handler.removeCallbacks(serviceDiscoveryFallback);
    handler.removeCallbacks(reconnectGatt);
    connectingDevice = null;
    connectionAttempt = 0;
    rejectWrite("DISCONNECTED", "DrainGuard Bluetooth disconnected.");
    if (bluetoothGatt != null) {
      BluetoothGatt gatt = bluetoothGatt;
      bluetoothGatt = null;
      try {
        gatt.disconnect();
      } catch (RuntimeException ignored) {
        // The Android Bluetooth service may already be unavailable.
      }
      gatt.close();
    }
    resetGattState();
  }

  @SuppressLint("MissingPermission")
  private void closeGattSession(BluetoothGatt gatt) {
    handler.removeCallbacks(serviceDiscoveryFallback);
    if (gatt == bluetoothGatt) bluetoothGatt = null;
    try {
      gatt.close();
    } catch (RuntimeException ignored) {
      // The failed Android GATT session may already be closed.
    }
    resetGattState();
  }

  private void resetGattState() {
    ready = false;
    serviceDiscoveryStarted = false;
    bondingRequested = false;
    notificationSetupStarted = false;
    commandCharacteristic = null;
    statusCharacteristic = null;
  }

  @Override
  public void invalidate() {
    stopScanInternal();
    closeGatt();
    try {
      reactContext.unregisterReceiver(bondReceiver);
    } catch (IllegalArgumentException ignored) {
      // Receiver was already removed by Android.
    }
    super.invalidate();
  }
}
