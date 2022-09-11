int m_machine = 0;
char serInCommand[buff_size+1];
char small[6];
byte serInIdx = 0;

std::string PREFIX = "dishwasher/machine/";

// if buffer starts with 55 0F 2A then parse 8 bytes and send to mqtt

bool pushMachine(byte c) {
  if (m_machine == 0 && c==0x55) {
    m_machine = 1;
    return true;
  } else if (m_machine == 1 && c==0x0F) { // could be 8f
      m_machine = 2;
      return true;
  }

  if (m_machine != 2) {
    m_machine = 0;
    return false;
  }

  if (serInCommand[serInIdx] != c) { 
    switch (serInIdx)
    {
      case 1:
        publish("state", state(c));
        break;
      case 2:
        publish("job_state", job_state(c));
        publish_hex("job_state/hex", c);
        break;
      case 3: 
        publish("completion min", std::to_string(c)); // time till end
        break;
      case 4:
        publish("next cycle in", std::to_string(c)); 
        break;
      // case 5: 0x15 or 0x00 -- nothing while washing
      case 6: 
        publish("water/level", std::to_string(c)); // maybe water level
        break;
      case 7:
        publish("temperature", std::to_string(c));
        break;
      // case 8: 0x55 0x0f 0x8/0x10/0x18/0x80 then 0x0  
      // nothing while washing
      case 9:
        publish("baskets", baskets((c & 0xf0) >> 4));
        publish("mode", mode(c & 0x0f));
        break;
      case 10: {
        byte c_water_hardness = c >> 3; 
        publish("water hardness/level", std::to_string(c_water_hardness));
        publish("water hardness/salt gram", water_hardness(c_water_hardness));
        break;}
      case 11:
        publish("door", (c & 0x0f) == 8 ? "open" : "closed");
        publish_hex("door/hex", c);
        break;
      // case 12: 0x35 0x65 0xb5 0x75 0x1c 0x9d
      case 13:
        publish("bottle_tab", bottle_tab(c & 0x0f));
        publish_hex("bottle_tab/hex", c & 0xf0);
        break;
      // case 14: seconds cound down?
      // heater? 0x55 when temp starts to rise
      // 00, 31, 00 ,2a 
      case 15: //check sum?
        c = c - serInCommand[3]*2 - serInCommand[6] - serInCommand[7] - serInCommand[11] - serInCommand[12] - serInCommand[14]; // linear
        if (c != serInCommand[15]) {
          publish_hex("checksum", c);
        }
        break;
      default:
        std::string topic;
        if (serInIdx<10)
          topic = "unknown/0" + std::to_string(serInIdx);
        else
          topic = "unknown/" + std::to_string(serInIdx);
    
        publish_hex((topic+"/hex").c_str(), c);
        publish((topic).c_str(), std::to_string(c));
        break;
    }
    serInCommand[serInIdx] = c;
  }

  serInIdx++;

  if (serInIdx >= 20 ) { // last byte is changing all the time
    serInIdx = 0;
    m_machine = 0;
  }

  return true;
}

void publish(const char* topic, std::string value) {
  client.publish((PREFIX + topic).c_str(), value.c_str());
}

void publish_hex(const char* topic, byte value) {
  sprintf(small, "0x%02x", value);      
  client.publish((PREFIX + topic).c_str(), small);
}

std::string water_hardness(byte c_water_hardness) {
  switch (c_water_hardness)
  {
    case 1:
      return "0";
    case 2:
      return "9";
    case 3:
      return "12";
    case 4:
      return "20";
    case 5:
      return "30";  
    case 6:
      return "60";
    default:
      return "unknown "+std::to_string(c_water_hardness);
  }
}

std::string job_state(byte val) {
  switch (val){
    case 0x01:
      return "pump";    
    case 0x9: // 9
      return "drying";
    
    case 0x11: // 17
      return "pause"; // in the beginning and between rinse cycles 
    case 0x13: // 19
      return "wash"; // normal washing
    case 0x14: // 20
      return "unknown_14"; // rinse 
    case 0x15: // 21
      return "tab drop";
    case 0x1a: // 26
      return "new water"; // temp drops
    
    case 0x24: // 36
      return "prep_24";
    case 0x26: // 38
      return "heating"; // temp rises
    case 0x28: // 40
      return "unknown_28"; 
    
    // case 0x30 // 48
    case 0x31: // 49
      return "ready";
    case 0x32: // 50
      return "finish";
    
    case 0x43: // 67
      return "drying";
    //
    default:
      return "unknown";
  }
}
