#include <Wire.h>
#include "DHT.h"
#include "SparkFun_ENS160.h"
#include "SparkFun_ENS160.h"  // ENS160 라이브러리 주석 처리
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>  // 웹 서버 라이브러리 추가
#define LED_PIN D7  // LED 핀 정의 (GPIO 13)

DHT dht(DHTPIN, DHTTYPE);
SparkFun_ENS160 myENS;
SparkFun_ENS160 myENS;  // ENS160 객체 주석 처리
int CDS_PIN = A0; // 조도 센서 모듈 연결 핀
int ensStatus;
int ensErrorCount = 0;  // ENS160 오류 발생 횟수
const int maxEnsErrorCount = 5;  // 최대 허용 오류 횟수
int ensStatus;  // ENS160 상태 변수 주석 처리
int ensErrorCount = 0;  // ENS160 오류 발생 횟수 주석 처리
const int maxEnsErrorCount = 5;  // 최대 허용 오류 횟수 주석 처리

// AP 모드 정보
const char* ap_ssid = "SmartFarm_AP2";  // AP 모드 SSID
    }
}

// LED 밝기 설정 함수 (CDS 값에 따라 2단계로 설정)
// LED 밝기 설정 함수 (CDS 값에 따라 2단계로 설정)
void setLEDBrightness(int CDS) {
    int brightnessLevel = 0;  // 초기화
        Serial.print("LED 밝기 단계: ");
        Serial.println(brightnessLevel);

        // MQTT로 LED 상태 전송
        // MQTT로 LED 상태 전송 (밝기 단계 전송)
        String ledStateTopic = uuid + "/light/state";
        client.publish(ledStateTopic.c_str(), brightnessLevel > 0 ? "true" : "false");
        client.publish(ledStateTopic.c_str(), String(brightnessLevel).c_str());  // 밝기 단계를 문자열로 전송

        // 이전 밝기 단계 업데이트
        previousBrightnessLevel = brightnessLevel;
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(CDS_PIN, INPUT);  // 조도 센서 핀을 입력으로 설정
    dht.begin();           // DHT 센서 초기화
    delay(1000);           // 센서 초기화 대기

    // ENS160 초기화 및 상태 확인
    if (!myENS.begin()) {
        Serial.println("ENS160 초기화 실패!");
        while (true) {
            delay(1000);  // 1초마다 에러 메시지 출력
            Serial.println("ENS160 초기화 실패, 재부팅 필요");
        }
    }
    myENS.setOperatingMode(SFE_ENS160_STANDARD);  // ENS160 센서 모드 설정
    ensStatus = myENS.getFlags();
    if (ensStatus != 0) {  // ENS160 오류 처리
        Serial.print("ENS160 센서 오류 발생, 플래그 값: ");
        Serial.println(ensStatus);
    }
   
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

    server.begin();  // 웹 서버 시작
    Serial.println("웹 서버 시작됨");

    // MQTT 연결 여부 확인 후 LED 상태 전송
    if (!client.connected()) {
        reconnect();
    }
    String ledStateTopic = uuid + "/light/state";
    client.publish(ledStateTopic.c_str(), "false");  // LED가 꺼진 상태로 초기화
}

void loop() {
        client.publish(tempTopic.c_str(), String(t).c_str());
    }

    // ENS160 센서 오류 처리 및 상태 확인
    ensStatus = myENS.getFlags();
    if (ensStatus != 0) {
        Serial.print("ENS160 센서 오류 발생, 플래그 값: ");
        Serial.println(ensStatus);
        ensErrorCount++;  // 오류 발생 횟수 증가
        // 오류 횟수가 최대치를 넘으면 재부팅
        if (ensErrorCount >= maxEnsErrorCount) {
            Serial.println("ENS160 오류 횟수 초과, 시스템 재부팅...");
            ESP.restart();  // 시스템 재부팅
        }
    } else {
        // 오류가 없으면 오류 카운트 초기화
        ensErrorCount = 0;
        if (myENS.checkDataStatus()) {  // ENS160 데이터 확인
            int AQI = myENS.getAQI();
            Serial.print("Air Quality Index (1-5): ");
            Serial.println(AQI);
            String ens160Topic = uuid + "/airQuality/sensor";
            client.publish(ens160Topic.c_str(), String(AQI).c_str());
        }
    }
     ENS160 관련 코드 주석 처리
     ensStatus = myENS.getFlags();
     if (ensStatus != 0) {
         Serial.print("ENS160 센서 오류 발생, 플래그 값: ");
         Serial.println(ensStatus);
         ensErrorCount++;  // 오류 발생 횟수 증가
         if (ensErrorCount >= maxEnsErrorCount) {
             Serial.println("ENS160 오류 횟수 초과, 시스템 재부팅...");
             ESP.restart();  // 시스템 재부팅
         }
     } else {
         ensErrorCount = 0;
         if (myENS.checkDataStatus()) {
             int AQI = myENS.getAQI();
             Serial.print("Air Quality Index (1-5): ");
             Serial.println(AQI);
             String ens160Topic = uuid + "/airQuality/sensor";
             client.publish(ens160Topic.c_str(), String(AQI).c_str());
         }
     }

    String cdsTopic = uuid + "/light/sensor";
    client.publish(cdsTopic.c_str(), String(CDS).c_str());