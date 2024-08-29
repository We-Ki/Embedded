#include <Wire.h>
#include "DHT.h"
#include "SparkFun_ENS160.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// 핀 및 센서 타입 정의
#define DHTPIN D6
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
SparkFun_ENS160 myENS;
int CDS_PIN = A0; // 조도 센서 모듈 연결 핀
int ensStatus;

// WiFi 정보
const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";

// MQTT 브로커 정보
const char* mqtt_server = "weki.jeuke.com"; // 여기에 주소넣으면 됨
const int mqtt_port = 1883; // 일반적으로 사용하는 mqtt 프로토콜 기본 포트 번호(TCP/IP를 통해 MQTT 메세지를 송수신 하는데 사용됨)

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  pinMode(CDS_PIN, INPUT);  // 조도 센서를 입력 핀으로 설정

  Wire.begin();          // I2C 통신 초기화
  Serial.begin(115200);  // 시리얼 통신 초기화

  dht.begin();           // DHT 센서 초기화
  delay(1000);           // 센서 초기화 시간 제공 (DHT11은 전력 공급 후 최소 1초 대기 필요)

  if (!myENS.begin()) {  // ENS160 초기화 체크
    Serial.println("ENS160 initialization failed!");
    while (1); // 오류 시 멈춤
  }

  // ENS160 모드 설정
  if (myENS.setOperatingMode(SFE_ENS160_RESET)) {
    Serial.println("ENS160 is ready.");
  }

  delay(100); // 센서 안정화 대기
  myENS.setOperatingMode(SFE_ENS160_STANDARD);

  // ENS160 상태 플래그 출력
  ensStatus = myENS.getFlags();
  Serial.print("Gas Sensor Status Flag: ");
  Serial.println(ensStatus);

  // WiFi 연결 설정
  setup_wifi();

  // MQTT 브로커 설정
  client.setServer(mqtt_server, mqtt_port);
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // MQTT 서버에 연결
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // 클라이언트 ID로 ESP8266를 사용
    String clientId = "ESP8266Client";
    // 사용자 인증 없이 MQTT 서버에 연결
    if (client.connect(clientId.c_str())) {
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

  // 조도 센서의 측정 값 읽기
  int CDS = analogRead(CDS_PIN);
  Serial.print("CDS_Sensor: ");
  Serial.println(CDS);

  // DHT11 센서로부터 습도 및 온도 읽기
  float h = dht.readHumidity();     // 습도 읽기
  float t = dht.readTemperature();  // 온도 읽기

  // 읽기 오류 확인
  if (isnan(h) || isnan(t)) { // NaN 반환 여부 확인
    Serial.println("Failed to read from DHT sensor!");
  } else {
    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print("%, ");
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.println("°C");

    // MQTT 메시지로 전송 (DHT11 데이터)
    String payload = "Humidity: " + String(h) + " %, Temperature: " + String(t) + " °C";
    client.publish("home/dht11", payload.c_str());
  }

  // ENS160 센서 데이터 상태 확인
  if (myENS.checkDataStatus()) {
    int AQI = myENS.getAQI();
    int TVOC = myENS.getTVOC();
    int CO2 = myENS.getECO2();

    Serial.print("Air Quality Index (1-5): ");
    Serial.println(AQI);

    Serial.print("Total Volatile Organic Compounds: ");
    Serial.print(TVOC);
    Serial.println(" ppb");

    Serial.print("CO2 concentration: ");
    Serial.print(CO2);
    Serial.println(" ppm");

    // MQTT 메시지로 전송 (ENS160 데이터)
    String ens160Payload = "AQI: " + String(AQI) + ", TVOC: " + String(TVOC) + " ppb, CO2: " + String(CO2) + " ppm";
    client.publish("home/ens160", ens160Payload.c_str());
  }

  // MQTT 메시지로 조도 센서 값 전송
  String cdsPayload = "CDS_Sensor: " + String(CDS);
  client.publish("home/cds", cdsPayload.c_str());

  // 모든 데이터 출력 후 2초 대기
  delay(2000); // 지금 센서 데이터를 2초마다 전송하도록 설정해놓음. MQTT 트래픽 고려해서 조정 필요할 수도 있음
}

// DHT11 데이터는 "home/dht11" 토픽으로 전송
// ENS160 데이터는 "home/ens160" 토픽으로 전송
// 조도 센서(CDS) 데이터는 "home/cds" 토픽으로 전송
