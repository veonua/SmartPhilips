int d_machine = 0;
char serOutCommand[buff_size+1];
char dsmall[6];
byte serOutIdx = 0;

std::string DPREFIX = "dishwasher/display/";

// if buffer starts with x07 x22 then parse 8 bytes and send to mqtt

bool pushDisplay(byte c) {
  if (d_machine == 0 && c==0x07) {
    d_machine = 1;
    return true;
  } else if (d_machine == 1 && c==0x22) {
    d_machine = 2;
    return true;
  } 

  if (d_machine != 2) {
    d_machine = 0;
    return false;
  }

  if (serOutCommand[serOutIdx] != c) {
    switch (serOutIdx)
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
      case 4:
        d_publish("mode", mode(c & 0x0f));
        break;
      case 5:
        d_publish("press", press((c & 0xf0) >> 4));
        d_publish("baskets", baskets(c & 0x0f));
        break;

      case 3:
        if (c==0) //always 0x00
          break;
      default:
        std::string topic;
        if (serInIdx<10)
          topic = "unknown/0" + std::to_string(serInIdx);
        else
          topic = "unknown/" + std::to_string(serInIdx);
    
        d_publish_hex((topic+"/hex").c_str(), c);
        d_publish((topic).c_str(), std::to_string(c));
        break;
    }
    serOutCommand[serOutIdx] = c;
  }

  serOutIdx++;

  if (serOutIdx >= 15) {
    serOutIdx = 0;
    d_machine = 0;
  }

  return true;
}

void d_publish(const char* topic, std::string value) {
  client.publish((DPREFIX + topic).c_str(), value.c_str());
}

void d_publish_hex(const char* topic, byte value) {
  sprintf(dsmall, "0x%02x", value);      
  client.publish((DPREFIX + topic).c_str(), dsmall);
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