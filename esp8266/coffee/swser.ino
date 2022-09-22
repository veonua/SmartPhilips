// swSerial Input

const size_t swSer_buf_size = 11;
char swSerInCommand[swSer_buf_size+1];
char swSerInCommand_old[swSer_buf_size+1];
int counter = 0;
int swSer_count = 0;

inline void swSerialInput2Mqtt() {
  while (swSer.available() && (swSer_count++ < 1024)) {
    char c = swSer.read();
    Serial.write(c);
    
    // if (c != 0xd5) {
    //   //debug.printf(".%02x", c);
    //   return;
    // }

    // swSer.readBytes(swSerInCommand, swSer_buf_size);
    // Serial.write   (swSerInCommand, swSer_buf_size);
    // swSer_count += swSer_buf_size;
    // if (strcmp(swSerInCommand, swSerInCommand_old) == 0) {
    //   counter++;
    // } else {
    //   debug.printf("\nprev %d. d5 ", counter); 
    //   for (int i = 0; i < swSer_buf_size; i++) {
    //       debug.printf("%02x", swSerInCommand[i]);
    //   }

    //   if (swSerInCommand[0] != 0x55 || swSerInCommand[swSer_buf_size-1] == 0x55) {
    //       debug.println (" invalid swSerInCommand, skip");
    //   } else {
    //       debug.println();
    //       memcpy( swSerInCommand_old, swSerInCommand, swSer_buf_size);
    //       counter = 0;
    //   }
    // }
  }
  swSer_count = 0;
}