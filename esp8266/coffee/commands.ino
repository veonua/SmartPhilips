

void powerOn(int count) {
  // d5550a0103000d0000001d36 
  // d555010103000d0000003603
  // delay 4000
  // d555000103000d000020e275

  // d5550a0103000d0000001d36
  // d555010103000d0000003603

  /// gg: d5 55 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 39 0d 
  /// d555000103000d0000000212


  byte cmd1[] = {0xd5, 0x55, 0x0a, 0x01, 0x03, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x1d, 0x36};
  byte cmd2[] = {0xd5, 0x55, 0x01, 0x01, 0x03, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x36, 0x03};

  //byte powerOnCmd[] =      {0xd5, 0x55, 0x01, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x35, 0x05};
  digitalWrite(GND_BREAKER_PIN, LOW);
  displayOff = true;
  debug.println("Power ON");
  serialSend(cmd1, count);
  serialSend(cmd2, count);
}

void set_switch(const char* value) {
    if (strcmp(value, "on") == 0) {
        powerOn(repeat);
    } else if (strcmp(value, "off") == 0) {
        serialSend(powerOff, repeat);
    } else {
        debug.printf("Unknown switch value: %s\n", value);
    }
}


void set_state(const char* value) {
  serialSend(startPause, repeat);
  
  if (strcmp(value, "cleaning") == 0 || // brewing
    strcmp(value, "idle") == 0) {
  } else {
    debug.printf("Unknown switch value: %s\n", value);
  }
}

void set_brew(const char* value) {
  if (strlen(value) == 0) {
    return;
  }

  byte status = get_status(buff);

  if (status == STATUS_OFF) {
    debug.printf("Turn on machine!\n");
    powerOn(repeat);
  }

  std::string _brew = selected_brew(buff);
  std::string brew = std::string(value);

  if (_brew == brew) {
    return;
  }

  if (brew == "espresso") {
    push_command(espresso);
  } else if (brew == "2xespresso") {
    push_command(espresso);
    if (_brew != "espresso") {
      push_command(espresso);
    }

  } else if (brew == "coffee") {
    push_command(coffee);
  } else if (brew == "2xcoffee") {
    push_command(coffee);
    if (_brew != "coffee") {
      push_command(coffee);
    }
  } else if (brew == "cappuccino") {
    push_command(steam);
  } else if (brew == "hot water" || brew == "hot_water") {
    push_command(hotWater);
  } else {
    debug.printf("Unknown brew value: %s\n", value);
  }
}


void set_level(int current_level, const char* value, const byte command[12]) {
  byte status = get_status(buff);

    if (status != STATUS_SELECTED) {
      debug.printf("Machine is not ready!\n");
      current_level = 2;
    }

    int selected_level = atoi(value);
    if (selected_level<1 || selected_level>3) {
        debug.printf("Unknown water level value: %s\n", value);
        return;
    }

    if (current_level == selected_level) {
        return;
    }

    int count = selected_level - current_level;
    if (current_level > selected_level) {
        count = 3 - current_level + selected_level;
    }

    debug.printf("current_level: %d, selected_level: %d, count: %d\n", current_level, selected_level, count);

    for (int i = 0; i < count; i++) {
        push_command(command);
    }
}

void set_strength_level(const char* value) {
    int current_level = level(buff[8]);
    set_level(current_level, value, coffeePulver);
}

void set_water_level(const char* value) {
    int current_level = level(buff[10]);
    set_level(current_level, value, coffeeWater);
}

void run_preset(const char* value) {
  debug.printf("run_preset: %s\n", value);
  set_brew("cappuccino");
  set_level(2, "3", coffeePulver);
  set_level(2, "3", coffeeWater);
}


void push_command(const byte value[12]) {
    std::array<byte, 12> a;
    std::copy( value, value + 12, a.begin() );

    command_queue.push(a);
}