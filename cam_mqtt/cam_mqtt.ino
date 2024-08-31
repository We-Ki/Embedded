#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <esp_camera.h>
#include <Arduino.h>
#include <base64.h>

// 카메라 핀 설정
#define CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM       5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

const char* ap_ssid = "SmartFarm_AP";
const char* ap_password = "12345678";
const char* mqtt_server = "weki.jeuke.com";

WebServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);

String uuid = "";
String ssid = "";
String password = "";

void startAP() {
  WiFi.softAP(ap_ssid, ap_password);
  Serial.println("AP mode started");
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
}

void stopAP() {
  WiFi.softAPdisconnect(true);
  Serial.println("AP mode stopped");
}

void startStation(const char* ssid, const char* password) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - startTime > 30000) {
      Serial.println("\nConnection failed, switching back to AP mode.");
      startAP();
      return;
    }
  }
  Serial.println("\nConnected to WiFi, stopping AP mode.");
  stopAP();
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  client.setServer(mqtt_server, 1883);
  if (client.connect("ESP32Client")) {
    Serial.println("MQTT connected");
  } else {
    Serial.println("MQTT connection failed, switching back to AP mode.");
    startAP();
  }
}

void handleRoot() {
  String html = "<form action=\"/connect\" method=\"POST\">"
                "UUID: <input type=\"text\" name=\"uuid\"><br>"
                "SSID: <input type=\"text\" name=\"ssid\"><br>"
                "Password: <input type=\"text\" name=\"password\"><br>"
                "<input type=\"submit\" value=\"Connect\">"
                "</form>";
  server.send(200, "text/html", html);
}

void handleConnect() {
  uuid = server.arg("uuid");
  ssid = server.arg("ssid");
  password = server.arg("password");
  Serial.print("UUID: ");
  Serial.println(uuid);
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);
  server.send(200, "text/html", "Connecting to WiFi...");
  startStation(ssid.c_str(), password.c_str());
}

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
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 10;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
}

void captureAndSendImage() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }
  String imageData = base64::encode(fb->buf, fb->len);
  Serial.println("Publishing image...");
  Serial.println("Image Data (truncated): \n" + imageData.substring(0, 60));
  String topic = uuid + "/image";
  client.setBufferSize(2048);
  if (!client.publish(topic.c_str(), imageData.substring(0, 60).c_str()/*imageData.c_str()*/)) {
    Serial.println("Failed to publish image data");
  }
  esp_camera_fb_return(fb);
}

void setup() {
  Serial.begin(115200);
  setupCamera();
  startAP();
  server.on("/", handleRoot);
  server.on("/connect", handleConnect);
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  server.handleClient();
  if (WiFi.status() == WL_CONNECTED && !client.connected()) {
    Serial.println("Reconnecting to MQTT server...");
    if (client.connect("ESP32Client")) {
      Serial.println("Reconnected to MQTT server");
    } else {
      Serial.println("Failed to reconnect to MQTT server");
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
    captureAndSendImage();
  }
  client.loop();
  delay(5000);
}
