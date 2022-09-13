#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <AsyncMqttClient.h>
#include <SoftwareSerial.h>
#include <iostream>  
#include <string>
#include <TelnetStream.h>

#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#define MQTT_HOST IPAddress(192, 168, 0, 224)
#define MQTT_PORT 1883

#define debug TelnetStream

const char *hostname = "dishwasher";

SoftwareSerial swSer (D2, D5); // RX, TX

#define debugSerial TelnetStream
AsyncMqttClient mqttClient;
ESP8266WebServer server(80);
WiFiClient espClient;
//PubSubClient client(espClient);

//*************globals*************
const byte buff_size = 255;
byte debugIdx = 0;


void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(4800);
  swSer.begin(4800);
  TelnetStream.begin();

  //debugSerial.begin(115200);
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  //mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  WIFI_Connect();
  mqtt_connect();
  otastart();
  
  if (MDNS.begin(hostname)) {
    debugSerial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/state", apiStatus);
  server.onNotFound(handleNotFound);
  server.begin();
}

void loop() {
  mqtt_connect();
  WIFI_Connect();
  ArduinoOTA.handle();
  
  server.handleClient();
  MDNS.update();
  
  serialHandler();

  while (swSer.available() > 0) {
    char b = swSer.read();
    Serial.write(b);
    pushMachine(b);
  }
}


