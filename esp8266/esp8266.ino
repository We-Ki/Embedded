#include <Wire.h>
#include "DHT.h"
#include "SparkFun_ENS160.h"  // ENS160 라이브러리 주석 처리
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>  // 웹 서버 라이브러리 추가

// 핀 및 센서 타입 정의
#define DHTPIN D6
#define DHTTYPE DHT22
#define LED_PIN D7  // LED 핀 정의 (GPIO 13)

DHT dht(DHTPIN, DHTTYPE);
SparkFun_ENS160 myENS;  // ENS160 객체 주석 처리
int CDS_PIN = A0; // 조도 센서 모듈 연결 핀
int ensStatus;  // ENS160 상태 변수 주석 처리
int ensErrorCount = 0;  // ENS160 오류 발생 횟수 주석 처리
const int maxEnsErrorCount = 5;  // 최대 허용 오류 횟수 주석 처리

// AP 모드 정보
const char* ap_ssid = "SmartFarm_AP2";  // AP 모드 SSID
const char* ap_password = "12345678";  // AP 모드 비밀번호 동일하게 수정완료

// MQTT 브로커 정보
const char* mqtt_server = "weki.jeuke.com";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);
ESP8266WebServer server(80);  // 웹 서버 객체 생성

// UUID 설정
String uuid = "your_unique_id";  // 실제 UUID로 대체 필요

// LED 밝기 단계 정의
int ledBrightness[2] = {51, 255};  // 1단계: 102, 2단계: 204
int previousBrightnessLevel = -1;  // 이a전 밝기 단계 기록 변수

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

// LED 밝기 설정 함수 (CDS 값에 따라 2단계로 설정)
// LED 밝기 설정 함수 (CDS 값에 따라 2단계로 설정)
void setLEDBrightness(int CDS) {
    int brightnessLevel = 0;  // 초기화

    // CDS 값에 따라 LED 밝기 설정
    if (CDS >= 600) {  // 어두운 상태
        brightnessLevel = 2;  // 2단계 밝기
    } else if (CDS >= 400) {  // 약간 어두운 상태
        brightnessLevel = 1;  // 1단계 밝기
    } else {
        brightnessLevel = 0;  // LED 끔 (매우 밝은 상태)
    }

    // 현재 밝기와 이전 밝기가 다를 때만 변경
    if (brightnessLevel != previousBrightnessLevel) {
        if (brightnessLevel == 1) {
            analogWrite(LED_PIN, ledBrightness[0]);  // 1단계 밝기 설정
        } else if (brightnessLevel == 2) {
            analogWrite(LED_PIN, ledBrightness[1]);  // 2단계 밝기 설정
        } else {
            analogWrite(LED_PIN, 0);  // LED 끄기
        }

        Serial.print("LED 밝기 단계: ");
        Serial.println(brightnessLevel);

        // MQTT로 LED 상태 전송 (밝기 단계 전송)
        String ledStateTopic = uuid + "/light/state";
        client.publish(ledStateTopic.c_str(), String(brightnessLevel).c_str());  // 밝기 단계를 문자열로 전송

        // 이전 밝기 단계 업데이트
        previousBrightnessLevel = brightnessLevel;
    }
}


void setup() {
    Serial.begin(115200);
    pinMode(CDS_PIN, INPUT);  // 조도 센서 핀을 입력으로 설정
    pinMode(LED_PIN, OUTPUT);  // LED 핀을 출력으로 설정

    Wire.begin();          // I2C 통신 초기화
    dht.begin();           // DHT 센서 초기화
    delay(1000);           // 센서 초기화 대기

    // ENS160 관련 코드 주석 처리
    if (!myENS.begin()) {
        Serial.println("ENS160 초기화 실패!");
        while (true) {
            delay(1000);  // 1초마다 에러 메시지 출력
            Serial.println("ENS160 초기화 실패, 재부팅 필요");
        }
    }

    myENS.setOperatingMode(SFE_ENS160_STANDARD);  // ENS160 센서 모드 설정
    ensStatus = myENS.getFlags();
    if (ensStatus != 0) {
        Serial.print("ENS160 센서 오류 발생, 플래그 값: ");
        Serial.println(ensStatus);
    }

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
        Serial.println("DHT 센서 읽기 실패! 다시 시도합니다.");
        delay(1000);  // 1초 대기 후 다시 시도
    } else {
        Serial.print("Humidity: ");
        Serial.print(h);
        int inth = h*10;
        Serial.print("%, Temperature: ");
        Serial.print(t);
        Serial.println("°C");

        String humidTopic = uuid + "/humid/air";
        client.publish(humidTopic.c_str(), String(inth).c_str());

        String tempTopic = uuid + "/temp";
        client.publish(tempTopic.c_str(), String(t).c_str());
    }



        if (myENS.checkDataStatus()) {
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