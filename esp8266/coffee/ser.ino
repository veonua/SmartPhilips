//serial Input
char serInCommand[39];
char serInCommand_old[39];
byte ser_buf_size = 16;

unsigned long timestampLastSerialMsg;
byte serInIdx = 0;


void serialInput2Mqtt() {
  if (Serial.available() == 0) return;


  char b = Serial.read();
  
  //Skip input if it doesn't start with 0xd5
  if (serInIdx == 0) {
    if (b != 0xd5) {
      return;
    }
    serInIdx = 1;
    return;
  }

  debug.print("+");  
  Serial.readBytes(serInCommand,ser_buf_size);
  debug.println("+");

  timestampLastSerialMsg = millis();
  if (strcmp(serInCommand, serInCommand_old) != 0) {
    //mqttClient.publish("coffee/status", serInCommand, 38);
    debug.print("rx: ");
    for (int i = 0; i < ser_buf_size; i++) {
      debug.printf("%02x ", serInCommand[i]);
    }
    debug.println();
    memcpy( serInCommand_old, serInCommand, ser_buf_size);
  }
  serInIdx = 0;
  //Send signal that the coffee machine is off if there is no incomming message for more than 3 seconds
  // if(timestampLastSerialMsg != 0 && millis() - timestampLastSerialMsg > 3000){
  //   mqttClient.publish("coffee/status", "d5550100000000000000000000000000000626", 38);
  //   timestampLastSerialMsg = 0;
  // }
}