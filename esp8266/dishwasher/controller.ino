int m_machine = 0;
#define controller_buff_len 15
byte controller_buff[controller_buff_len];
byte old_controller_buff[controller_buff_len];
byte sw_sum = 0;
byte old_sw_sum = 0;

float temperature = 0f;
float tempBuff = 0f;
byte tempBuffSize = 0;


std::string PREFIX = "smartthings/dishwasher/samsung/";

// if buffer starts with 55 0F 2A then parse 8 bytes and send to mqtt
void serial2Handler() {
  if (!swSer.available()) return;

  char c = swSer.read();
  
  switch (m_machine) {
    case 0:
      if (c == 0x55) {
        m_machine = 1;
      } else {
        debug.printf("-%02X", c);
      }
      break;
    case 1:
      if (c == 0x0F) {
        m_machine = 2;
        sw_sum = 0;
      } else {
        debug.printf("\n unknown buffer start: 0x%02X\n", c);
        m_machine = 0;
      }
      break;
    /***/
    case 17: {
      m_machine = 0;
      if (sw_sum != c) {
        debug.printf("\n cont checksum error: 0x%02X\n", c);
        break;
      }

      if (old_sw_sum == sw_sum) {
        //debug.printf("\n cont checksum same: 0x%02X\n", c);
        break;
      }

      if (old_controller_buff[1] != controller_buff[1]) {
        byte _state = controller_buff[1];
        publish("state", state(_state));
        publish("switch", _state/16 == 0 ? "off" : "on");
      }
      if (old_controller_buff[2] != controller_buff[2]) {
        byte _jobState = controller_buff[2];
        publish("jobState", job_state(_jobState));
        publish_hex("jobState/hex", _jobState);
      }
      if (old_controller_buff[3] != controller_buff[3]) {
        publish("completion_min", std::to_string(controller_buff[3])); // time till end
      }
      if (old_controller_buff[4] != controller_buff[4]) {
        publish("next_cycle_in", std::to_string(controller_buff[4]));
      }

      if (old_controller_buff[7] != controller_buff[7]) {
        byte temp = controller_buff[7];
        tempBuff + temp;
        if (tempBuffSize < 10) {
          tempBuffSize++;
        }
        
        float avg = tempBuff / tempBuffSize;
        tempBuff -= avg;
        
        if ( abs(temperature - avg) > 0.5) { // reports way too often otherwise
           temperature = temp;
           publish("temperature", std::to_string(temperature));
        }
      }
      
      if (old_controller_buff[9] != controller_buff[9]) {
        byte b9 = controller_buff[9];
        publish("baskets", baskets((b9 & 0xf0) >> 4));
        publish("mode", mode(b9 & 0x0f));
      }
      // if (old_controller_buff[10] != controller_buff[10]) {
      //   byte c_water_hardness = controller_buff[10] >> 3; 
      //   publish("water_hardness/level", std::to_string(c_water_hardness));
      //   publish("water_hardness/salt_gram", water_hardness(c_water_hardness));
      // }
      if (old_controller_buff[11] != controller_buff[11]) {
        publish("contact", isDoorOpen() ? "open" : "closed");
        publish_hex("contact/hex", controller_buff[11]);
      }
      
      if (old_controller_buff[12] != controller_buff[12]) {
        byte b12 = controller_buff[12];
        publish("bottle_tab", bottle_tab(b12 & 0x0f));
        publish_hex("bottle_tab/hex", b12);
      }

      
      memcpy(old_controller_buff, controller_buff, controller_buff_len);
      old_sw_sum = sw_sum;
      break;
    }
    /***/

    default:
      sw_sum += c;
      m_machine++;
      controller_buff[m_machine-3] = c;
      //c = processSwBuff_0F(m_machine-3, c);
      break;
  }
  Serial.write(c);
}

char processSwBuff_0F(int i, char c) {
  if (i<15) {
    sw_sum += c;
  } else {
    m_machine = 0;
    
    if (sw_sum != c) {
      debug.printf("\n cont checksum error: 0x%02X\n", c);
    } else {

      char _state = controller_buff[1];
      publish("state", state(_state));
      publish("switch", _state/16 == 0 ? "off" : "on");
      
      char jobState = controller_buff[2];
      publish("job_state", job_state(jobState));
      publish_hex("job_state/hex", jobState);

    }

    if (c != controller_buff[i]) {
      controller_buff[i] = c;
      print_c_old();
    }

    return sw_sum;
  }

  if (c == controller_buff[i]) return c;
  controller_buff[i] = c;
  switch (i)
  {
    case 3: 
      publish("completion_min", std::to_string(c)); // time till end
      break;
    case 4:
      publish("next_cycle_in", std::to_string(c)); 
      break;
    // case 5: 0x15 or 0x00 -- nothing while washing
    // case 6: 
    //   publish("water/level", std::to_string(c)); // maybe water level
    //   break;
    // case 7:
    //   if ( abs(temperature - c) > 1) { // reports way too often otherwise
    //     temperature = c;
    //     publish("temperature", std::to_string(c));
    //   }
    //   break;
    // case 8: 0x55 0x0f 0x8/0x10/0x18/0x80 then 0x0  
    // nothing while washing
    case 9:
      publish("baskets", baskets((c & 0xf0) >> 4));
      publish("mode", mode(c & 0x0f));
      break;
    case 10: {
      byte c_water_hardness = c >> 3; 
      publish("water_hardness/level", std::to_string(c_water_hardness));
      publish("water_hardness/salt_gram", water_hardness(c_water_hardness));
      break;}
    case 11:
      publish("contact", (c & 0x0f) == 8 ? "open" : "closed");
      publish_hex("contact/hex", c);
      break;
    // case 12: 0x35 0x65 0xb5 0x75 0x1c 0x9d
    case 13:
      publish("bottle_tab", bottle_tab(c & 0x0f));
      publish_hex("bottle_tab/hex", c & 0xf0);
      break;
    // case 14: seconds cound down?
    // heater? 0x55 when temp starts to rise
    // 00, 31, 00 ,2a
    //default:
      //std::string topic = "unknown/" + std::to_string(i);
    
      // publish_hex((topic+"/hex").c_str(), c);
      // publish((topic).c_str(), std::to_string(c));
    //  break;
  }
}

inline bool isDoorOpen() {
  return (controller_buff[11] & 0x0f) == 8;
}

void print_c_old() {
  debug.print("c_old: ");
  for (int i = 0; i < 16; i++) {
    debug.printf("%02X ", controller_buff[i]);
  }
  debug.println();
}

inline void publish(const char* topic, std::string payload) {
  mqttClient.publish((PREFIX + topic).c_str(), 1, true, payload.c_str(), payload.length());
  //debug.printf("%s: %s\n", topic, payload.c_str());
}

inline void publish_hex(const char* topic, byte value) {
  char small[6];
  sprintf(small, "0x%02x", value);      
  mqttClient.publish((PREFIX + topic).c_str(), 1, true, small, strlen(small));
  //debug.printf("%s: 0x%02x\n", topic, value);
}

inline std::string water_hardness(byte c_water_hardness) {
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

inline std::string job_state(byte val) {
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
