# Drain Guard IoT System

A comprehensive drain monitoring and management system using ESP32 with mobile app control.

## Features

- **Real-time Monitoring**: Ultrasonic sensor for water level detection
- **GPS Tracking**: A9G module for location tracking
- **SMS Alerts**: A7670 module for SMS notifications
- **Live Streaming**: ESP32-CAM for real-time video monitoring
- **Motor Control**: TB6612 dual motor driver for drain cover operation
- **Mobile App**: React Native app for iOS and Android
- **Bluetooth Provisioning**: Configure WiFi and telemetry settings without reflashing
- **Private Robot Hotspot**: Local control and camera streaming without a router

## Hardware Components

- ESP32 DevKit V1 (30-pin)
- A9G GPS/GPRS Module
- A7670 GSM/LTE Module
- HC-SR04 Ultrasonic Sensor
- TB6612FNG Dual Motor Driver
- ESP32-CAM Module
- 12V Power Supply
- DC Motors (2x)

## Project Structure

```
drain-guard/
├── firmware/           # ESP32 firmware (Arduino/PlatformIO)
├── mobile-app/         # React Native mobile application
├── docs/              # Documentation and schematics
└── README.md
```

## Getting Started

### Firmware Setup
1. Install Arduino IDE or PlatformIO
2. Install required libraries (see firmware/README.md)
3. Configure the private hotspot credentials and optional internet uplink
4. Upload the DevKit firmware, then the ESP32-CAM firmware
5. Connect the phone to `DrainGuard-Robot` for local control

### Mobile App Setup
1. Install Node.js 16+, JDK 17, and the Android SDK
2. Run `npm ci` in the `mobile-app` directory
3. Run `npm run android:apk` to create a standalone installable APK
4. Install `mobile-app/android/app/build/outputs/apk/release/app-release.apk` on the phone
5. Connect the phone to `DrainGuard-Robot`; the app defaults to controller address `192.168.4.1`

## Wiring Diagram

See `docs/wiring-diagram.md` for detailed pin connections.

## License

MIT License
