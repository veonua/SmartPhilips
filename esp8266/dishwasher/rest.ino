void handleRoot() {
  server.send(200, "text/plain", "hello from dishwasher!\r\n");
}

void apiStatus() {
    std::string door = (sw_old[11] & 0x0f) == 8 ? "open" : "closed";

    std::string response = "{";
    response += "\"state\":\"" + state(sw_old[1]) +"\",";
    response += "\"job_state\":\"" + job_state(sw_old[2]) +"\",";
    response += "\"completion_min\":" + std::to_string(sw_old[3]) +",";
    response += "\"water_level\":" + std::to_string(sw_old[6]) +",";
    response += "\"temperature\":" + std::to_string(sw_old[7]) +",";
    response += "\"baskets\":\"" + baskets((sw_old[9] & 0xf0) >> 4) +"\",";
    response += "\"mode\":\"" + mode(sw_old[9] & 0x0f) +"\",";
    response += "\"water_hardness\":" + std::to_string(sw_old[10] >> 3) +",";
    response += "\"bottle_tab\":\"" + bottle_tab(sw_old[13] & 0x0f) +"\",";
    response += "\"door\":\"" + door +"\"";
    response += "}";

    server.send(200, "text/json", response.c_str());
}

std::string json_bool(bool b) {
    return b ? "true" : "false";
}

void handleNotFound() {
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