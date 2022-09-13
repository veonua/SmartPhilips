int d_machine = 0;
byte ser_buff[16];
byte ser_old[16];

char dsmall[6];

std::string DPREFIX = "dishwasher/display/";

void serialHandler() {
  if (!Serial.available()) return;
    
  char c = Serial.read();
  //swSer.write(b);
    
  switch (d_machine)
  {
    case 0:
      if (c == 0x07) {
        d_machine = 1;
      };
      return;
    case 1:
      if (c == 0x22) {
        processBuff();
      }
      d_machine = 0;
      break;
  }
}

void processBuff() {
  Serial.readBytes(ser_buff, 7);
  byte sum = 0x22; 
    
  for (int i = 0; i < 6; i++) { 
    char c = ser_buff[i];
    sum += c;
    if (c == ser_old[i]) continue;
    ser_old[i] = c;
    switch (i)
    {
      case 0:
        if (c != 0x00) {
          d_publish("child_lock", key_state(c));
        }
        break;
      case 1:
        d_publish("bottle_tab", bottle_tab(c));
      case 2:
        d_publish("state", state(c));
        break;
      case 3:
        if (c==0) //always 0x00
          break;
      case 4:
        d_publish("mode", mode(c & 0x0f));
        break;
      case 5:
        d_publish("press", press((c & 0xf0) >> 4));
        d_publish("baskets", baskets(c & 0x0f));
        break;

      default:
        std::string topic;
        if (i<10)
          topic = "unknown/0" + std::to_string(i);
        else
          topic = "unknown/" + std::to_string(i);
    
        d_publish_hex((topic+"/hex").c_str(), c);
        d_publish((topic).c_str(), std::to_string(c));
        break;
    }
  }

  if (sum != ser_buff[6]) {
    d_publish_hex("checksum", sum);
  }
}


void d_publish(const char* topic, std::string payload) {
  mqttClient.publish((DPREFIX + topic).c_str(), 1, true, payload.c_str(), payload.length());
  //debug.printf("%s: %s\n", topic, payload.c_str());
}

void d_publish_hex(const char* topic, byte value) {
  sprintf(dsmall, "0x%02x", value);      
  mqttClient.publish((DPREFIX + topic).c_str(), 1, true, dsmall, strlen(dsmall));
  //debug.printf("%s: 0x%02x\n", topic, value);
}

std::string press(byte press){
  switch (press)
  {
    case 0:
      return "up";
    case 2:
      return "down";
    default:
      return "unknown_"+std::to_string(press);
  }
}


std::string mode(byte mode){
  switch (mode) {
    case 0x00:
      return "finish";
    case 0x1:
      return "auto";
    case 0x2:
      return "intense";
    case 0x4:
      return "eco";
    case 0x5:
      return "delicate";
    case 0x6:
      return "quick";
    case 0x7:
      return "quick delicate";
    case 0x8:
      return "rinse";
    default:
      return "unknown_"+std::to_string(mode);
  }
}

std::string baskets(byte baskets){
  switch (baskets) {
    case 0x01:
      return "top";
    case 0x02:
      return "bottom";
    case 0x03:
      return "both";
    default: // can be 9
      return "unknown_"+std::to_string(baskets);
  }
}

std::string bottle_tab(int bottle_tab) {
  switch (bottle_tab) {
    case 0x00:
      return "none";
    case 0x01:
      return "tab";
    case 0x02:
      return "bottle";
    default:
      return "unknown_"+std::to_string(bottle_tab);
  }
}

std::string state(int state) {
  switch (state) {
    case 0x00:
    case 0x02:
      return "off";

    case 0x10:
    case 0x11:
      return "prep";
    case 0x12:
    case 0x13:
      return "ready";

    case 0x20:
      return "timer";
    case 0x21:
      return "timer + closed door";
    case 0x22:
      return "timer + open door";

    case 0x30:
    case 0x31:
      return "wash preparation";
    case 0x32:
      return "wash";
    case 0x33:
      return "pause";
    case 0x34:
      return "finish";

    case 0x60:
    case 0x61:
      return "drain";
    default:
      return "unknown_"+std::to_string(state);
  }
}

std::string key_state(int key_state) {
  switch (key_state) {
    case 0x00: // button is not pressed
      return "none";
    case 0x03: 
      return "timer";
    case 0xe9: // lock request
      return "locked";
    case 0xee: // unlock request
      return "unlocked";
    case 0xea: // normal when locked
      return "locked";
    case 0xe1: // power on
    case 0xe2: // power off
    case 0xe5: // normal when unlocked
      return "unlocked";
    default:
      return "unknown_"+std::to_string(key_state);
  }
}