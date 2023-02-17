#include <sstream>

void webserver_init() {
  server.on("/", handle_root);
  server.on("/state", handle_status);
  server.onNotFound(handle_notfound);
  server.begin();
}

void handle_root() {
  server.send(200, "text/plain", "hello from dishwasher!\r\n");
}

void handle_status() {
    std::ostringstream oss;

    oss << "{";
    oss << "\"state\":\"" << state(controller_buff[1]) << "\"";
    oss << ",\"job_state\":\"" << job_state(controller_buff[2]) << "\"";
    oss << ",\"completion_min\":" << static_cast<int>(controller_buff[3]);
    oss << ",\"water_level\":" << static_cast<int>(controller_buff[6]);
    oss << ",\"temperature\":" << static_cast<int>(controller_buff[7]);
    oss << ",\"baskets\":\"" << baskets((controller_buff[9] & 0xf0) >> 4) << "\"";
    oss << ",\"mode\":\"" << mode(controller_buff[9] & 0x0f) << "\"";
    oss << ",\"water_hardness\":" << static_cast<int>(controller_buff[10] >> 3);
    oss << ",\"door\":\"" << (isDoorOpen(controller_buff[11]) ? "open" : "closed") << "\"";
    oss << ",\"rinse\":\"" << (isRinseEmpty(controller_buff[11]) ? "empty" : "ok") << "\"";
    oss << ",\"bottle_tab\":\"" << bottle_tab(controller_buff[13] & 0x0f) << "\"";
    oss << "}";

    server.send(200, "text/json", oss.str().c_str());
}

std::string json_bool(bool b) {
    return b ? "true" : "false";
}

void handle_notfound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}