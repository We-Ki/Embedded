#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>

const char* ap_ssid = "SmartFarm_AP";
const char* ap_password = "12345678";
const char* mqtt_server = "mqtt.example.com"; // 사용 중인 MQTT 서버 주소로 변경

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

void startStation(const char* ssid, const char* password) {
  WiFi.mode(WIFI_STA);  // STATION 모드 설정
  WiFi.begin(ssid, password);

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - startTime > 30000) {  // 30초 동안 연결 시도
      Serial.println("\nConnection failed, switching back to AP mode.");
      startAP();
      return;
    }
  }

  Serial.println("\nConnected to WiFi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // MQTT 서버 연결 시도
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

  // STATION 모드로 전환
  startStation(ssid.c_str(), password.c_str());
}

void setup() {
  Serial.begin(115200);

  // AP 모드 시작
  startAP();

  // 웹 서버 핸들러 설정
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

  client.loop();
}
