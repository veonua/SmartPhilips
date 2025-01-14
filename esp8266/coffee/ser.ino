//serial Input
char buff_old[ser_buf_size+2];

size_t serIn = 0;

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

        // print buffer to debug
        debug.printf("received %02x %02x:", buff[0], buff[1]);
        // for (int i = 0; i < ser_buf_size; i++) {
        //   debug.printf("%02x ", buff[i]);
        // }
        // debug.printf("\n");
        
        if (buff_old[2] != buff[2]) {
          publish("switch", buff[2] == 0x1 ? "off" : "on");
        }

        debug.printf("on:%02x", buff[2]);
        debug.printf(" es:%02x hw:%02x co:%02x ca:%02x am:%02x", buff[3], buff[4], buff[5], buff[6], buff[7]);

        debug.printf(" st:%02x", buff[8]);
        debug.printf(" gr:%02x", buff[9]);
        debug.printf(" wl:%02x", buff[10]);
        debug.printf(" ??:%02x", buff[11]);
        debug.printf(" ac:%02x", buff[12]);
        debug.printf(" cl:%02x", buff[13]);
        debug.printf(" wt:%02x", buff[14]);
        debug.printf(" er:%02x", buff[15]);
        debug.printf(" br:%02x", buff[16]);
        debug.printf(" **:%02x\n", buff[17]);
        

        if (status != get_status(buff_old)) {
          publish("status", status_str(status));

          if (status != STATUS_SELECTED && status != STATUS_SELECT_BREW && status != STATUS_BREWING) {
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
        
        if (status == STATUS_SELECTED) {
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
          bool ready = (status == STATUS_SELECT_BREW) || (status == STATUS_SELECTED);
          if (ready) {
            debug.printf("status: %s commands: %d now: %d, last: %d \n", status_str(status).c_str(), command_queue.size(), now, lastPush);

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
  mqttClient.publish(("smartthings/coffee/lattego/"+std::string(topic)).c_str(), 1, true, payload.c_str(), payload.length());
}
