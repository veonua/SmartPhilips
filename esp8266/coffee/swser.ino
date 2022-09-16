
int pos = 0;
const int buffLen = 10;
char swBuf[buffLen];
char swBuf_old[buffLen];
byte swState = 0;

void swser() {
    byte b = swSer.read();
    Serial.write(b);
    
    if (swState == 0) {
        if (b == 0xd5) {
            swState = 1;
        }
        return;
    }

    if (swState == 1) {
        if (b == 0x55) {
            swState = 2;
        } else {
            swState = 0;
        }
        return;
    }

    swSer.readBytes(swBuf, 8);
    Serial.write(swBuf, 8);

    if (strncmp(swBuf, swBuf_old, 6) != 0) {
        debug.print("d5: ");
        for (int i = 0; i < 6; i++) {
            debug.printf("%02x ", swBuf[i]);
        }
        debug.println();
        memcpy( swBuf_old, swBuf, buffLen);
    }
    
    swState = 0;
}