void webserver_init() {
  server.on("/", handle_root);
  server.on("/state", handle_status);
  server.onNotFound(handle_notfound);
  server.begin();
}

void handle_root() {
  server.send(200, "text/plain", "hello from coffeemaker!\r\n");
}

void handle_status() {
    int status = get_status(buff);
    std::string response = "{";
    response += "\"state\":\"" + status_str(status) +"\"";
    
    if (status == STATUS_SELECTED || status == STATUS_BREWING) {
      response += ",\"brew\":\"" + selected_brew(buff) +"\",";
      response += "\"strength_level\":\"" + std::to_string(level(buff[ 8])) +"\",";
      response += "\"grinder\":\"" + grinder(buff[9]) +"\",";
      response += "\"water_level\":\""    + std::to_string(level(buff[10])) +"\"";
      response += "\"buf11\":\""    + std::to_string(buff[11]) +"\"";
    } else if (status >= STATUS_ERROR) {
      response += ",\"error\":\"" + error(buff[15]);
    }
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