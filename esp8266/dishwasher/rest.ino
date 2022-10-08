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
    std::string door = (controller_buff[11] & 0x0f) == 8 ? "open" : "closed";

    std::string response = "{";
    response += "\"state\":\"" + state(controller_buff[1]) +"\",";
    response += "\"job_state\":\"" + job_state(controller_buff[2]) +"\",";
    response += "\"completion_min\":" + std::to_string(controller_buff[3]) +",";
    response += "\"water_level\":" + std::to_string(controller_buff[6]) +",";
    response += "\"temperature\":" + std::to_string(controller_buff[7]) +",";
    response += "\"baskets\":\"" + baskets((controller_buff[9] & 0xf0) >> 4) +"\",";
    response += "\"mode\":\"" + mode(controller_buff[9] & 0x0f) +"\",";
    response += "\"water_hardness\":" + std::to_string(controller_buff[10] >> 3) +",";
    response += "\"bottle_tab\":\"" + bottle_tab(controller_buff[13] & 0x0f) +"\",";
    response += "\"door\":\"" + door +"\"";
    response += "}";

    server.send(200, "text/json", response.c_str());
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