#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>

const char* ap_ssid = "SmartFarm_AP";
const char* ap_password = "12345678";
const char* mqtt_server = "weki.jeuke.com";
const int soilSensorPin = A2;  // 토양 수분 센서가 연결된 아날로그 핀

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

void readAndSendSoilMoisture() {
  int moistureLevel = analogRead(soilSensorPin);
  char msg[50];
  sprintf(msg, "%d", moistureLevel);
  
  Serial.println(msg);  // 시리얼 모니터에 출력
  String topic = uuid + "/soilMoisture";
  client.publish(topic.c_str(), msg);  // MQTT 서버로 데이터 전송
}

void setup() {
  Serial.begin(115200);
  pinMode(soilSensorPin, INPUT);  // 토양 수분 센서 핀을 입력으로 설정
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
    readAndSendSoilMoisture();  // 토양 수분 데이터 읽기 및 전송
  }

  client.loop();
  delay(2000);  // 데이터 샘플링 주기
}
