const int LED = 2;
bool led_state = 0;

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
        digitalWrite(LED, led_state);
        led_state != led_state;
      }
    }
    debug.println(WiFi.localIP());
    if (WiFi.status() != WL_CONNECTED) {
      ESP.restart();
    }
  }
}


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