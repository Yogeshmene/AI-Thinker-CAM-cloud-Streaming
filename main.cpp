#include <Arduino.h>
#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>

// ===========================
// Configuration Parameters
// ===========================
const char* ssid = "METAYB-IOT";
const char* password = "12345678";

// Cloud Server Configuration
// For quick testing, you can use a free service like https://webhook.site/
// Example: if URL is https://webhook.site/12345-abcde
// serverName = "webhook.site", serverPath = "/12345-abcde"
const char* serverName = "webhook.site"; 
const char* serverPath = "/a54d3537-f957-4cfe-bf1e-1096ccb0493e";
const int serverPort = 443; // 443 for HTTPS, 80 for HTTP

// Capture interval in milliseconds (10 seconds)
const unsigned long captureInterval = 10000; 
unsigned long lastCaptureTime = 0;

// ===========================
// ESP32-CAM (AI Thinker) Pins
// ===========================
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

void initCamera() {
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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // Frame size config based on PSRAM availability
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA; // High resolution
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA; // Lower resolution
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Initialize Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }
  Serial.println("Camera initialized successfully");
}

void sendPhotoToCloud() {
  // Capture photo
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();
  if(!fb) {
    Serial.println("Camera capture failed");
    return;
  }
  Serial.printf("Picture taken! Size: %zu bytes\n", fb->len);

  // Connect to Cloud
  WiFiClientSecure client;
  client.setInsecure(); // Skip certificate validation for generic testing
  
  Serial.println("Connecting to server...");
  if (client.connect(serverName, serverPort)) {
    Serial.println("Connection successful! Uploading...");
    
    // Multipart form data boundaries
    String boundary = "Esp32CamBoundary";
    String head = "--" + boundary + "\r\nContent-Disposition: form-data; name=\"imageFile\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--" + boundary + "--\r\n";

    uint32_t imageLen = fb->len;
    uint32_t extraLen = head.length() + tail.length();
    uint32_t totalLen = imageLen + extraLen;

    // Send HTTP POST request headers
    client.println("POST " + String(serverPath) + " HTTP/1.1");
    client.println("Host: " + String(serverName));
    client.println("Content-Length: " + String(totalLen));
    client.println("Content-Type: multipart/form-data; boundary=" + boundary);
    client.println();
    
    // Send form data header
    client.print(head);

    // Send image buffer in chunks
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n = 0; n < fbLen; n = n + 1024) {
      if (n + 1024 < fbLen) {
        client.write(fbBuf, 1024);
        fbBuf += 1024;
      } else if (fbLen % 1024 > 0) {
        size_t remainder = fbLen % 1024;
        client.write(fbBuf, remainder);
      }
    }
    
    // Send form data tail
    client.print(tail);

    // Read server response
    long timeoutTimer = millis();
    boolean isBody = false;
    String response = "";
    
    while (client.connected() && (millis() - timeoutTimer < 10000)) {
      if (client.available()) {
        char c = client.read();
        response += c;
        timeoutTimer = millis();
      }
    }
    
    Serial.println("--- Server Response ---");
    Serial.println(response);
    Serial.println("-----------------------");
    
  } else {
    Serial.println("Connection to server failed.");
  }
  
  client.stop();
  
  // Return the frame buffer back to the driver for reuse
  esp_camera_fb_return(fb);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nBooting ESP32-CAM...");

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  initCamera();
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Check if 10 seconds have passed
  if (currentMillis - lastCaptureTime >= captureInterval) {
    lastCaptureTime = currentMillis;
    
    if (WiFi.status() == WL_CONNECTED) {
      sendPhotoToCloud();
    } else {
      Serial.println("WiFi disconnected, skipping upload.");
    }
  }
}
