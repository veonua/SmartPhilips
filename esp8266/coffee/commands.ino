#define repeat 5
#define click_delay 2000

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
    
    if (strcmp(value, "cleaning") == 0) {
        
    } else {
        debug.printf("Unknown switch value: %s\n", value);
    }
}

void set_brew(const char* value) {
  std::string _brew = selected_brew(buff);
  std::string brew = std::string(value);

  if (_brew == brew) {
    return;
  }

  if (brew == "espresso") {
    serialSend(espresso, repeat);
  } else if (brew == "2xespresso") {
    serialSend(espresso, repeat);
    if (_brew != "espresso") {
      push_command(espresso);
    }

  } else if (brew == "coffee") {
    serialSend(coffee, repeat);
  } else if (brew == "2xcoffee") {
    serialSend(coffee, repeat);
    if (_brew != "coffee") {
      push_command(coffee);
    }
  } else if (brew == "cappuccino") {
    serialSend(steam, repeat);
  } else if (brew == "hot water" || brew == "hot_water") {
    serialSend(hotWater, repeat);
  } else {
    debug.printf("Unknown brew value: %s\n", value);
  }
}


void set_level(int current_level, const char* value, const byte command[12]) {
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

    serialSend(command, repeat);
    for (int i = 0; i < count-1; i++) {
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


void push_command(const byte value[12]) {
    std::array<byte, 12> a;
    std::copy( value, value + 12, a.begin() );

    command_queue.push(a);
}