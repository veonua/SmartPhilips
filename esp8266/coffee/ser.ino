//serial Input
const size_t ser_buf_size = 39;
char serInCommand[ser_buf_size+1];
char serInCommand_old[ser_buf_size+1];

char buff[20];


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

    if(serInIdx2 > 37){
      serInCommand[38] = '\0';
	    timestampLastSerialMsg = millis();
      if(strcmp(serInCommand, serInCommand_old) != 0){
        //mqttClient.publish("coffee/status", serInCommand, 38);
        debug.printf("publish %s\n", serInCommand);
        memcpy( serInCommand_old, serInCommand, 39);

        std::string brew = selected_brew(buff);
        mqttClient.publish("coffee/brew", brew.c_str());
      }
      serInIdx2 = 0;
      serIn = 0;
    }
  }

}

// inline void serialInput2Mqtt() {
//   while(Serial.available() > 0) {
//     char b = Serial.read();
//     serInCommand[serInIdx] = b;
//     serInIdx ++; 

//     //Skip input if it doesn't start with 0xd5
//     if(serInIdx == 1 && b != 0xd5) {
//       serInIdx = 0;
//       debug.printf("%02x.", b);
//     }

//     if(serInIdx >= ser_buf_size-1){
//       timestampLastSerialMsg = millis();
//       if(strcmp(serInCommand, serInCommand_old) != 0){
//         print_status(serInCommand);
//         memcpy( serInCommand_old, serInCommand, ser_buf_size);
//       }
//       serInIdx = 0;
//     }
//   }
//   //Send signal that the coffee machine is off if there is no incomming message for more than 3 seconds
//   if(timestampLastSerialMsg != 0 && millis() - timestampLastSerialMsg > 3000){
//     //client.publish("coffee/status", "d5550100000000000000000000000000000626", 38);
//     debug.println("timeout");
//     timestampLastSerialMsg = 0;
//   }
// }

// void print_status(char* buffer) {
//   debug.printf("> ");
//   for (int i = 0; i < ser_buf_size; i++) {
//     debug.printf("%02x", buffer[i]);
//   }
//   debug.println();
// }