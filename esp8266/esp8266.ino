#include <Wire.h>
#include "DHT.h"
#include "SparkFun_ENS160.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>  // 웹 서버 라이브러리 추가

// 핀 및 센서 타입 정의
#define DHTPIN D6
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
SparkFun_ENS160 myENS;
int CDS_PIN = A0; // 조도 센서 모듈 연결 핀
int ensStatus;

// AP 모드 정보
const char* ap_ssid = "SmartFarm_AP2";  // AP 모드 SSID
const char* ap_password = "123456789";  // AP 모드 비밀번호

// MQTT 브로커 정보
const char* mqtt_server = "weki.jeuke.com";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);
ESP8266WebServer server(80);  // 웹 서버 객체 생성

// UUID 설정
String uuid = "your_unique_id";  // 실제 UUID로 대체 필요

void startAP() {
    WiFi.softAP(ap_ssid, ap_password);  // AP 모드 시작
    Serial.println("AP 모드 시작됨");
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP 주소: ");
    Serial.println(IP);
}

void handleRoot() {
    String html = "<form action=\"/connect\" method=\"POST\">"
                  "SSID: <input type=\"text\" name=\"ssid\"><br>"
                  "Password: <input type=\"text\" name=\"password\"><br>"
                  "<input type=\"submit\" value=\"Connect\">"
                  "</form>";
    server.send(200, "text/html", html);
}

void handleConnect() {
    String user_ssid = server.arg("ssid");
    String user_password = server.arg("password");
    uuid = server.arg("uuid");

    Serial.print("SSID: ");
    Serial.println(user_ssid);
    Serial.print("Password: ");
    Serial.println(user_password);

    // WiFi에 연결 시도
    WiFi.begin(user_ssid.c_str(), user_password.c_str());

    server.send(200, "text/html", "WiFi에 연결 시도 중...");
}

void reconnect() {
    while (!client.connected()) {
        Serial.print("MQTT 서버에 연결 중...");
        if (client.connect("ESP8266Client")) {
            Serial.println("MQTT 서버에 연결됨");
        } else {
            Serial.print("MQTT 서버에 연결 실패, rc=");
            Serial.print(client.state());
            Serial.println(" 다시 시도 중...");
            delay(5000);
        }
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(CDS_PIN, INPUT);  // 조도 센서 핀을 입력으로 설정

    Wire.begin();          // I2C 통신 초기화
    dht.begin();           // DHT 센서 초기화
    delay(1000);           // 센서 초기화 대기

    if (!myENS.begin()) {  // ENS160 초기화 체크
        Serial.println("ENS160 초기화 실패!");
        while (true) {
            delay(1000);  // 1초마다 에러 메시지 출력
            Serial.println("ENS160 초기화 실패, 재부팅 필요");
        }
    }

    myENS.setOperatingMode(SFE_ENS160_STANDARD);  // ENS160 센서 모드 설정
    ensStatus = myENS.getFlags();
    Serial.print("가스 센서 상태 플래그: ");
    Serial.println(ensStatus);

    startAP();  // AP 모드 시작

    server.on("/", handleRoot);  // 루트 핸들러 설정
    server.on("/connect", handleConnect);  // 연결 핸들러 설정
    server.begin();  // 웹 서버 시작
    Serial.println("웹 서버 시작됨");

    client.setServer(mqtt_server, mqtt_port);  // MQTT 서버 설정
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        server.handleClient();  // AP 모드 웹 서버 클라이언트 처리
        return;  // WiFi가 연결되지 않은 경우 데이터 수집 및 전송을 하지 않음
    }

    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    int CDS = analogRead(CDS_PIN);  // 조도 센서 값 읽기
    Serial.print("CDS_Sensor: ");
    Serial.println(CDS);

    float h = dht.readHumidity();  // 습도 읽기
    float t = dht.readTemperature();  // 온도 읽기

    if (isnan(h) || isnan(t)) {
        Serial.println("DHT 센서 읽기 실패!");
    } else {
        Serial.print("Humidity: ");
        Serial.print(h);
        Serial.print("%, Temperature: ");
        Serial.print(t);
        Serial.println("°C");

        // String dhtTopic = uuid + "/dht11";
        // String dhtPayload = "Humidity: " + String(h) + " %, Temperature: " + String(t) + " °C";

        String humidTopic = uuid + "/humid/air";

        client.publish(humidTopic.c_str(), String(h).c_str());

        String TempTopic = uuid + "/temp";

        client.publish(TempTopic.c_str(), String(t).c_str());
        
    }

    if (myENS.checkDataStatus()) {  // ENS160 데이터 확인
        int AQI = myENS.getAQI();
       

        Serial.print("Air Quality Index (1-5): ");
        Serial.println(AQI);

        String ens160Topic = uuid + "/airQuality/sensor";
        // String ens160Payload = "AQI: " + String(AQI) ;
        client.publish(ens160Topic.c_str(), String(AQI).c_str());
    }

    String cdsTopic = uuid + "/light/sensor";
    // String cdsPayload = "CDS_Sensor: " + String(CDS);
    client.publish(cdsTopic.c_str(), String(CDS).c_str());

    delay(2000);  // 데이터 샘플링 주기 (2초)
}

//토픽 통일
//토픽:UUID/dht11
//페이로드: "Humidity: 55.4 %, Temperature: 23.7 °C
//토픽: UUID/ens160
//페이로드: "AQI: 2, TVOC: 400 ppb, CO2: 500 ppm"
//토픽: UUID/cds
//페이로드: "CDS_Sensor: 1023"
//숫자만 보내는 것으로 수정함(토픽)