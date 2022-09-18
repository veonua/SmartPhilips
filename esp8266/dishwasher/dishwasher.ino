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

#define debug TelnetStream

#define MQTT_HOST IPAddress(192, 168, 0, 224)
#define MQTT_PORT 1883
const char *hostname = "dishwasher_2";

//SoftwareSerial swSer (D2, D3); // RX, TX
SoftwareSerial swSer (D5, D6); // RX, TX

AsyncMqttClient mqttClient;
ESP8266WebServer server(80);
WiFiClient espClient;

//*************globals*************
const byte buff_size = 255;
byte debugIdx = 0;


void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(4800);
  swSer.begin(4800);
  TelnetStream.begin();

  WIFI_Connect();
  mqtt_init();
  mqtt_connect();
  otastart();
  
  if (MDNS.begin(hostname)) {
    debug.println("MDNS responder started");
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
  serial2Handler();
}


