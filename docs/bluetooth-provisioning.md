# DrainGuard Bluetooth WiFi Provisioning

DrainGuard uses Bluetooth Low Energy (BLE) GATT provisioning so the same workflow works on Android and iOS. The ESP32 advertises as `DrainGuard-XXXX`, where `XXXX` is the final four hexadecimal digits of its factory MAC address.

## Provisioning flow

1. Power the DrainGuard robot and open **WiFi Setup** from the app dashboard.
2. Tap **Scan** and allow Bluetooth/nearby-device access.
3. Select the `DrainGuard-XXXX` device. The operating system establishes a BLE connection.
4. Enter an SSID manually or tap **Find WiFi** to ask the ESP32 to scan nearby 2.4 GHz networks.
5. Enter the WiFi password and, optionally, a telemetry API endpoint.
6. Tap **Configure WiFi**.
7. The ESP32 reports `connecting`, followed by `connected` or `failed`. A connection attempt times out after 30 seconds.
8. After success, the ESP32 stores the credentials in NVS. The app keeps using the private hotspot address for camera and arm controls; the password is never stored by the app.

If connection fails, BLE remains available and the user can correct the credentials and retry without rebooting or reflashing the ESP32.

## Relationship to the private hotspot

BLE provisioning configures the DevKit's optional station-mode internet uplink. Independently, the DevKit maintains the `DrainGuard-Robot` private hotspot at `192.168.4.1`. The ESP32-CAM joins that hotspot at the fixed address `192.168.4.50`, and the phone can join it for router-free local control and video.

The firmware uses `WIFI_AP_STA`, so an uplink connection or failed provisioning attempt does not turn off the private hotspot. Hotspot credentials are compile-time settings in `firmware/src/config.h` and must match the values in `firmware/esp32cam/esp32cam.ino`.

## Firmware behavior

The implementation is in:

- `firmware/src/wifi_provisioning.h`
- `firmware/src/wifi_provisioning.cpp`
- `firmware/src/main.cpp`

The provisioning manager starts before the robot hardware modules and never blocks startup. It first tries credentials stored under the NVS namespace `drainguard`. If no saved credentials exist, it can use non-placeholder values from `config.h`; otherwise it waits for BLE provisioning.

Credentials are committed only after `WL_CONNECTED`, so a failed provisioning attempt does not overwrite the last known working configuration. The stored keys are:

| NVS key | Value |
| --- | --- |
| `configured` | Whether a valid configuration was saved |
| `ssid` | WiFi network name |
| `password` | WiFi password |
| `api_endpoint` | Telemetry API endpoint |

`BLE_PROVISIONING_STAY_ACTIVE` in `firmware/src/config.h` controls whether BLE advertising can restart after WiFi succeeds. It defaults to `true` so deployed devices can be reconfigured later.

## BLE protocol

| Item | UUID / value |
| --- | --- |
| Service | `7b0d1001-5f6b-4c4f-9a7e-2f3b4d5e6f70` |
| Command characteristic | `7b0d1002-5f6b-4c4f-9a7e-2f3b4d5e6f70` (write with response) |
| Status characteristic | `7b0d1003-5f6b-4c4f-9a7e-2f3b4d5e6f70` (read/notify) |
| Message encoding | UTF-8 JSON terminated by `\n` |

The mobile app splits each JSON command into conservative 18-byte BLE chunks and terminates it with `\n`. The ESP32 reassembles chunks until the newline delimiter.

Configure WiFi:

```json
{
  "command": "set_wifi",
  "ssid": "MyHotspot",
  "password": "MyPassword123",
  "api_endpoint": "https://server.com/api/telemetry"
}
```

The `api_endpoint` member may be omitted to keep the endpoint already configured on the ESP32.

Scan WiFi networks:

```json
{ "command": "scan_wifi" }
```

Forget the saved uplink credentials while leaving the private robot hotspot active:

```json
{ "command": "forget_wifi" }
```

Status notifications include:

| Status | Meaning |
| --- | --- |
| `ready` | BLE is connected and ready for commands |
| `scanning_wifi` | ESP32 network scan started |
| `network` | One discovered network; includes `ssid`, `rssi`, and `secure` |
| `scan_complete` | Network scan completed |
| `connecting` | ESP32 is attempting to join the supplied SSID |
| `connected` | WiFi connected; includes `ssid`, station `ip`, `hotspot_ip`, and `saved` |
| `failed` | WiFi failed; includes a machine-readable `reason` |
| `forgotten` | Saved uplink credentials were cleared; the hotspot remains active |
| `clear_failed` | Saved uplink credentials could not be cleared |
| `invalid` | Malformed or invalid command |
| `busy` | Command queue is full |

## Mobile dependency installation

From `mobile-app`:

```bash
npm install
```

The app uses `react-native-ble-plx` and `buffer`. BLE requires a native build; it will not work in a JavaScript-only preview environment.

This repository snapshot does not contain `android/` or `ios/` project directories. Apply the following settings after restoring or generating the React Native native projects.

### Android

Ensure the Android minimum SDK is at least 23. Add these entries directly under the `<manifest>` element in `android/app/src/main/AndroidManifest.xml`:

```xml
<uses-permission android:name="android.permission.BLUETOOTH_SCAN"
    android:usesPermissionFlags="neverForLocation" />
<uses-permission android:name="android.permission.BLUETOOTH_CONNECT" />

<uses-permission android:name="android.permission.BLUETOOTH"
    android:maxSdkVersion="30" />
<uses-permission android:name="android.permission.BLUETOOTH_ADMIN"
    android:maxSdkVersion="30" />
<uses-permission android:name="android.permission.ACCESS_FINE_LOCATION"
    android:maxSdkVersion="30" />

<uses-feature android:name="android.hardware.bluetooth_le" android:required="true" />
```

Android 12 and newer prompt for nearby-device scan/connect access. Android 11 and older prompt for location because those platform versions gate BLE scanning behind location permission.

### iOS

Run CocoaPods after installing dependencies:

```bash
cd ios
pod install
cd ..
```

Add this key to `ios/DrainGuardApp/Info.plist`:

```xml
<key>NSBluetoothAlwaysUsageDescription</key>
<string>DrainGuard uses Bluetooth to securely configure your robot's WiFi connection.</string>
```

Background BLE mode is not required because provisioning runs while the setup screen is open.

## Build and test

Firmware:

```bash
cd firmware
pio run
pio run --target upload
pio device monitor --baud 115200
```

Mobile app:

```bash
cd mobile-app
npm run android
# or
npm run ios
```

Test on a physical phone; BLE scanning is generally unavailable or unreliable in simulators.

## Security model

- Both GATT characteristics must be readable/writable without requiring a saved Android bond.
- The ESP32 initiates encrypted bonding when a phone connects. Pairing remains
  compatible with phones that support only legacy BLE security; newer phones
  may negotiate LE Secure Connections.
- The ESP32 never prints the password to serial output.
- The app never stores the WiFi password.
- Credentials are stored in ESP32 NVS and retained across power loss.

Because the ESP32 DevKit has no trusted display or keypad, pairing uses Bluetooth's **Just Works** association model. The link is encrypted, but initial pairing does not have passkey-based man-in-the-middle protection. For hostile or public deployment environments, add a physical provisioning window/button or a per-device QR/passkey flow. For at-rest protection, enable ESP32 Secure Boot and Flash Encryption/NVS encryption in the production build. Use HTTPS for the telemetry endpoint whenever possible.

## Troubleshooting

- **Robot is not found:** Confirm Bluetooth is enabled, the phone is close to the ESP32, permissions were granted, and the flashed board supports BLE.
- **Pairing fails repeatedly:** Forget/remove the DrainGuard device in system Bluetooth settings, restart Bluetooth, and retry.
- **WiFi network is not listed:** ESP32 supports 2.4 GHz WiFi only. Hidden networks can still be entered manually.
- **Authentication fails:** Re-enter the password; open networks require an empty password.
- **Connection times out:** Move the robot closer to the access point and verify the SSID is visible.
- **App builds but BLE is unavailable:** Confirm the native permissions above are present and rebuild the native application after installing `react-native-ble-plx`.

## References

- [Espressif Arduino BLE API](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/ble.html)
- [Espressif Preferences/NVS API](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/preferences.html)
- [react-native-ble-plx setup and API](https://github.com/dotintent/react-native-ble-plx)
