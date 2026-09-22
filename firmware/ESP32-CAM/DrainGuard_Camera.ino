/*
 * DrainGuard ESP32-CAM Module
 * 
 * This firmware runs on the ESP32-CAM and provides:
 * - WiFi connectivity to DrainGuard hotspot
 * - HTTP web server for camera streaming
 * - RESTful API for camera control
 * - Status reporting
 * 
 * Hardware: ESP32-CAM (AI-Thinker)
 * Camera: OV2640
 * 
 * Upload Instructions:
 * 1. Connect FTDI adapter to ESP32-CAM
 * 2. Set board to "AI Thinker ESP32-CAM"
 * 3. Hold GPIO0 to GND during upload
 * 4. Press reset after upload
 */

#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// ============================================================================
// CONFIGURATION
// ============================================================================

// WiFi credentials for DrainGuard hotspot
#define WIFI_SSID "DrainGuard-Robot"
#define WIFI_PASSWORD "DrainGuard123"

// Static IP on DrainGuard network
#define CAM_IP "192.168.4.50"
#define CAM_GATEWAY "192.168.4.1"
#define CAM_SUBNET "255.255.255.0"

// Camera pins for AI-Thinker ESP32-CAM
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define LED_GPIO_NUM       4  // Built-in flash LED
#define LED_BUILTIN        33 // Status LED

// ============================================================================
// GLOBAL STATE
// ============================================================================

WebServer server(80);

struct CameraSettings {
  int quality;      // 0-3 (0=low, 3=max)
  int brightness;   // -2 to +2
  int contrast;     // -2 to +2
  bool flashEnabled;
  bool streaming;
} settings;

camera_fb_t * fb = NULL;

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

void setupCamera();
void setupWiFi();
void setupWebServer();
void handleStream();
void handleCapture();
void handleStatus();
void handleApiStatus();
void handleApiQuality();
void handleApiBrightness();
void handleApiContrast();
void handleApiFlash();
void handleApiStreamStart();
void handleApiStreamStop();
void applyCameraSettings();
void setFlash(bool enable);

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== DrainGuard ESP32-CAM ===");

  // Initialize LED pins
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_GPIO_NUM, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(LED_GPIO_NUM, LOW);

  // Initialize default settings
  settings.quality = 1;       // Medium quality
  settings.brightness = 0;
  settings.contrast = 0;
  settings.flashEnabled = false;
  settings.streaming = false;

  // Setup camera
  setupCamera();

  // Connect to WiFi
  setupWiFi();

  // Start web server
  setupWebServer();
  server.begin();

  Serial.println("DrainGuard Camera Ready!");
  Serial.printf("Stream URL: http://%s/stream\n", CAM_IP);
  Serial.printf("Capture URL: http://%s/capture\n", CAM_IP);
  
  // Flash LED to indicate ready
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  server.handleClient();
  
  // Status LED blink when streaming
  static unsigned long lastBlink = 0;
  if (settings.streaming && millis() - lastBlink > 1000) {
    lastBlink = millis();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
  
  delay(1);
}

// ============================================================================
// CAMERA SETUP
// ============================================================================

void setupCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // Init with high specs to pre-allocate larger buffers
  if (psramFound()) {
    config.frame_size = FRAMESIZE_SVGA;  // 800x600
    config.jpeg_quality = 10;
    config.fb_count = 2;
    Serial.println("PSRAM found - using high quality");
  } else {
    config.frame_size = FRAMESIZE_VGA;   // 640x480
    config.jpeg_quality = 12;
    config.fb_count = 1;
    Serial.println("No PSRAM - using reduced quality");
  }

  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }

  // Apply initial settings
  applyCameraSettings();
  
  Serial.println("Camera initialized successfully");
}

// ============================================================================
// WIFI SETUP
// ============================================================================

void setupWiFi() {
  Serial.print("Connecting to DrainGuard hotspot...");
  
  // Configure static IP
  IPAddress ip, gateway, subnet;
  ip.fromString(CAM_IP);
  gateway.fromString(CAM_GATEWAY);
  subnet.fromString(CAM_SUBNET);
  
  WiFi.config(ip, gateway, subnet);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" Connected!");
    Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
  } else {
    Serial.println(" Failed!");
    Serial.println("Check WiFi credentials and hotspot");
  }
}

// ============================================================================
// WEB SERVER SETUP
// ============================================================================

void setupWebServer() {
  // Enable CORS
  server.enableCORS(true);

  // Root endpoint
  server.on("/", HTTP_GET, []() {
    String html = "<html><body><h1>DrainGuard Camera</h1>";
    html += "<p><a href='/stream'>Live Stream</a></p>";
    html += "<p><a href='/capture'>Capture Photo</a></p>";
    html += "<p><a href='/status'>Status</a></p>";
    html += "</body></html>";
    server.send(200, "text/html", html);
  });

  // Video stream endpoint
  server.on("/stream", HTTP_GET, handleStream);

  // Capture single photo
  server.on("/capture", HTTP_GET, handleCapture);

  // Status endpoint
  server.on("/status", HTTP_GET, handleStatus);

  // API endpoints
  server.on("/api/status", HTTP_GET, handleApiStatus);
  server.on("/api/quality", HTTP_POST, handleApiQuality);
  server.on("/api/brightness", HTTP_POST, handleApiBrightness);
  server.on("/api/contrast", HTTP_POST, handleApiContrast);
  server.on("/api/flash", HTTP_POST, handleApiFlash);
  server.on("/api/stream/start", HTTP_POST, handleApiStreamStart);
  server.on("/api/stream/stop", HTTP_POST, handleApiStreamStop);

  // 404 handler
  server.onNotFound([]() {
    server.send(404, "text/plain", "Not found");
  });

  Serial.println("Web server endpoints configured");
}

// ============================================================================
// HTTP HANDLERS
// ============================================================================

void handleStream() {
  settings.streaming = true;
  
  WiFiClient client = server.client();
  
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
  client.println();

  Serial.println("Streaming started");

  while (client.connected()) {
    if (settings.flashEnabled) {
      setFlash(true);
    }

    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      break;
    }

    client.printf("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", fb->len);
    client.write(fb->buf, fb->len);
    client.println();

    esp_camera_fb_return(fb);
    
    if (settings.flashEnabled) {
      setFlash(false);
    }
  }

  settings.streaming = false;
  Serial.println("Streaming stopped");
}

void handleCapture() {
  if (settings.flashEnabled) {
    setFlash(true);
    delay(100);
  }

  fb = esp_camera_fb_get();
  if (!fb) {
    server.send(500, "text/plain", "Camera capture failed");
    return;
  }

  server.sendHeader("Content-Disposition", "inline; filename=capture.jpg");
  server.send_P(200, "image/jpeg", (const char *)fb->buf, fb->len);
  
  esp_camera_fb_return(fb);

  if (settings.flashEnabled) {
    setFlash(false);
  }

  Serial.println("Photo captured");
}

void handleStatus() {
  StaticJsonDocument<256> doc;
  doc["device"] = "DrainGuard-CAM";
  doc["wifi_connected"] = WiFi.status() == WL_CONNECTED;
  doc["wifi_rssi"] = WiFi.RSSI();
  doc["ip_address"] = WiFi.localIP().toString();
  doc["streaming"] = settings.streaming;
  doc["quality"] = settings.quality;
  doc["brightness"] = settings.brightness;
  doc["contrast"] = settings.contrast;
  doc["flash_enabled"] = settings.flashEnabled;
  doc["free_heap"] = ESP.getFreeHeap();
  doc["uptime_s"] = millis() / 1000;

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleApiStatus() {
  handleStatus();
}

void handleApiQuality() {
  if (!server.hasArg("value")) {
    server.send(400, "application/json", "{\"error\":\"missing value\"}");
    return;
  }

  int quality = constrain(server.arg("value").toInt(), 0, 3);
  settings.quality = quality;
  applyCameraSettings();

  server.send(200, "application/json", "{\"status\":\"ok\",\"quality\":" + String(quality) + "}");
  Serial.printf("Quality set to: %d\n", quality);
}

void handleApiBrightness() {
  if (!server.hasArg("value")) {
    server.send(400, "application/json", "{\"error\":\"missing value\"}");
    return;
  }

  int brightness = constrain(server.arg("value").toInt(), -2, 2);
  settings.brightness = brightness;
  applyCameraSettings();

  server.send(200, "application/json", "{\"status\":\"ok\",\"brightness\":" + String(brightness) + "}");
  Serial.printf("Brightness set to: %d\n", brightness);
}

void handleApiContrast() {
  if (!server.hasArg("value")) {
    server.send(400, "application/json", "{\"error\":\"missing value\"}");
    return;
  }

  int contrast = constrain(server.arg("value").toInt(), -2, 2);
  settings.contrast = contrast;
  applyCameraSettings();

  server.send(200, "application/json", "{\"status\":\"ok\",\"contrast\":" + String(contrast) + "}");
  Serial.printf("Contrast set to: %d\n", contrast);
}

void handleApiFlash() {
  if (!server.hasArg("enabled")) {
    server.send(400, "application/json", "{\"error\":\"missing enabled\"}");
    return;
  }

  bool enabled = server.arg("enabled") == "true" || server.arg("enabled") == "1";
  settings.flashEnabled = enabled;

  server.send(200, "application/json", "{\"status\":\"ok\",\"flash_enabled\":" + String(enabled ? "true" : "false") + "}");
  Serial.printf("Flash: %s\n", enabled ? "enabled" : "disabled");
}

void handleApiStreamStart() {
  settings.streaming = true;
  server.send(200, "application/json", "{\"status\":\"streaming_started\"}");
  Serial.println("API: Streaming started");
}

void handleApiStreamStop() {
  settings.streaming = false;
  server.send(200, "application/json", "{\"status\":\"streaming_stopped\"}");
  Serial.println("API: Streaming stopped");
}

// ============================================================================
// CAMERA CONTROL
// ============================================================================

void applyCameraSettings() {
  sensor_t * s = esp_camera_sensor_get();
  if (!s) return;

  // Set frame size based on quality
  framesize_t frameSize;
  int jpegQuality;

  switch (settings.quality) {
    case 0: // Low
      frameSize = FRAMESIZE_QVGA;  // 320x240
      jpegQuality = 15;
      break;
    case 1: // Medium (default)
      frameSize = FRAMESIZE_VGA;   // 640x480
      jpegQuality = 12;
      break;
    case 2: // High
      frameSize = FRAMESIZE_SVGA;  // 800x600
      jpegQuality = 10;
      break;
    case 3: // Max
      frameSize = FRAMESIZE_XGA;   // 1024x768
      jpegQuality = 8;
      break;
    default:
      frameSize = FRAMESIZE_VGA;
      jpegQuality = 12;
  }

  s->set_framesize(s, frameSize);
  s->set_quality(s, jpegQuality);

  // Set brightness (-2 to +2)
  s->set_brightness(s, settings.brightness);

  // Set contrast (-2 to +2)
  s->set_contrast(s, settings.contrast);

  Serial.printf("Camera settings applied: Q=%d B=%d C=%d\n",
                settings.quality, settings.brightness, settings.contrast);
}

void setFlash(bool enable) {
  digitalWrite(LED_GPIO_NUM, enable ? HIGH : LOW);
}

// ============================================================================
// END OF FILE
// ============================================================================
