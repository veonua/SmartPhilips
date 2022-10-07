bool led_state = true;
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
  
  String subs = String(hostname) + "/set/#";
  mqttClient.subscribe(subs.c_str(), 1);
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

void mqtt_init() {
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  //mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
}

void mqtt_connect() {
  if (mqtt_status == false && WiFi.status() == WL_CONNECTED) {
    debug.println("Connecting to MQTT...");
    mqttClient.connect();
    mqtt_status = true;
  }
}

void onMqttMessage(char *topic, char *_payload,
                   AsyncMqttClientMessageProperties properties, size_t len, size_t index,
                   size_t total) {
  
  char payload[len + 1];
  memcpy(payload, _payload, len);
  payload[len] = '\0';

  size_t host_len = strlen(hostname) + 1;
  topic = topic + host_len;

  if (strncmp(topic, "sdisplay", 8) == 0) {
    if (strlen(topic) == 8) {
      hex2bin(payload, ser_payload, min((size_t)7, len));
      debug.println("sdisplay");
    } else {
      int index = atoi(topic + 9);
      ser_payload[index] = hex2bin(payload);
      debug.printf("index: %d, payload: %s, ser_payload: %d\n", index, payload, ser_payload[index]);
    }
  } else if (strcmp(topic, "set/state") == 0) {
    set_state(payload);
  } else if (strcmp(topic, "set/switch") == 0) {
    set_switch(payload);
  }else if (strcmp(topic, "set/mode") == 0) {
    set_mode(payload);
  } else if (strcmp(topic, "set/baskets") == 0) {
    set_baskets(payload);
  } else if (strcmp(topic, "turn_on") == 0) {
    turn_on();
  } else if (strcmp(topic, "turn_off") == 0) {
    turn_off();
  } else {
    debug.printf("Publish received. topic: %s len: %d index: %d total: %d\n",
              topic, len, index, total);
  }
}

void onMqttPublish(uint16_t packetId) {
//   debug.print("Publish acknowledged.   packetId: ");
//   debug.println(packetId);
}

/****/

byte turn_off_cmd = 0x00;
byte turn_on_cmd  = 0x10;
//   timer        = 0x20;
byte start_cmd    = 0x30;
byte pause_cmd    = 0x33;
byte resume_cmd   = 0x31;
byte finish_cmd   = 0x40;
byte water_cmd    = 0x50;
byte drain_cmd    = 0x60;
// sounds
// E1 - on
// E2 - off
// E3 - click / error
// E4 - ding (wash finished)
// E5 - start wash == E3
// E6 - stop wash

void set_baskets(char* value) {
    if (strcmp(value, "top") == 0) {
        ser_payload[5] = 0x01;
    } else if (strcmp(value, "bottom") == 0) {
        ser_payload[5] = 0x02;
    } else if (strcmp(value, "both") == 0) {
        ser_payload[5] = 0x03;
    } else {
        debug.printf("Unknown basket value: %s (should be top, bottom or both)", value);
    }
}

void set_mode(char* mode) {
    if (strcmp(mode, "auto") == 0) {
        ser_payload[4] = 0x11;
    } else if (strcmp(mode, "intense") == 0) {
        ser_payload[4] = 0x12;
    } else if (strcmp(mode, "eco") == 0) {
        ser_payload[4] = 0x14;
    } else if (strcmp(mode, "delicate") == 0) {
        ser_payload[4] = 0x15;
    } else if (strcmp(mode, "quick") == 0) {
        ser_payload[4] = 0x16;
    } else if (strcmp(mode, "quick_delicate") == 0) {
        ser_payload[4] = 0x17;
    } else if (strcmp(mode, "rinse") == 0) {
        ser_payload[4] = 0x18;
    } else {
        debug.printf("Unknown mode value: %s (should be auto, intense, eco, delicate, quick, quick_delicate or rinse)", mode);
    }
}

void set_state(char* state) {
    if (strcmp(state, "run") == 0) {
      if (controller_buff[1] == pause_cmd) {
        ser_payload[2] = resume_cmd;
      } else {
        ser_payload[2] = start_cmd;
      }
    } else if (strcmp(state, "stop") == 0) {
        ser_payload[2] = drain_cmd;
    } else if (strcmp(state, "pause") == 0) {
        ser_payload[2] = pause_cmd;
    } else {
        debug.printf("Unknown state value: %s (should be run, pause or stop)", state);
    }
}

void set_switch(char* value) {
    if (strcmp(value, "on") == 0) {
        ser_payload[2] = turn_on_cmd;
    } else if (strcmp(value, "off") == 0) {
        ser_payload[2] = turn_off_cmd;
    } else {
        debug.printf("Unknown switch value: %s (should be on or off)", value);
    }
}

void turn_on() {
    ser_payload[2] = 0x10; 
}

void turn_off() {
    ser_payload[2] = 0x00; 
}

void start() {
    ser_payload[2] = 0x30; 
}

void finish() {
    ser_payload[2] = 0x40; 
}

inline byte hex2bin(const char *hex) {
  char a = hex[0];
  char b = hex[1];
  if (a >= '0' && a <= '9') a -= '0';
  else if (a >= 'A' && a <= 'F') a -= 'A' - 10;
  else if (a >= 'a' && a <= 'f') a -= 'a' - 10;
  if (b >= '0' && b <= '9') b -= '0';
  else if (b >= 'A' && b <= 'F') b -= 'A' - 10;
  else if (b >= 'a' && b <= 'f') b -= 'a' - 10;
  return (a << 4) | b;
}

// hex string to buffer
void hex2bin(const char *hex, byte *bin, size_t len) {
  for (size_t i = 0; i < len; i++) {
    bin[i] = hex2bin(hex + i * 2);
  }
}