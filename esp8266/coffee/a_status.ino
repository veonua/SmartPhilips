const byte STATUS_UNKNOWN = 0x00;
const byte STATUS_OFF = 0x01;
const byte STATUS_HEATING = 0x02;
const byte STATUS_SELECT_BREW = 0x03;
const byte STATUS_SELECTED = 0x04;
const byte STATUS_BREWING = 0x05;

const byte STATUS_ERROR = 0x10;
const byte STATUS_ERROR_TRESTER = 0x11;
const byte STATUS_ERROR_NOWATER = 0x12;

#define repeat 3


int level(byte value) {
  switch (value)
  {
    case 0x0:
      return  1;
    case 0x7:
      return -1;
    case 0x38:
      return  2;
    case 0x3f:
      return  3;
    default:
      return -2;
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

inline bool heating(char* buffer) {
    return (buffer[2] == 0x0) && 
    (buffer[3] == 3 || buffer[4] == 3 || buffer[5] == 3 || buffer[6] == 3);
}

inline bool not_selected(char* buffer) {
    return buffer[15] == 0x00 // no error
        && buffer[14] == 0x00 // water tank ok
        && buffer[13] == 0x00 // calcncclean
        && buffer[12] == 0x00 // aquaclean
        && buffer[3] == 0x07
        && buffer[4] == 0x07
        && buffer[5] == 0x07
        && buffer[6] == 0x07;
}


inline byte get_status(char* buffer) {
  if (buffer[2] == 0x1) 
    return STATUS_OFF;

  if (buffer[15] != 0x00) {
    if (buffer[15] == 0x07) {
      return STATUS_ERROR_TRESTER;
    }

    return STATUS_ERROR;
  }
  if (buffer[14] != 0x00)
    return STATUS_ERROR_NOWATER;

  if (buffer[11] == 0x7)
    return STATUS_SELECTED;
  if (heating(buffer))
    return STATUS_HEATING;
  if (buffer[16] == 0x7)
    return STATUS_BREWING;
  if (not_selected(buffer))
    return STATUS_SELECT_BREW;
  return STATUS_UNKNOWN;
}

std::string status_str(byte status) {
  switch (status)
  {
    case STATUS_OFF:
        return "off";
    case STATUS_SELECT_BREW:
        return "ready";
    case STATUS_HEATING:
        return "heating";
    case STATUS_SELECTED:
        return "waiting";
    case STATUS_BREWING:
        return "brewing";
    
    case STATUS_ERROR_NOWATER:
        return "no water";
    case STATUS_ERROR_TRESTER:
        return "trester";
    case STATUS_ERROR:
        return "error"; 
    default:
        return "unknown";
  } 
}

std::string selected_brew(char* buffer) {
    byte s = get_status(buffer);
    if (s == STATUS_SELECTED || s == STATUS_BREWING) {
        if (buffer[3] == 0x7) return "espresso";
        if (buffer[3] == 0x38) return "2xespresso";
        if (buffer[4] == 0x7) return "hot_water";
        if (buffer[5] == 0x7) return "coffee";
        if (buffer[5] == 0x38) return "2xcoffee";
        if (buffer[6] == 0x7) return "cappuccino";
    }

    return "";
}