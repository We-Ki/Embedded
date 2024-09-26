#include <WiFi.h>
#include <PubSubClient.h>
#include <WebServer.h>

const char* ap_ssid = "SmartFarm_AP";
const char* ap_password = "12345678";
const char* mqtt_server = "weki.jeuke.com";
const int soilSensorPin = A2;  // 토양 수분 센서가 연결된 아날로그 핀

WebServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);

String uuid = "";  // UUID 저장
String ssid = "";
String password = "";

// 모터 핀 설정
int motorDir1Pin = 10;    //in1
int motorDir2Pin = 13;    //in2
int motorENA = 9;         //ena

int fanDir1Pin = 2;       //in3
int fanDir2Pin = 6;       //in4
int fanENA = 5;           //enb

// 상태 변수
bool waterPumpActive = false;
bool fanActive = false;
unsigned long pumptimerStart = 0;
//const unsigned long waitTime = 600000;  // 10분
bool isPumpWating = false;

// 기준값
int soilThreshold = 530;  // 기준값 예시
int airQualityThreshold = 3; // 팬이 동작할 공기질의 기준값

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
    client.subscribe((uuid + "/Water").c_str());
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

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  String waterTopic = uuid + "/water";

  // UUID/Water 토픽 처리
  if (String(topic) == waterTopic) {
    if (message == "true") {
      waterPumpActive = true;
    } else if (message == "false") {
      waterPumpActive = false;
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32Client")) {
      Serial.println("Reconnected to MQTT server");
      client.subscribe((uuid + "/Water").c_str());
    } else {
      delay(5000);
    }
  }
}

void operatePump() {
  // 워터펌프 동작
  digitalWrite(motorDir1Pin, LOW);
  digitalWrite(motorDir2Pin, HIGH);
  analogWrite(motorENA, 255);  // 워터펌프 동작

  // 워터펌프 동작 상태를 MQTT 서버에 알림
  client.publish((uuid + "/water").c_str(), "true");
}

// 토양 수분 센서 값을 읽어오는 함수
int readSoilMoisture() {
  int moistureLevel = analogRead(soilSensorPin);  // 센서에서 값을 읽어옴
  Serial.print("Soil Moisture Level: ");
  Serial.println(moistureLevel);
  return moistureLevel;
}

// 토양 수분 값을 서버로 전송하는 함수
void sendSoilMoistureToServer(int moistureLevel) {
  String topic = uuid + "/humid/soil";
  String payload = String(moistureLevel);
  client.publish(topic.c_str(), payload.c_str());
  Serial.print("Soil moisture level sent to server: ");
  Serial.println(payload);
}

void setup() {
  Serial.begin(115200);
  pinMode(soilSensorPin, INPUT);  // 토양 수분 센서 핀을 입력으로 설정
  pinMode(motorDir1Pin, OUTPUT);
  pinMode(motorDir2Pin, OUTPUT);
  pinMode(motorENA, OUTPUT);

  startAP();

  server.on("/", handleRoot);
  server.on("/connect", handleConnect);
  server.begin();
  Serial.println("Web server started");

  client.setCallback(callback);
}

void loop() {
  server.handleClient();

  if (WiFi.status() == WL_CONNECTED && !client.connected()) {
    reconnect();
  }

  if (WiFi.status() == WL_CONNECTED && client.connected()) {
    // 토양 수분 센서 값 읽기
    int soilMoisture = readSoilMoisture();  // 센서 값 읽기 및 출력

    // 토양 수분 값을 MQTT 서버로 전송
    sendSoilMoistureToServer(soilMoisture);

    // 토양 수분 값이 기준값 이상이면 워터펌프 동작
    if (soilMoisture >= soilThreshold) {
      operatePump();  // 워터펌프 동작
      Serial.println("Water pump activated.");  // 워터펌프 동작 상태 출력
    } else {
      Serial.println("Water pump not activated.");  // 워터펌프 미작동 상태 출력
    }
  }

  client.loop();
  delay(1000);  // 데이터 샘플링 주기
}