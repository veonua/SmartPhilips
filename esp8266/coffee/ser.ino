//serial Input
const size_t ser_buf_size = 18;
char serInCommand[ser_buf_size+1];
char serInCommand_old[ser_buf_size+1];

void serialInput2Mqtt() {
  size_t len = Serial.available();
  if (len == 0) return;

  char c = Serial.read();
  swSer.write(c);

  if (c != 0xd5) {
    //debug.printf("%02x+", c);
    return;
  }

  Serial.readBytes(serInCommand, ser_buf_size);
  //swSer.write(serInCommand, ser_buf_size);

  if (strcmp(serInCommand, serInCommand_old) != 0) {
    //mqttClient.publish("coffee/status", serInCommand, 38);
    debug.print("\ngg: d5 "); // green - gray
      for (int i = 0; i < ser_buf_size; i++) {
      debug.printf("%02x ", serInCommand[i]);
    }
    debug.println();
    memcpy( serInCommand_old, serInCommand, ser_buf_size);
  }
}