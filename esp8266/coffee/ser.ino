//serial Input
char buff_old[ser_buf_size+2];

size_t serIn = 0;

unsigned long lastPush = 0;
unsigned long lastChange = 0;

inline void serialInput2Mqtt() {

  while(Serial.available() > 0) {
    char b = Serial.read();
    buff[serIn] = b;
    serIn++; 

    //Skip input if it doesn't start with 0xd5
    if(serIn == 1 && b != 0xd5) {
      serIn = 0;
    }

    if(serIn >= ser_buf_size) {
      byte status = get_status(buff); 

      if (memcmp(buff, buff_old, ser_buf_size) != 0) {
        lastChange = millis();
      
        if (buff_old[2] != buff[2]) {
          publish("switch", buff[2] == 0x1 ? "off" : "on");
        }
        if (status != get_status(buff_old)) {
          publish("status", status_str(status));

          if (status != STATUS_WAITING && status != STATUS_READY) {
            publish("brew", "");
            publish("strength_level", "0");
            publish("grinder", "none");
            publish("water_level", "1");
          }
        }

        if (buff_old[14] != buff[14]) {
          publish("water_tank", buff[14] == 0x0 ? "ok" : "empty");
        }

        if (buff_old[15] != buff[15]) {
          publish("error", error(buff[15]));
        }
        
        if (status == STATUS_WAITING) {
          std::string brew = selected_brew(buff);
          if ( brew != selected_brew(buff_old) && brew != "") {
            publish("brew", brew);
          }
          
          if (buff_old[9] != buff[9] || buff[8] != buff_old[8]) {
            byte grind = buff[9];
            if (grind == 0x07) { // grind
              publish("strength_level", std::to_string(level(buff[8])));
            } else {
              publish("strength_level", "0");
            }
            publish("grinder", grinder(grind));
          }

          if (buff_old[10] != buff[10]) {
            publish("water_level", std::to_string(level(buff[10])));
          }
        }

        memcpy( buff_old, buff, ser_buf_size);
      }


      ///
      if (command_queue.size() > 0) {
        unsigned long now = millis();
        if (now - lastPush > 300) {
          bool ready = (status == STATUS_READY) || (status == STATUS_WAITING);
          if (ready) {
            debug.printf("commands: %d now: %d, last: %d \n", command_queue.size(), now, lastPush);

            lastPush = now;

            //std::array<byte, 12> command = command_queue.front();
            auto array = (byte(&)[12])* command_queue.front().data();
            
            serialSend(array, repeat);
            command_queue.pop();
          }
        }
      }

      serIn = 0;
    }
  }

}

void publish(const char* topic, std::string payload) {
  //debug.printf("publish %s %s\n", topic, payload);
  mqttClient.publish(("smartthings/coffee/lattego/"+std::string(topic)).c_str(), payload.c_str());
}