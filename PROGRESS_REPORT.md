DrainGuard Robot - Progress Report

Project Duration: July 20, 2024 - August 26, 2024  
Developer: Juzzo  
Status: 🟢 Functional Prototype Complete

Project Overview

The DrainGuard project is an autonomous drain monitoring and cleaning robot featuring GPS tracking, SMS alerts, motor control, a 4-axis servo robotic arm, and dual camera systems. The system integrates ESP32 microcontroller, TB6612FNG motor driver, PCA9685 servo controller, ultrasonic sensor, GPS (A9G), GSM module (A7670), and ESP32-CAM for live surveillance. The project includes firmware (C++/PlatformIO) and mobile app (React Native).

Week 1-2: July 20 - August 3, 2024
Initial Setup & Hardware Integration

The project began with component selection and pin mapping. I finalized the ESP32 DevKit V1 as the main controller and integrated TB6612FNG motor driver, PCA9685 servo controller, HC-SR04 ultrasonic sensor, A9G GPS module, and A7670 GSM module. I created comprehensive documentation including pin mapping tables and wiring diagrams. The firmware architecture was set up in PlatformIO with modular design using separate headers for config, sensors, motors, GPS, SMS, camera, servo, and auto mode. Power system design was completed using 12V main supply with 5V buck converter distribution.

Week 3: August 4 - August 14, 2024
Mobile Application Development & Initial Assembly

I developed the mobile application using React Native with six main screens: Home Dashboard, GPS Map View, Live Camera Stream, Servo Control Panel, Event History, and Settings. The API service layer was implemented for ESP32 communication with REST endpoints and WebSocket support for real-time data. I integrated React Navigation for screen management and implemented state management for telemetry data. The app features real-time water level monitoring, manual motor control, servo arm manipulation, and camera feed display. By August 14, the robot was physically assembled and the mobile app was completed. Initial testing showed all major systems were functional.

Week 4: August 15 - August 18, 2024
Acrylic Frame Design & Planning

After completing the initial assembly, I focused on designing a custom acrylic chassis for better component organization and protection. I sketched the frame design using 5mm acrylic sheets and planned mounting positions for electronics, motors, camera, battery, and servo arm. During this planning phase, I also prepared measurements and templates for the manual cutting process.

Week 5: August 19 - August 23, 2024
ESP32-CAM System Failure & Acrylic Frame Construction

On August 19, during routine testing, the ESP32-CAM module suddenly failed. The camera system experienced critical boot failures with error messages indicating hardware initialization problems. The specific issues included: camera driver timeout errors (0x20002), I2C communication failures preventing SCCB initialization, and the OV2640 image sensor not responding to configuration commands. The system would hang at "Camera init..." during boot sequence. I immediately began troubleshooting by checking power supply voltage levels, verifying GPIO pin connections, and testing with different camera modules. On August 20, I attempted firmware reflashing and adjusted power supply decoupling capacitors, but the issue persisted.

The fabrication phase ran concurrently from August 21-23. I manually cut acrylic sheets using a mini drill with cutting disc attachment, hand-drilled all mounting holes, and smoothed edges with sandpaper. By August 23, I successfully replaced the faulty ESP32-CAM with a new unit, added heat sinks for thermal management, improved power supply filtering, and optimized the camera initialization code in firmware. The new module achieved stable 640x480 @ 15fps streaming. I also completed the physical assembly with all components properly mounted on the new acrylic frame using M3 and M4 hardware.

Week 6-7: August 24 - August 26, 2024
Final Assembly & WiFi Provisioning

I completed the final component integration by mounting all electronics: ESP32, motor driver, motors, PCA9685, sensors, GPS/GSM antennas, and the 4-DOF servo arm with gripper mechanism. Cable management and proper routing were implemented with strain relief and tie-downs. The final days focused on advanced connectivity features and build systems. I implemented Bluetooth Low Energy (BLE) provisioning for WiFi configuration, allowing users to set up network credentials without hardcoding. The system supports AP+STA mode, running both hotspot (DrainGuard-XXXX) and WiFi client simultaneously. Secure credential storage uses ESP32 NVS with JSON-based provisioning protocol. I set up the Android build environment, configured Gradle, installed JDK 17 and Android SDK, and successfully compiled the mobile app APK. The firmware supports WiFi network scanning via BLE and automatic reconnection. All project files were organized and pushed to GitHub repository.

Technical Specifications

Hardware: ESP32 DevKit V1, TB6612FNG motor driver, PCA9685 servo controller, HC-SR04 ultrasonic sensor, A9G GPS, A7670 GSM, ESP32-CAM, 12V/3A power supply with 5V buck converter, custom acrylic frame, 4-DOF servo arm, dual DC motors.

Pin Usage: Motors (GPIO 26,27,14,13,12,23,4), Ultrasonic (GPIO 25,34), GPS (GPIO 32,33,5), GSM (GPIO 17,16,15), I2C (GPIO 21,22).

Software: Firmware (C++/PlatformIO), Mobile App (React Native 0.72), Communication (HTTP, WebSocket, BLE).

Features: Autonomous water level monitoring, GPS tracking, SMS alerts, remote motor control, 4-axis arm manipulation, live camera streaming, WiFi provisioning via BLE, hotspot mode, RESTful API.

Key Challenges & Solutions

1. ESP32-CAM System Failure (August 19-23): Camera module experienced critical hardware initialization failures with error code 0x20002, I2C/SCCB communication timeout, and OV2640 sensor not responding. Resolved by hardware replacement, improved power filtering, thermal management with heat sinks, and firmware optimization.
2. Manual Frame Construction: Precise cutting with mini drill required test cuts on scrap material and slow, steady cutting speed.
3. Android Build Setup: Resolved JAVA_HOME and ANDROID_HOME configuration issues for successful APK compilation.
4. Pin Mapping: Navigated ESP32 boot strapping constraints (GPIO 0,12,15) to avoid conflicts.
5. Multi-file Compilation: Confirmed PlatformIO handles multiple .cpp files (main.cpp + wifi_provisioning.cpp) correctly.

Current Status

- Hardware: 95% complete - all components installed and wired
- Firmware: 95% complete - core functionality working, final testing
- Mobile App: 95% complete - APK built and functional
- Documentation: Comprehensive guides created for setup, wiring, and operation

Testing Results

✅ Motor control verified (forward/reverse)  
✅ Ultrasonic sensor operational (2-400cm range)  
✅ GPS location acquisition successful  
✅ SMS messaging functional  
✅ Servo arm full range of motion achieved  
✅ Camera streaming stable at 640x480 @ 15fps  
✅ Mobile app APK builds and installs successfully  
✅ WiFi provisioning via BLE working  
⏳ 24-hour endurance test pending  
⏳ Field deployment testing scheduled  

Next Steps

Immediate: Upload firmware to ESP32, conduct field test in simulated drain environment, test end-to-end system integration.

Short-term: Refine autonomous algorithms, implement cloud data logging, add battery monitoring, create OTA firmware update capability.

Long-term: Deploy pilot unit for real-world testing, gather 2-week performance data, iterate design based on results, prepare final documentation and presentation.

Budget

Total Cost: ₱10,000 (includes all electronics, mechanical components, power system, and replacement parts)

Conclusion

The DrainGuard project successfully progressed from concept to functional prototype despite the ESP32-CAM hardware failure on August 19-23. The robot was initially assembled by August 14 with completed mobile app. The camera system failure involving initialization errors (0x20002) and sensor communication timeout was resolved through hardware replacement and firmware optimization. The manual acrylic frame construction demonstrated practical fabrication skills using basic tools. All major systems are integrated and operational: embedded control, motor/servo actuation, GPS/GSM communication, camera streaming, BLE provisioning, and mobile interface. The system is ready for comprehensive field testing and deployment. This project showcases successful integration of embedded systems, IoT communication, mobile development, and mechanical design.