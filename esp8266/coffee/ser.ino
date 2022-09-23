//serial Input
char buff_old[ser_buf_size+2];

size_t serIn = 0;

long timestampLastSerialMsg = 0;

inline void serialInput2Mqtt() {

  while(Serial.available() > 0) {
    char b = Serial.read();
    buff[serIn] = b;
    serIn++; 

    //Skip input if it doesn't start with 0xd5
    if(serIn == 1 && b != 0xd5) {
      serIn = 0;
    }

    if(serIn >= ser_buf_size){
      timestampLastSerialMsg = millis();
      if (memcmp(buff, buff_old, ser_buf_size) != 0) {
      
        if (buff_old[2] != buff[2]) {
          publish("switch", buff[2] == 0x1 ? "off" : "on");
        }

        if (get_status(buff) != get_status(buff_old)) {
          publish("status", status_str(buff));
        }

        std::string brew = selected_brew(buff);
        if (brew != selected_brew(buff_old)) {
          publish("brew", brew);
        }
        
        if (buff_old[14] != buff[14]) {
          publish("water_tank", buff[14] == 0x0 ? "ok" : "empty");
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

        if (buff_old[15] != buff[15]) {
          publish("error", error(buff[15]));
        }

        memcpy( buff_old, buff, ser_buf_size);
      }

      if (command_queue.size() > 0) {
        debug.printf("commands: %d\n", command_queue.size());

        std::array<byte, 12> command = command_queue.front();
        auto array = (byte(&)[12])*command.data();
        
        serialSend(array, 5);
        command_queue.pop();
      }

      serIn = 0;
    }
  }

}

void publish(const char* topic, std::string payload) {
  //debug.printf("publish %s %s\n", topic, payload);
  mqttClient.publish(("smartthings/coffee/lattego/"+std::string(topic)).c_str(), payload.c_str());
}