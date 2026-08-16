# Drain Guard Complete Setup Guide

This guide will walk you through setting up the entire Drain Guard system from scratch.

## Table of Contents
1. [Hardware Assembly](#hardware-assembly)
2. [Firmware Installation](#firmware-installation)
3. [Mobile App Installation](#mobile-app-installation)
4. [System Configuration](#system-configuration)
5. [Testing](#testing)
6. [Deployment](#deployment)

## Hardware Assembly

### Required Components

**Main Components:**
- ESP32 DevKit V1 (30-pin) x1
- ESP32-CAM module x1
- HC-SR04 Ultrasonic Sensor x1
- TB6612FNG Motor Driver x1
- A9G GPS/GPRS Module x1
- A7670 GSM/LTE Module x1
- DC Geared Motors x2

**Power & Accessories:**
- 12V 3A Power Supply x1
- 5V Buck Converter x1
- GPS Antenna x1
- GSM Antenna x1
- SIM Card (activated) x1
- Breadboard or PCB
- Jumper Wires
- Mounting Hardware
- Waterproof Enclosure

### Assembly Steps

1. **Prepare the Enclosure**
   - Drill holes for motors, sensors, and cables
   - Add cable glands for waterproofing
   - Ensure adequate ventilation

2. **Mount Electronics**
   - Mount ESP32 on standoffs
   - Secure motor driver
   - Position GPS and GSM modules for antenna access

3. **Wire Connections**
   - Follow the wiring diagram in `docs/wiring-diagram.md`
   - Use color-coded wires (red=power, black=ground)
   - Double-check all connections before powering on
   - Secure connections with heat shrink tubing

4. **Install Sensors**
   - Mount ultrasonic sensor pointing downward
   - Ensure clear line of sight to water surface
   - Mount at appropriate height (account for maximum water level)

5. **Mount Motors**
   - Attach motors to drain cover mechanism
   - Test mechanical operation manually
   - Ensure smooth operation and adequate torque

6. **Install Antennas**
   - Attach GPS antenna (external mounting preferred)
   - Attach GSM antenna (keep away from GPS antenna)
   - Route cables properly to avoid interference

## Firmware Installation

### 1. Install Development Environment

**Option A: PlatformIO (Recommended)**
```bash
# Install VS Code
# Install PlatformIO extension from VS Code marketplace
```

**Option B: Arduino IDE**
```bash
# Download from arduino.cc
# Install ESP32 board support
```

### 2. Configure Firmware

Edit `firmware/src/config.h`:

```cpp
// WiFi Settings
#define WIFI_SSID "YourNetworkName"
#define WIFI_PASSWORD "YourNetworkPassword"

// Alert Settings
#define ALERT_PHONE_NUMBER "+1234567890"

// Thresholds (adjust based on your drain)
#define CRITICAL_LEVEL 20.0  // cm
#define WARNING_LEVEL 50.0   // cm
#define TANK_HEIGHT 200.0    // cm

// Auto-open feature
#define AUTO_OPEN_DRAIN true
```

### 3. Upload Main Firmware

**PlatformIO:**
1. Open project folder in VS Code
2. Connect ESP32 via USB
3. Click Upload button (→)
4. Wait for upload to complete

**Arduino IDE:**
1. Open `firmware/src/main.cpp`
2. Select Board: "ESP32 Dev Module"
3. Select correct COM port
4. Click Upload

### 4. Upload ESP32-CAM Firmware

1. Connect ESP32-CAM via FTDI adapter:
   - Connect IO0 to GND (programming mode)
   - Connect VCC to 5V
   - Connect GND to GND
   - Connect TX to RX, RX to TX

2. Open `firmware/esp32cam/esp32cam.ino`
3. Select Board: "ESP32 Wrover Module"
4. Upload firmware
5. Disconnect IO0 from GND
6. Reset ESP32-CAM

### 5. Verify Firmware

Open Serial Monitor (115200 baud):

```
Drain Guard System Starting...
Connecting to WiFi...
WiFi connected!
IP Address: 192.168.1.100
All modules initialized
HTTP server started
```

**Note the IP address - you'll need it for the mobile app!**

## Mobile App Installation

### For Development/Testing

1. **Install Prerequisites**
```bash
# Install Node.js (v16+)
node --version

# Install React Native CLI
npm install -g react-native-cli
```

2. **Install Dependencies**
```bash
cd mobile-app
npm install

# For iOS
cd ios && pod install && cd ..
```

3. **Configure Device IP**

Edit `mobile-app/src/services/api.js`:
```javascript
let DEVICE_IP = '192.168.1.100'; // Your ESP32 IP from step 5
```

4. **Run on Device**

**iOS:**
```bash
npm run ios
```

**Android:**
```bash
npm run android
```

### For Production

See `mobile-app/README.md` for building production APK/IPA.

## System Configuration

### 1. Network Setup

**WiFi Configuration:**
- Use 2.4GHz network (ESP32 compatible)
- Assign static IP to ESP32 (recommended)
- Configure router to allow local network communication
- Open necessary ports if remote access needed

**Cellular Configuration:**
- Insert activated SIM card into A7670
- Verify cellular signal strength
- Test SMS functionality

### 2. GPS Configuration

- Position GPS antenna with clear sky view
- Allow 1-3 minutes for initial GPS fix
- Verify coordinates via Serial Monitor or API

### 3. Camera Setup

- Power on ESP32-CAM
- Connect to same WiFi network
- Note camera IP address from Serial Monitor
- Test stream: `http://[camera-ip]/stream`

### 4. Mobile App Settings

Open app Settings screen:
- Set Device IP Address
- Set Alert Phone Number
- Configure Water Level Thresholds
- Enable/Disable Auto-Open Drain
- Enable Push Notifications

## Testing

### 1. Sensor Test

```bash
# Send test command via Serial Monitor
sensor

# Expected output:
Distance: 154.5 cm
Water Level: 45.5 cm
```

### 2. Motor Test

**Via Serial Monitor:**
```bash
open   # Should open drain
close  # Should close drain
```

**Via Mobile App:**
- Tap "Open" button
- Observe motor operation
- Tap "Close" button
- Verify smooth operation

### 3. GPS Test

```bash
# Serial Monitor
gps

# Expected output:
Lat: 37.774929, Lon: -122.419418
Satellites: 8
Fix: Valid
```

### 4. SMS Test

Send SMS to device:
```
STATUS
```

Expected reply:
```
Drain Guard Status:
Water Level: 45.5cm
Drain: CLOSED
Location: 37.7749, -122.4194
```

### 5. Camera Test

- Open Live Stream in mobile app
- Verify video feed appears
- Check for smooth streaming
- Test different network conditions

### 6. Alert Test

Manually trigger water level:
1. Simulate high water (cover sensor)
2. Verify alert SMS sent
3. Check auto-open if enabled
4. Verify mobile app notification

## Deployment

### 1. Physical Installation

- Mount enclosure securely
- Position ultrasonic sensor correctly
- Ensure drain cover mechanism operates freely
- Weatherproof all connections
- Secure cables to prevent damage

### 2. Power Connection

- Connect 12V power supply
- Verify voltage at all components
- Check current draw
- Install battery backup (optional)

### 3. Final Checks

- [ ] All sensors reading correctly
- [ ] Motors operating smoothly
- [ ] GPS acquiring fix
- [ ] SMS sending/receiving
- [ ] WiFi connected
- [ ] Camera streaming
- [ ] Mobile app connected
- [ ] Alerts functioning
- [ ] Enclosure waterproof

### 4. Monitoring

- Monitor Serial output for first 24 hours
- Check for any error messages
- Verify regular telemetry updates
- Test alert system with manual trigger

## Maintenance

### Daily
- Check mobile app for status updates
- Verify system is online

### Weekly
- Clean ultrasonic sensor
- Check motor operation
- Inspect for water ingress

### Monthly
- Clean camera lens
- Check all connections
- Verify GPS accuracy
- Test SMS alerts
- Update firmware if needed

## Troubleshooting

See individual README files:
- `firmware/README.md` - Firmware issues
- `mobile-app/README.md` - App issues
- `docs/wiring-diagram.md` - Hardware issues

## Safety Warnings

⚠️ **Important Safety Notes:**

1. **Electrical Safety**
   - Disconnect power before making changes
   - Use proper insulation
   - Install GFCI protection if near water

2. **Water Protection**
   - Use IP65+ rated enclosure
   - Seal all cable entries
   - Test waterproofing before deployment

3. **Mechanical Safety**
   - Add limit switches to prevent over-travel
   - Implement emergency stop mechanism
   - Test under load before deployment

4. **Network Security**
   - Change default passwords
   - Use WPA2/WPA3 encryption
   - Consider VPN for remote access
   - Keep firmware updated

## Support

For issues or questions:
1. Check troubleshooting sections in README files
2. Review Serial Monitor output
3. Verify all connections per wiring diagram
4. Test components individually

## License

MIT License - See LICENSE file for details
