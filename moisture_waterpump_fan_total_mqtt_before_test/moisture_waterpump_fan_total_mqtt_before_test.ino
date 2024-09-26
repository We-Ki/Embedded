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
int Dir1Pin_B = 10;
int Dir2Pin_B = 13;
int SpeedPin_B = 9;

// 팬 제어 핀 설정 (예시로 11번 핀 사용)
int fanPin = 11;

// 상태 변수
bool waterPumpActive = false;
bool fanActive = false;
unsigned long pumptimerStart = 0;
const unsigned long waitTime = 600000;  // 10분?

// 기준값
int soilThreshold = 535;  // 기준값 예시
int airQualityThreshold = 3;  // 팬이 동작할 공기질 기준값

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
    client.subscribe((uuid + "/airquality").c_str());  // airquality 토픽 구독
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

  String waterTopic = uuid + "/Water";
  String airQualityTopic = uuid + "/airquality";

  // UUID/Water 토픽 처리
  if (String(topic) == waterTopic) {
    if (message == "true") {
      waterPumpActive = true;
    } else if (message == "false") {
      waterPumpActive = false;
    }
  }

  // UUID/airquality 토픽 처리 (공기질 데이터)
  if (String(topic) == airQualityTopic) {
    int airQualityValue = message.toInt();
    if (airQualityValue >= airQualityThreshold) {
      fanActive = true;
      Serial.println("Fan activated due to air quality.");
      digitalWrite(fanPin, HIGH);  // 팬 켜기
    } else {
      fanActive = false;
      Serial.println("Fan deactivated.");
      digitalWrite(fanPin, LOW);  // 팬 끄기
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32Client")) {
      Serial.println("Reconnected to MQTT server");
      client.subscribe((uuid + "/Water").c_str());
      client.subscribe((uuid + "/airquality").c_str());  // airquality 토픽 구독
    } else {
      delay(5000);
    }
  }
}

void operatePump() {
  // 워터펌프 동작
  digitalWrite(Dir1Pin_B, LOW);
  digitalWrite(Dir2Pin_B, HIGH);
  analogWrite(SpeedPin_B, 255);  // 워터펌프 동작

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
  pinMode(Dir1Pin_B, OUTPUT);
  pinMode(Dir2Pin_B, OUTPUT);
  pinMode(SpeedPin_B, OUTPUT);
  
  // 팬 제어 핀 설정
  pinMode(fanPin, OUTPUT);
  digitalWrite(fanPin, LOW);  // 팬 초기 상태는 끔

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
