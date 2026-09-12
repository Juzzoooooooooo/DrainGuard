# Drain Guard Firmware

ESP32-based firmware for the Drain Guard IoT system.

## Hardware Requirements

- ESP32 DevKit V1 (30-pin)
- HC-SR04 Ultrasonic Sensor
- TB6612FNG Dual Motor Driver
- A9G GPS/GPRS Module
- A7670 GSM/LTE Module
- ESP32-CAM (separate module)
- DC Motors (2x)
- 12V Power Supply

## Software Requirements

### PlatformIO (Recommended)
1. Install [Visual Studio Code](https://code.visualstudio.com/)
2. Install [PlatformIO extension](https://platformio.org/install/ide?install=vscode)
3. Open project folder in VS Code
4. PlatformIO will auto-install dependencies

### Arduino IDE (Alternative)
1. Install [Arduino IDE](https://www.arduino.cc/en/software)
2. Add ESP32 board support:
   - Go to File → Preferences
   - Add to Additional Board Manager URLs:
     `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Go to Tools → Board → Boards Manager
   - Search "ESP32" and install "esp32 by Espressif Systems"

## Required Libraries

Install these libraries via Arduino Library Manager or PlatformIO:

- **ArduinoJson** (^6.21.3) - JSON parsing and generation
- **ESPAsyncWebServer** - Async web server
- **AsyncTCP** - Async TCP library

## Configuration

### Bluetooth WiFi provisioning (recommended)

The firmware advertises a BLE setup service as `DrainGuard-XXXX`. Use the mobile app's **WiFi Setup** screen to scan for the robot, send WiFi credentials, optionally configure the telemetry endpoint, and receive live connection status. Successful credentials are persisted in ESP32 NVS and reused after restart.

Provisioning uses a non-blocking 30-second WiFi attempt. If it fails, the robot continues operating and BLE remains available for another attempt. See [`docs/bluetooth-provisioning.md`](../docs/bluetooth-provisioning.md) for the protocol, native mobile permissions, security model, and complete setup instructions.

### Private robot hotspot and camera

The DevKit keeps a private WPA2 hotspot active independently of the optional BLE-provisioned internet connection:

| Device | Network role | Address |
|---|---|---|
| ESP32 DevKit V1 | Hotspot and robot API | `192.168.4.1` |
| ESP32-CAM | Hotspot client and video server | `192.168.4.50` |
| Mobile phone | Hotspot client | Assigned automatically |

The default hotspot is `DrainGuard-Robot` with password `DrainGuard123`. Change `DRAINGUARD_AP_SSID` and `DRAINGUARD_AP_PASSWORD` in `src/config.h` before deployment, then make the identical change in `esp32cam/esp32cam.ino`. The password must contain at least eight characters.

This uses `WIFI_AP_STA`, so local control and video continue working without a router while the station interface can still use BLE-provisioned WiFi for telemetry. The ESP32-CAM communicates entirely over WiFi; there are no GPIO data wires between the two ESP32 boards.

### Compile-time fallback

1. Edit `src/config.h`:

```cpp
// WiFi credentials
#define WIFI_SSID "YourWiFiSSID"
#define WIFI_PASSWORD "YourWiFiPassword"

// Alert phone number
#define ALERT_PHONE_NUMBER "+1234567890"

// API endpoint (optional cloud backend)
#define API_ENDPOINT "http://your-server.com/api/telemetry"

// Private robot hotspot
#define DRAINGUARD_AP_SSID "DrainGuard-Robot"
#define DRAINGUARD_AP_PASSWORD "replace-with-a-private-password"
```

2. Adjust pin assignments if needed (see wiring diagram)

3. Configure thresholds:
   - `CRITICAL_LEVEL`: Water level threshold for critical alert
   - `WARNING_LEVEL`: Water level threshold for warning
   - `TANK_HEIGHT`: Total depth of drain/tank

## Pin Configuration

See `docs/wiring-diagram.md` for complete wiring details.

## Building and Uploading

### PlatformIO
1. Connect ESP32 via USB
2. Click the upload button (→) in VS Code
3. Monitor serial output with serial monitor button

### Arduino IDE
1. Select board: Tools → Board → ESP32 Dev Module
2. Select port: Tools → Port → (your COM port)
3. Click Upload button
4. Open Serial Monitor (115200 baud)

## ESP32-CAM Firmware

The ESP32-CAM runs separate firmware for video streaming and joins the DevKit hotspot at `192.168.4.50`:

1. Open `firmware/esp32cam/esp32cam.ino` in Arduino IDE
2. Confirm that `ssid` and `password` match the hotspot values in `src/config.h`
3. Select board: **AI Thinker ESP32-CAM**
4. Connect ESP32-CAM via FTDI adapter:
   - FTDI TX → ESP32-CAM RX
   - FTDI RX → ESP32-CAM TX
   - GND → GND
   - 5V → 5V
   - IO0 → GND (for programming mode)
5. Upload firmware
6. Remove IO0 to GND connection
7. Reset ESP32-CAM
8. Check the 115200-baud serial monitor for `192.168.4.50`

Upload and power the DevKit firmware first so its hotspot is available when the camera starts.

### Local mobile connection

1. Power the ESP32 DevKit and wait for `DrainGuard hotspot ready` in Serial Monitor.
2. Power the ESP32-CAM and wait for it to connect at `192.168.4.50`.
3. Connect the phone to the `DrainGuard-Robot` WiFi network. A “no internet” notice is expected in local-only mode.
4. In the app's **Settings**, set **Device IP address** to `192.168.4.1`.
5. Test `http://192.168.4.50/capture` in the phone browser, then open **Live Camera** in the app.

## API Endpoints

Once running, the ESP32 exposes these REST endpoints:

- `GET /api/status` - Get current system status
- `POST /api/drain/open` - Open drain cover
- `POST /api/drain/close` - Close drain cover
- `GET /api/gps` - Get GPS coordinates
- `GET /api/camera/stream` - Get camera stream URL

## Testing

### 1. Serial Monitor Test
```
Drain Guard System Starting...
BLE provisioning active as DrainGuard-1A2B
DrainGuard hotspot ready: DrainGuard-Robot at http://192.168.4.1
ESP32-CAM expected at http://192.168.4.50:80
Loaded WiFi configuration for SSID: MyHotspot
Ultrasonic sensor initialized
Motor controller initialized
A9G GPS module initialized
A7670 SMS module initialized
WiFi connected. IP address: 192.168.1.100
HTTP server ready on hotspot: http://192.168.4.1
```

### 2. Ultrasonic Sensor Test
- Check serial output for distance readings
- Should update every 2 seconds
- Valid range: 2-400 cm

### 3. Motor Test
- Send POST request to `/api/drain/open`
- Motors should run forward for configured time
- Send POST request to `/api/drain/close`
- Motors should run backward

### 4. GPS Test
- Takes 1-3 minutes for initial GPS fix (must be outdoors)
- Check serial output for GPS coordinates
- Valid fix shows satellites > 0

### 5. SMS Test
- Send SMS to device with command: "STATUS"
- Should receive reply with current status
- Commands: OPEN, CLOSE, STATUS

## Troubleshooting

### WiFi Won't Connect
- Open **WiFi Setup** in the mobile app and retry provisioning
- Check the saved SSID and password, or the compile-time fallback in config.h
- Ensure 2.4GHz WiFi (ESP32 doesn't support 5GHz)
- Check WiFi signal strength

### Ultrasonic Sensor No Reading
- Check wiring (VCC, GND, TRIG, ECHO)
- Ensure 5V power supply
- Check for obstacles in sensor path

### Motors Don't Move
- Verify 12V power supply connected to VM
- Check STBY pin is HIGH
- Test motor connections directly
- Check TB6612 wiring

### GPS No Fix
- Ensure GPS antenna is connected
- Must be outdoors or near window
- Allow 1-3 minutes for initial fix
- Check A9G power and serial connection

### SMS Not Working
- Verify SIM card is inserted and activated
- Check A7670 power and serial connection
- Ensure sufficient cellular signal
- Check AT commands in serial monitor

### ESP32-CAM Not Streaming
- Verify separate ESP32-CAM is powered and programmed
- Confirm its hotspot credentials exactly match the DevKit configuration
- Connect the phone to `DrainGuard-Robot`
- Test the snapshot at `http://192.168.4.50/capture`
- Test the stream at `http://192.168.4.50/stream`
- Confirm the app's Device IP is `192.168.4.1`
- Ensure adequate 5V power (brown-out common issue)

## Power Consumption

Typical current draw:
- ESP32: ~500mA
- A9G: 500-800mA peak
- A7670: 1-2A peak
- Motors: 500-1000mA each
- ESP32-CAM: 300-500mA

**Recommended power supply: 12V 3A**

## Serial Commands (Debug)

Send via Serial Monitor (115200 baud):
- `status` - Print system status
- `gps` - Print GPS coordinates
- `open` - Open drain
- `close` - Close drain
- `sensor` - Read ultrasonic sensor

## Safety Notes

1. **Water Protection**: Ensure electronics are waterproof/weather-resistant
2. **Motor Safety**: Add limit switches to prevent over-travel
3. **Power Safety**: Use appropriate fuses and circuit protection
4. **Testing**: Always test in safe environment before deployment
5. **Emergency Stop**: Implement manual override mechanism

## License

MIT License
