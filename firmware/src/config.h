#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration
#define WIFI_SSID "YourWiFiSSID"
#define WIFI_PASSWORD "YourWiFiPassword"

// API Configuration
#define API_ENDPOINT "http://your-server.com/api/telemetry"
#define DEVICE_ID "DRAIN_GUARD_001"

// BLE WiFi Provisioning
#define WIFI_PROVISIONING_TIMEOUT_MS 30000
#define BLE_PROVISIONING_STAY_ACTIVE true

// Private robot hotspot. The ESP32 DevKit keeps this access point active so
// the mobile app and ESP32-CAM can communicate without an external router.
// WPA2 passwords must contain at least 8 characters.
#define DRAINGUARD_AP_SSID "DrainGuard-Robot"
#define DRAINGUARD_AP_PASSWORD "DrainGuard123"
#define DRAINGUARD_AP_CHANNEL 1
#define DRAINGUARD_AP_MAX_CLIENTS 4

// ESP32-CAM address on the private robot hotspot.
#define CAMERA_IP_ADDRESS "192.168.4.50"
#define CAMERA_HTTP_PORT 80

// Alert Configuration
#define ALERT_PHONE_NUMBER "+1234567890"
#define AUTO_OPEN_DRAIN true

// Pin Definitions - ESP32 DevKit V1 30-pin
// Ultrasonic Sensor HC-SR04
#define TRIG_PIN 25
#define ECHO_PIN 34

// TB6612 Motor Driver
#define MOTOR_AIN1 27
#define MOTOR_AIN2 14
#define MOTOR_BIN1 12
#define MOTOR_BIN2 23
#define MOTOR_PWMA 26
#define MOTOR_PWMB 13
#define MOTOR_STBY 4

// A9G GPS Module (Serial communication)
#define A9G_RX 33  // ESP32 RX <- A9G TX
#define A9G_TX 32  // ESP32 TX -> A9G RX
#define A9G_POWER 5

// A7670 SMS Module (Serial2)
#define A7670_RX 16  // ESP32 RX2 <- A7670 TX
#define A7670_TX 17  // ESP32 TX2 -> A7670 RX
#define A7670_POWER 15

// PCA9685 Servo Controller (I2C)
#define PCA9685_SDA 21
#define PCA9685_SCL 22
#define PCA9685_ADDRESS 0x40

// ESP32-CAM is a separate WiFi module; no DevKit GPIO connection is required.
// These legacy optional I2C definitions remain reserved for compatibility.
#define CAM_SDA 21
#define CAM_SCL 22

// System Configuration
#define SENSOR_READ_INTERVAL 2000  // ms
#define TELEMETRY_INTERVAL 10000   // ms
#define GPS_UPDATE_INTERVAL 5000   // ms

// Motor Configuration
#define MOTOR_SPEED 200  // 0-255
#define DRAIN_OPEN_TIME 5000  // ms to fully open
#define DRAIN_CLOSE_TIME 5000 // ms to fully close

// Ultrasonic Sensor Configuration
#define MAX_DISTANCE 400  // cm
#define TANK_HEIGHT 200   // cm (adjust to your drain depth)

// PCA9685 Servo Configuration
#define SERVO_FREQ 60     // Analog servos run at ~60 Hz
#define SERVO_BASE 0      // Base servo channel
#define SERVO_SHOULDER 1  // Shoulder servo channel
#define SERVO_ELBOW 2     // Elbow servo channel
#define SERVO_GRIPPER 3   // Gripper servo channel

// Servo position limits (adjust based on your servos)
#define SERVO_BASE_MIN 150
#define SERVO_BASE_MAX 450
#define SERVO_SHOULDER_MIN 150
#define SERVO_SHOULDER_MAX 380
#define SERVO_ELBOW_MIN 300
#define SERVO_ELBOW_MAX 380
#define SERVO_GRIPPER_MIN 410
#define SERVO_GRIPPER_MAX 510

#endif
