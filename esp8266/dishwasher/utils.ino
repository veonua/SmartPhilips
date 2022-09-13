bool led_state = 0;
const char *ssid = "TP-Link_D5BA";
const char *password = "wifipassword654";

// MQTT
bool validData = 0;
int mqtt_status = false;

void otastart() {
  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.setPort(8266);
  ArduinoOTA.onStart([]() {
    debug.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    debug.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    debug.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    debug.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) debug.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) debug.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) debug.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) debug.println("Receive Failed");
    else if (error == OTA_END_ERROR) debug.println("End Failed");
  });
  ArduinoOTA.begin();
  debug.println("Ready");
  debug.print("IP address: ");
  debug.println(WiFi.localIP());
}

void WIFI_Connect() {
  if (WiFi.status() != WL_CONNECTED) {
    //WiFi.config(ip, gateway, subnet);
    debug.println("Connect WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    debug.println(WiFi.macAddress());

    for (int i = 0; i < 50; i++) {
      if (WiFi.status() != WL_CONNECTED) {
        delay(500);
        debug.print(".");
        delay(500);
        digitalWrite(BUILTIN_LED, led_state);
        led_state != led_state;
      }
    }
    debug.println(WiFi.localIP());
    if (WiFi.status() != WL_CONNECTED) {
      ESP.restart();
    }
  }
}


void onMqttConnect(bool sessionPresent) {
  debug.print("Connected to MQTT. Session present: ");
  debug.println(sessionPresent);
  
  mqttClient.publish(hostname, 1, true, "hello", 5);
  
//   String sub1 = "/#";
//   String subs = hostname + sub1;
//   mqttClient.subscribe(subs.c_str(), 1);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  debug.println("Disconnected from MQTT.");
  mqtt_status = false;
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  debug.print("Subscribe acknowledged.   packetId: ");
  debug.print(packetId);
  debug.print("  qos: ");
  debug.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
  debug.print("Unsubscribe acknowledged.  packetId: ");
  debug.println(packetId);
}

void mqtt_connect() {
  if (mqtt_status == false && WiFi.status() == WL_CONNECTED) {
    debug.println("Connecting to MQTT...");
    mqttClient.connect();
    mqtt_status = true;
  }
}

void onMqttMessage(char *topic1, char *payload1,
                   AsyncMqttClientMessageProperties properties, size_t len, size_t index,
                   size_t total) {
  debug.println(topic1);
  debug.println(payload1);
  debug.println(len);
//   strncpy(topic, topic1, 49);
//   if (len < 99) {
//     strncpy(payload, payload1, len);
//     payload[len] = '\0';
//     validData = 1;
//   }
}

void onMqttPublish(uint16_t packetId) {
//   debug.print("Publish acknowledged.   packetId: ");
//   debug.println(packetId);
}