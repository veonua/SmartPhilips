std::string level(byte level) {
  switch (level)
  {
    case 0x0:
      return "1";
    case 0x7:
      return "error?";
    case 0x38:
      return "2";
    case 0x3f:
      return "3";
    default:
      return "unknown "+std::to_string(level);
  }
}

std::string indicator(byte level) {
  switch (level)
  {
    case 0:
      return "";
    case 1:
      return "off?";
    case 3:
      return "blink";
    case 0x7:
      return "x1";
    case 0x38:
      return "x2";
    default:
      return "unknown "+std::to_string(level);
  }
}

std::string watertank(byte level) {
  switch (level)
  {
    case 0:
      return "ok";
    case 0x38:
      return "empty";
    default:
      return "unknown "+std::to_string(level);
  }
}

std::string grinder(byte mode) {
  switch (mode)
  {
    case 0:
      return "none";
    case 0x7:
      return "normal";
    case 0x38:
      return "powder";
    default:
      return "unknown "+std::to_string(mode);
  }
}

std::string error(byte mode) {
  switch (mode)
  {
    case 0:
      return "";
    case 0x7:
      return "trester";
    case 0x38:
      return "error";
    default:
      return "unknown "+std::to_string(mode);
  }
}

bool heating(char* buffer) {
    return (buffer[2] == 0x0) && 
    (buffer[3] == 3 || buffer[4] == 3 || buffer[5] == 3 || buffer[6] == 3);
}

bool ready(char* buffer) {
    return buffer[15] == 0x00 // no error
        && buffer[14] == 0x00 // water tank ok
        && buffer[13] == 0x00 // calcncclean
        && buffer[12] == 0x00 // aquaclean
        && !heating(buffer);
}


const byte STATUS_UNKNOWN = 0x00;
const byte STATUS_OFF = 0x01;
const byte STATUS_HEATING = 0x02;
const byte STATUS_WAITING = 0x03;
const byte STATUS_WATER_TANK_EMPTY = 0x04;
const byte STATUS_ERROR = 0x05;
const byte STATUS_READY = 0x06;

byte getStatus(char* buffer) {
  if (buffer[2] == 0x1) 
    return STATUS_OFF;

  if (buffer[15] != 0x00) 
    return STATUS_ERROR;
  if (buffer[14] != 0x00)
    return STATUS_WATER_TANK_EMPTY;
  if (buffer[11] == 0x7)
    return STATUS_WAITING;
  if (heating(buffer))
    return STATUS_HEATING;
  if (ready(buffer))
    return STATUS_READY;
  return STATUS_UNKNOWN;
}

std::string status_str(char* buffer) {
  switch (getStatus(buffer))
    {
    case STATUS_OFF:
        return "off";
    case STATUS_READY:
        return "ready";
    case STATUS_HEATING:
        return "heating";
    case STATUS_WAITING:
        return "waiting";
    case STATUS_WATER_TANK_EMPTY:
        return "water tank empty";
    case STATUS_ERROR:
        return "error"; 
    default:
        return "unknown";
    } 
}

std::string selected_brew(char* buffer) {
    byte s = getStatus(buffer);
    if (s == STATUS_WAITING) {
        if (buffer[3] == 0x7) return "espresso";
        if (buffer[3] == 0x38) return "2xespresso";
        if (buffer[4] == 0x7) return "hot water";
        if (buffer[5] == 0x7) return "coffee";
        if (buffer[5] == 0x38) return "2xcoffee";
        if (buffer[6] == 0x7) return "capucino";
    }

    return "";
}