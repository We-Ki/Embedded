#include <WiFi.h>
#include "PubSubClient.h"

WebServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);

String uuid = "";
String ssid = "";
String password = "";

void startAP();
void startStation(const char* ssid, const char* password);

void handleRoot();
void handleConnect();