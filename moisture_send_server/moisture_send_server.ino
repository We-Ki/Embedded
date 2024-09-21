#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "HY-DORM5-5F 휴게실";          // Wi-Fi 이름
const char* password = "residence";  // Wi-Fi 비밀번호
const char* mqtt_server = "weki.jeuke.com"; // MQTT 서버 주소

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);  // 시리얼 통신 속도 설정
  setup_wifi();          // Wi-Fi 연결 설정
  client.setServer(mqtt_server, 1883); // MQTT 서버 설정
}

void setup_wifi() {
  delay(10);
  // Wi-Fi 연결 시도
  Serial.println("Connecting to WiFi..");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // MQTT 서버에 연결 시도
    if (client.connect("DFR0478Client")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  int soilMoistureValue = analogRead(A2);  // A0 핀에서 토양 수분 값 읽기
  char msg[50];
  sprintf(msg, "Moisture Level: %d", soilMoistureValue);
  
  // MQTT를 통해 메시지 전송
  client.publish("soilMoisture/topic", msg);
  
  // 시리얼 모니터에 값 출력
  Serial.println(msg);

  delay(2000);  // 2초 간격으로 데이터 전송 및 출력
}

