//serial Input
char serInCommand[40];

const size_t ser_buf_size = 18;

char buff[ser_buf_size+2];
char buff_old[ser_buf_size+2];

size_t serInIdx2 = 0;
size_t serIn = 0;

long timestampLastSerialMsg = 0;

inline void serialInput2Mqtt() {

  while(Serial.available() > 0) {
    char b = Serial.read();
    sprintf(&serInCommand[serInIdx2],"%02x", b);
    serInIdx2 += 2;
    buff[serIn] = b;
    serIn++; 

    //Skip input if it doesn't start with 0xd5
    if(serInIdx2 == 2 && b != 0xd5) {
      serInIdx2 = 0;
      serIn = 0;
    }

    if(serIn >= ser_buf_size){
      serInCommand[38] = '\0';
	    timestampLastSerialMsg = millis();
      if (memcmp(buff, buff_old, ser_buf_size) != 0) {
      
        publish("brew", selected_brew(buff));
        if (buff_old[2] != buff[2]) {
          publish("switch", buff[2] == 0x1 ? "off" : "on");
        }
        publish("status", status_str(buff));

        if (buff_old[14] != buff[14]) {
          publish("water_tank", buff[14] == 0x0 ? "ok" : "empty");
        }
        
        if (buff_old[9] != buff[9]) {
          byte grind = buff[9];
          if (grind == 0x07) { // grind
            publish("strength", level(buff[8]));
          } else {
            publish("strength", "");
          }
          publish("grinder", grinder(grind));
        }

        if (buff_old[10] != buff[10]) {
          publish("water", level(buff[10]));
        }

        memcpy( buff_old, buff, 20);
      }

      serInIdx2 = 0;
      serIn = 0;
    }
  }

}

void publish(const char* topic, std::string payload) {
  //debug.printf("publish %s %s\n", topic, payload);
  mqttClient.publish(("smartthings/coffee/lattego/"+std::string(topic)).c_str(), payload.c_str());
}