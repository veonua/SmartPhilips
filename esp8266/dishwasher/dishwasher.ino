#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <SoftwareSerial.h>
#include <iostream>  
#include <string>

#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#define WIFI_SSID "TP-Link_D5BA"
#define WIFI_PASS "wifipassword654"


const char* ssid = WIFI_SSID;
const char* password = WIFI_PASS;
const char* mqtt_server = "192.168.0.224";

SoftwareSerial swSer (D2, D5); // RX, TX

#define debugSerial Serial1

ESP8266WebServer server(80);
const byte buff_size = 255;
byte debugIdx = 0;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(4800);
  swSer.begin(4800);

  debugSerial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  ///
  ArduinoOTA.onStart([]() {
      debugSerial.println("Start OTA");
	});

	ArduinoOTA.onEnd([]() {
	  debugSerial.println("End OTA");
	});

	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
	  debugSerial.printf("Progress: %u%%\n", (progress / (total / 100)));
	});

	ArduinoOTA.onError([](ota_error_t error) {
	debugSerial.printf("Error[%u]: ", error);

	switch (error) {
	  case OTA_AUTH_ERROR:
	    debugSerial.println("Auth Failed");
	    break;
	  case OTA_BEGIN_ERROR:
	    debugSerial.println("Begin Failed");
	    break;
	  case OTA_CONNECT_ERROR:
	    debugSerial.println("Connect Failed");
	    break;
	  case OTA_RECEIVE_ERROR:
	    debugSerial.println("Receive Failed");
	    break;
	  case OTA_END_ERROR:
	    debugSerial.println("End Failed");
	    break;
	  }
	});

	ArduinoOTA.begin();
///
  if (MDNS.begin("dishwasher")) {
    debugSerial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/state", apiStatus);
  server.onNotFound(handleNotFound);
  server.begin();
}

void setup_wifi() {
  delay(10);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void callback(char* topic, byte* hex, unsigned int length) {
  for (int i = 0; i < length/2; i++) {
    char c = (hex[i * 2] >= 'A' ? ((hex[i * 2] & 0xdf) - 'A') + 10 : (hex[i * 2] - '0'));
    c = (c << 4) | (hex[i * 2 + 1] >= 'A' ? ((hex[i * 2 + 1] & 0xdf) - 'A') + 10 : (hex[i * 2 + 1] - '0'));
    Serial.print(c); // (char)payload[i]
  }
}

String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

void reconnect() {
  String clientName;
  const char* mqttUser = "admin";
  const char* mqttPW = "admin";
  clientName += "diswasher-";
  uint8_t mac[6];
  WiFi.macAddress(mac);
  clientName += macToStr(mac);
  clientName += "-";
  clientName += String(micros() & 0xff, 16);
  while (!client.connected()) {
    if (client.connect((char*) clientName.c_str(), mqttUser, mqttPW)) { // random client id
      digitalWrite(BUILTIN_LED, LOW); // low = high, so this turns on the led
      client.subscribe("dishwasher/sub"); // callback: mqtt bus -> arduino
      client.publish  ("dishwasher/pub", "hello"); 
    } else {
      digitalWrite(BUILTIN_LED, HIGH); // high = low, so this turns off the led
      delay(5000);
    }
  }
}

void loop() {
  ArduinoOTA.handle();
  
  if (!client.connected()) {
    reconnect();
    return;
  }

  server.handleClient();
  MDNS.update();

  client.loop();
  while (Serial.available() > 0) {
    char b = Serial.read();
    swSer.write(b);
    pushDisplay(b);
  }

  while (swSer.available() > 0) {
    char b = swSer.read();
    Serial.write(b);
    pushMachine(b);
  }
}


