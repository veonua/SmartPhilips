const size_t host_len = strlen(hostname);

void onMqttMessage(char *topic, char *_payload,
                   AsyncMqttClientMessageProperties properties, size_t len, size_t index,
                   size_t total) {
  if (len == 0) {
    return;
  }

  char payload[len + 1];
  memcpy(payload, _payload, len);
  payload[len] = '\0';

  topic = topic + (host_len+5); // remove hostname and "/set/"
  
  if (strcmp(topic, "switch") == 0) {
    set_switch(payload);
  } else if (strcmp(topic, "state") == 0) {
    set_state(payload);
  } else if (strcmp(topic, "brew") == 0) {
    set_brew(payload);
  } else if (strcmp(topic, "water_level") == 0) {
    set_water_level(payload);  
  } else if (strcmp(topic, "strength_level") == 0) {
    set_strength_level(payload);
  } else if (strcmp(topic, "preset1") == 0) {
    run_preset(payload);
  } else if (strcmp(topic, "powerOn") == 0) {
    powerOn(repeat);
  } else if (strcmp(topic, "powerOff") == 0) {
    serialSend(powerOff, repeat);
  } else if (strcmp(topic, "hotWater") == 0) {
    serialSend(hotWater, repeat);
  } else if (strcmp(topic, "espresso") == 0) {
    serialSend(espresso, repeat);
  } else if (strcmp(topic, "coffee") == 0) {
    serialSend(coffee, repeat);
  } else if (strcmp(topic, "steam") == 0) {
    serialSend(steam, repeat);
  } else if (strcmp(topic, "coffeePulver") == 0) {
    serialSend(coffeePulver, repeat);
  } else if (strcmp(topic, "coffeeWater") == 0) {
    serialSend(coffeeWater, repeat);
  } else if (strcmp(topic, "calcNclean") == 0) {
    serialSend(calcNclean, repeat);
  } else if (strcmp(topic, "aquaClean") == 0) {
    serialSend(aquaClean, repeat);
  //} else if (strcmp(topic, "startPause") == 0) {
  //  serialSend(startPause, repeat);
  } else if (strcmp(topic, "restart") == 0) {
    debug.println("Restarting...");
    ESP.restart();
  } else {
    debug.printf("Publish received. topic: %s len: %d index: %d total: %d\n",
              topic, len, index, total);
  }
}
