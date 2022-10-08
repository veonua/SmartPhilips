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

SoftwareSerial swSer (D2, D1); // RX, TX
AsyncMqttClient mqttClient;
ESP8266WebServer server(80);
WiFiClient espClient;

//*************globals*************
byte debugIdx = 0;
const char *hostname = "dishwasher";
const size_t host_len = strlen(hostname);


void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(4800);
  swSer.begin(4800);
  TelnetStream.begin();

  WIFI_Connect();
  mqtt_init();
  mqtt_connect();
  otastart();
  
  webserver_init();
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


