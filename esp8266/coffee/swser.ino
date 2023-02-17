// swSerial Input

const size_t swSer_buf_size = 12;
char swSerInCommand[swSer_buf_size+1];
char swSerInCommand_old[swSer_buf_size+1];
int counter = 0;
int swSer_count = 0;

inline void swSerialInput2Mqtt() {
  while (swSer.available() && (swSer_count++ < 1024)) {
    char c = swSer.read();
    Serial.write(c);

    if (swSer_count <= swSer_buf_size)
      swSerInCommand[swSer_count-1] = c;
  }

  // if swSerInCommand is different from swSerInCommand_old, then print

  if (memcmp(swSerInCommand, swSerInCommand_old, swSer_buf_size) != 0) {
    // print buffer to debug
    if (swSerInCommand[11] != 0x12) {
      // d5 55 00 01 03 00 0d 00 00 00 02 12
      debug.printf("swSer received: ");
      for (int i = 0; i < swSer_buf_size; i++) {
        debug.printf("%02x ", swSerInCommand[i]);
        if (i==11) debug.printf("*");
      }
      debug.printf("\n");
    }
    // copy swSerInCommand to swSerInCommand_old
    memcpy(swSerInCommand_old, swSerInCommand, swSer_buf_size);
  }
  
  swSer_count = 0;
}