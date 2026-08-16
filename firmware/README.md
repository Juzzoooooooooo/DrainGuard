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

1. Edit `src/config.h`:

```cpp
// WiFi credentials
#define WIFI_SSID "YourWiFiSSID"
#define WIFI_PASSWORD "YourWiFiPassword"

// Alert phone number
#define ALERT_PHONE_NUMBER "+1234567890"

// API endpoint (optional cloud backend)
#define API_ENDPOINT "http://your-server.com/api/telemetry"
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

The ESP32-CAM runs separate firmware for video streaming:

1. Open `firmware/esp32cam/esp32cam.ino` in Arduino IDE
2. Select board: ESP32 Wrover Module
3. Connect ESP32-CAM via FTDI adapter:
   - FTDI TX → ESP32-CAM RX
   - FTDI RX → ESP32-CAM TX
   - GND → GND
   - 5V → 5V
   - IO0 → GND (for programming mode)
4. Upload firmware
5. Remove IO0 to GND connection
6. Reset ESP32-CAM
7. Check serial monitor for IP address

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
Connecting to WiFi...
WiFi connected!
IP Address: 192.168.1.100
Ultrasonic sensor initialized
Motor controller initialized
A9G GPS module initialized
A7670 SMS module initialized
HTTP server started
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
- Check SSID and password in config.h
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
- Check WiFi connection on ESP32-CAM
- Access stream at: http://[ESP32-CAM-IP]/stream
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
