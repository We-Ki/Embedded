#include <Wire.h>
#include "DHT.h"
#include "SparkFun_ENS160.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>  // 웹 서버 라이브러리 추가

// 핀 및 센서 타입 정의
#define DHTPIN D6
#define DHTTYPE DHT11
#define LED_PIN D7  // LED 핀 정의 (GPIO 13)

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

// LED 밝기 단계 정의 (1단계: 51, 2단계: 102, ..., 4단계: 255)
int ledBrightness[4] = {51, 102, 153, 204};  // 5단계 제거

void startAP() {
    WiFi.softAP(ap_ssid, ap_password);  // AP 모드 시작
    Serial.println("AP 모드 시작됨");
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP 주소: ");
    Serial.println(IP);
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

    client.setServer(mqtt_server, mqtt_port);  // MQTT 서버 설정
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

// LED 밝기 설정 함수 (CDS 값에 따라 4단계로 설정)
void setLEDBrightness(int CDS) {
    int brightnessLevel = 0;  // 초기화

    if (CDS <= 204) {
        brightnessLevel = 0;  // LED 끔
    } else if (CDS <= 409) {
        brightnessLevel = 1;  // 1단계
    } else if (CDS <= 614) {
        brightnessLevel = 2;  // 2단계
    } else if (CDS <= 819) {
        brightnessLevel = 3;  // 3단계
    } else {
        brightnessLevel = 4;  // 4단계
    }

    if (brightnessLevel > 0) {
        analogWrite(LED_PIN, ledBrightness[brightnessLevel - 1]);  // LED 밝기 설정
    } else {
        analogWrite(LED_PIN, 0);  // LED 끄기
    }

    Serial.print("LED 밝기 단계: ");
    Serial.println(brightnessLevel);

    // LED 상태를 MQTT로 전송
    String ledStateTopic = uuid + "/light/state";
    client.publish(ledStateTopic.c_str(), brightnessLevel > 0 ? "true" : "false");
}

void setup() {
    Serial.begin(115200);
    pinMode(CDS_PIN, INPUT);  // 조도 센서 핀을 입력으로 설정
    pinMode(LED_PIN, OUTPUT);  // LED 핀을 출력으로 설정

    // 초기 LED 상태 전송
    String ledStateTopic = uuid + "/light/state";
    client.publish(ledStateTopic.c_str(), "false");  // LED가 꺼진 상태로 초기화

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

    // LED 밝기 조절 함수 호출
    setLEDBrightness(CDS);

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

        String humidTopic = uuid + "/humid/air";
        client.publish(humidTopic.c_str(), String(h).c_str());

        String tempTopic = uuid + "/temp";
        client.publish(tempTopic.c_str(), String(t).c_str());
    }

    if (myENS.checkDataStatus()) {  // ENS160 데이터 확인
        int AQI = myENS.getAQI();
        Serial.print("Air Quality Index (1-5): ");
        Serial.println(AQI);

        String ens160Topic = uuid + "/airQuality/sensor";
        client.publish(ens160Topic.c_str(), String(AQI).c_str());
    }

    String cdsTopic = uuid + "/light/sensor";
    client.publish(cdsTopic.c_str(), String(CDS).c_str());

    delay(2000);  // 데이터 샘플링 주기 (2초)
}
