/*
 * ESP32-CAM Firmware — DrainGuard
 *
 * Network topology:
 *   ESP32 DevKit  →  creates hotspot "DrainGuard-Robot" (192.168.4.1)
 *   ESP32-CAM     →  joins that hotspot with STATIC IP  192.168.4.50
 *   Phone / Web   →  also joins that hotspot, opens http://192.168.4.50/stream
 *
 * Static IP is essential so the DevKit always knows where to find the camera
 * regardless of DHCP lease order.
 *
 * Endpoints:
 *   GET /          — simple HTML page with embedded stream
 *   GET /stream    — MJPEG live stream (multipart/x-mixed-replace)
 *   GET /capture   — single JPEG snapshot
 *   GET /status    — JSON health check
 */

#include "esp_camera.h"
#include "esp_timer.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// ============================================================================
// HOTSPOT CREDENTIALS  —  must match DrainGuard.ino / config.h
// ============================================================================
static const char *HOTSPOT_SSID     = "DrainGuard-Robot";
static const char *HOTSPOT_PASSWORD = "DrainGuard123";

// Static IP assigned to this ESP32-CAM on the DevKit hotspot
static const IPAddress CAM_IP      (192, 168, 4, 50);
static const IPAddress GATEWAY_IP  (192, 168, 4,  1);   // DevKit hotspot IP
static const IPAddress SUBNET_MASK (255, 255, 255, 0);

// How long to wait for WiFi before rebooting (ms)
#define WIFI_TIMEOUT_MS   30000
// How long between reconnect attempts when WiFi drops (ms)
#define RECONNECT_DELAY_MS 5000

// ============================================================================
// CAMERA PINS  —  AI-Thinker ESP32-CAM
// ============================================================================
#define PWDN_GPIO_NUM   32
#define RESET_GPIO_NUM  -1
#define XCLK_GPIO_NUM    0
#define SIOD_GPIO_NUM   26
#define SIOC_GPIO_NUM   27
#define Y9_GPIO_NUM     35
#define Y8_GPIO_NUM     34
#define Y7_GPIO_NUM     39
#define Y6_GPIO_NUM     36
#define Y5_GPIO_NUM     21
#define Y4_GPIO_NUM     19
#define Y3_GPIO_NUM     18
#define Y2_GPIO_NUM      5
#define VSYNC_GPIO_NUM  25
#define HREF_GPIO_NUM   23
#define PCLK_GPIO_NUM   22

// Built-in flash LED on AI-Thinker board
#define FLASH_LED_PIN   4

// ============================================================================
// GLOBALS
// ============================================================================
WebServer server(80);
bool      cameraOk        = false;
unsigned long lastReconnectAttempt = 0;

// ============================================================================
// CAMERA INIT
// ============================================================================
bool initCamera() {
  camera_config_t cfg;
  cfg.ledc_channel  = LEDC_CHANNEL_0;
  cfg.ledc_timer    = LEDC_TIMER_0;
  cfg.pin_d0        = Y2_GPIO_NUM;
  cfg.pin_d1        = Y3_GPIO_NUM;
  cfg.pin_d2        = Y4_GPIO_NUM;
  cfg.pin_d3        = Y5_GPIO_NUM;
  cfg.pin_d4        = Y6_GPIO_NUM;
  cfg.pin_d5        = Y7_GPIO_NUM;
  cfg.pin_d6        = Y8_GPIO_NUM;
  cfg.pin_d7        = Y9_GPIO_NUM;
  cfg.pin_xclk      = XCLK_GPIO_NUM;
  cfg.pin_pclk      = PCLK_GPIO_NUM;
  cfg.pin_vsync     = VSYNC_GPIO_NUM;
  cfg.pin_href      = HREF_GPIO_NUM;
  cfg.pin_sscb_sda  = SIOD_GPIO_NUM;
  cfg.pin_sscb_scl  = SIOC_GPIO_NUM;
  cfg.pin_pwdn      = PWDN_GPIO_NUM;
  cfg.pin_reset     = RESET_GPIO_NUM;
  cfg.xclk_freq_hz  = 20000000;
  cfg.pixel_format  = PIXFORMAT_JPEG;

  if (psramFound()) {
    // PSRAM available — higher resolution, two frame buffers for smoother stream
    cfg.frame_size   = FRAMESIZE_VGA;   // 640x480 — good balance for streaming
    cfg.jpeg_quality = 12;              // lower = better quality (range 0-63)
    cfg.fb_count     = 2;
  } else {
    // No PSRAM — keep it small to avoid OOM crashes
    cfg.frame_size   = FRAMESIZE_QVGA;  // 320x240
    cfg.jpeg_quality = 15;
    cfg.fb_count     = 1;
  }

  esp_err_t err = esp_camera_init(&cfg);
  if (err != ESP_OK) {
    Serial.printf("[CAM] Init failed: 0x%x\n", err);
    return false;
  }

  // Fine-tune sensor
  sensor_t *s = esp_camera_sensor_get();
  if (s != nullptr) {
    s->set_brightness(s,  0);
    s->set_contrast(s,    0);
    s->set_saturation(s,  0);
    s->set_whitebal(s,    1);   // auto white balance
    s->set_exposure_ctrl(s, 1); // auto exposure
    s->set_gain_ctrl(s,   1);   // auto gain
    s->set_vflip(s,       0);   // flip vertically if camera is mounted upside-down
    s->set_hmirror(s,     0);   // mirror horizontally if needed
  }

  Serial.println("[CAM] Initialized OK");
  return true;
}

// ============================================================================
// WIFI — connect to ESP32 DevKit hotspot with static IP
// ============================================================================
void connectToHotspot() {
  Serial.printf("[WiFi] Connecting to hotspot: %s\n", HOTSPOT_SSID);

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);

  // Assign static IP so DevKit always finds us at 192.168.4.50
  if (!WiFi.config(CAM_IP, GATEWAY_IP, SUBNET_MASK)) {
    Serial.println("[WiFi] Static IP config failed — will use DHCP");
  }

  WiFi.setAutoReconnect(true);
  WiFi.begin(HOTSPOT_SSID, HOTSPOT_PASSWORD);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > WIFI_TIMEOUT_MS) {
      Serial.println("[WiFi] Timeout — rebooting");
      ESP.restart();
    }
    delay(250);
    Serial.print(".");
  }

  Serial.printf("\n[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("[Stream] http://%s/stream\n", WiFi.localIP().toString().c_str());
}

// ============================================================================
// HTTP HANDLERS
// ============================================================================

void handleRoot() {
  String html = R"html(
<!DOCTYPE html><html><head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>DrainGuard Camera</title>
<style>
  body { margin:0; background:#000; display:flex; flex-direction:column;
         align-items:center; justify-content:center; min-height:100vh; }
  img  { max-width:100%; border:2px solid #2196F3; border-radius:4px; }
  h2   { color:#fff; font-family:sans-serif; }
  a    { color:#2196F3; }
</style>
</head><body>
<h2>&#128247; DrainGuard Live Feed</h2>
<img src="/stream" alt="Loading stream...">
<p><a href="/capture">&#128248; Capture Snapshot</a></p>
</body></html>
)html";
  server.send(200, "text/html", html);
}

// MJPEG multipart stream — each frame is a JPEG pushed continuously
void handleStream() {
  WiFiClient client = server.client();

  // Send multipart HTTP header
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Cache-Control: no-cache, no-store, must-revalidate");
  client.println("Connection: close");
  client.println();

  Serial.println("[Stream] Client connected");

  uint32_t frameCount = 0;
  unsigned long streamStart = millis();

  while (client.connected()) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("[Stream] Frame capture failed — skipping");
      delay(100);
      continue;
    }

    // Write MIME boundary + frame
    client.printf("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", fb->len);
    size_t written = client.write(fb->buf, fb->len);
    client.print("\r\n");

    esp_camera_fb_return(fb);

    if (written == 0) {
      // Client stopped reading — exit cleanly
      break;
    }

    frameCount++;

    // Print FPS every 100 frames
    if (frameCount % 100 == 0) {
      float elapsed = (millis() - streamStart) / 1000.0f;
      Serial.printf("[Stream] %u frames in %.1f s (%.1f FPS)\n",
                    frameCount, elapsed, frameCount / elapsed);
    }

    delay(33);   // ~30 FPS cap — adjust if stream is too slow/fast
  }

  Serial.printf("[Stream] Client disconnected after %u frames\n", frameCount);
}

// Single JPEG snapshot
void handleCapture() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    server.send(500, "text/plain", "Camera capture failed");
    return;
  }

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Content-Disposition", "inline; filename=capture.jpg");
  server.send_P(200, "image/jpeg", (const char *)fb->buf, fb->len);
  esp_camera_fb_return(fb);
  Serial.println("[CAM] Snapshot sent");
}

// JSON health/status endpoint — DevKit polls this to verify camera is alive
void handleStatus() {
  server.sendHeader("Access-Control-Allow-Origin", "*");

  StaticJsonDocument<192> doc;
  doc["status"]      = cameraOk ? "ok" : "camera_error";
  doc["ip"]          = WiFi.localIP().toString();
  doc["rssi"]        = WiFi.RSSI();
  doc["stream_url"]  = "http://" + WiFi.localIP().toString() + "/stream";
  doc["capture_url"] = "http://" + WiFi.localIP().toString() + "/capture";
  doc["heap_free"]   = (int)ESP.getFreeHeap();
  doc["uptime_s"]    = (int)(millis() / 1000);

  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

// ============================================================================
// SETUP
// ============================================================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== ESP32-CAM DrainGuard ===");

  // Turn off flash LED (GPIO 4 is shared with camera data on some boards)
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);

  // Init camera first — WiFi init can interfere if done before
  cameraOk = initCamera();
  if (!cameraOk) {
    Serial.println("[CAM] Camera init failed! Rebooting in 5s...");
    delay(5000);
    ESP.restart();
  }

  // Connect to ESP32 DevKit hotspot
  connectToHotspot();

  // Register HTTP routes
  server.on("/",        HTTP_GET, handleRoot);
  server.on("/stream",  HTTP_GET, handleStream);
  server.on("/capture", HTTP_GET, handleCapture);
  server.on("/status",  HTTP_GET, handleStatus);

  server.begin();
  Serial.println("[HTTP] Server started");
}

// ============================================================================
// LOOP
// ============================================================================
void loop() {
  // Handle incoming HTTP requests
  server.handleClient();

  // Auto-reconnect if WiFi drops (e.g., DevKit reboots)
  if (WiFi.status() != WL_CONNECTED) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt >= RECONNECT_DELAY_MS) {
      lastReconnectAttempt = now;
      Serial.println("[WiFi] Lost connection — reconnecting...");
      WiFi.disconnect();
      WiFi.begin(HOTSPOT_SSID, HOTSPOT_PASSWORD);
    }
  }
}
