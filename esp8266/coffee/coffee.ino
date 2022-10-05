#include <Arduino.h>
#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#define GET_CHIPID()  (ESP.getChipId())
#elif defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <SPIFFS.h>
#include <HTTPClient.h>
#define GET_CHIPID()  ((uint16_t)(ESP.getEfuseMac()>>32))
#endif
#include <FS.h>
#include <ArduinoOTA.h>
#include <SoftwareSerial.h>
#include <PubSubClient.h>
#include <AutoConnect.h>
#include <TelnetStream.h>
#include <queue>

#define MQTT_HOST IPAddress(192, 168, 0, 224)
#define MQTT_PORT 1883

#define debug TelnetStream
const char *hostname = "cofeemaker";
const char *ssid = "TP-Link_D5BA";
const char *password = "wifipassword654";


//SoftwareSerial to read Display TX (D5 = RX, D6 = unused)
SoftwareSerial swSer (D2, D1); // RX, TX

#define PARAM_FILE      "/param.json"
#define AUX_SETTING_URI "/mqtt_setting"
#define AUX_SAVE_URI    "/mqtt_save"
#define AUX_TEST_URI    "/test"

// Adjusting WebServer class with between ESP8266 and ESP32.
#if defined(ARDUINO_ARCH_ESP8266)
typedef ESP8266WebServer  WiFiWebServer;
#elif defined(ARDUINO_ARCH_ESP32)
typedef WebServer WiFiWebServer;
#endif

#define GND_BREAKER_PIN D5

AutoConnect  portal;
AutoConnectConfig config;
WiFiClient   wifiClient;
PubSubClient mqttClient(wifiClient);

//Definition of functions:
char convertCharToHex(char ch);
void runCustomCommand(String customCmd, int length);
void callback(String topic, byte* message, int length);
void serialReadPublish(char newStatus[]);
void serialSend(const byte command[], int sendCount);
void redirect(String uri);
void powerOn(int count);

//MQTT Settings
String mqttServer = "192.168.0.224";
String mqttPort = "1883";
String mqttUser = "admin";
String mqttPW = "admin";

// current status
const size_t ser_buf_size = 18;
char buff[ser_buf_size+2];

std::queue<std::array<byte, 12>> command_queue;


//commands
//byte powerOn[] =      {0xd5, 0x55, 0x01, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x35, 0x05};
byte requestInfo[] =  {0xD5, 0x55, 0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x14};

//                       d5    55.   00    01    03    00    0d    01    00    00    0e    1f.
byte powerOff[] =     {0xd5, 0x55, 0x00, 0x01, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x0d, 0x19};


byte hotWater[] =     {0xD5, 0x55, 0x00, 0x01, 0x00, 0x00, 0x02, 0x04, 0x00, 0x00, 0x31, 0x23};
byte espresso[12] =     {0xD5, 0x55, 0x00, 0x01, 0x00, 0x00, 0x02, 0x02, 0x00, 0x00, 0x19, 0x0F};
byte coffee[12] =       {0xD5, 0x55, 0x00, 0x01, 0x00, 0x00, 0x02, 0x08, 0x00, 0x00, 0x29, 0x3E};
byte steam[] =        {0xD5, 0x55, 0x00, 0x01, 0x00, 0x00, 0x02, 0x10, 0x00, 0x00, 0x19, 0x04};
byte coffeePulver[12] = {0xD5, 0x55, 0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x02, 0x00, 0x19, 0x0D};
byte coffeeWater[12] =  {0xD5, 0x55, 0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x04, 0x00, 0x30, 0x27};
byte calcNclean[] =   {0xD5, 0x55, 0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x20, 0x00, 0x38, 0x15};
byte aquaClean[] =    {0xD5, 0x55, 0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x10, 0x00, 0x1D, 0x14};
byte startPause[] =   {0xD5, 0x55, 0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x01, 0x09, 0x10};

char oldStatus[19] = "";


// Declare AutoConnectElements for the page asf /mqtt_setting
ACStyle(style, "label+input,label+select{position:sticky;left:120px;width:230px!important;box-sizing:border-box;}");
ACText(header, "<h2>MQTT Settings</h2>", "text-align:center;color:#2f4f4f;padding:10px;");
ACText(caption, "MQTT Server Settings. The following MQTT commands are availabe:<br><br>coffee/command/powerOn<br>coffee/command/powerOff<br>coffee/command/hotWater<br>coffee/command/espresso<br>coffee/command/coffee<br>coffee/command/steam<br>coffee/command/coffeePulver<br>coffee/command/coffeeWater<br>coffee/command/calcNclean<br>coffee/command/aquaClean<br>coffee/command/startPause<br><br>Send the command with a count of repeats as value (typical 5).", "font-family:serif;color:#4682b4;");
ACInput(inMqttserver, mqttServer.c_str(), "MQTT-Server", "", "e.g. 192.168.172.99");
ACInput(inMqttport, mqttPort.c_str(), "MQTT Port", "", "e.g. 1883 or 1884");
ACInput(inMqttuser, mqttUser.c_str(), "MQTT User", "", "default: admin");
ACInput(inMqttpw, mqttPW.c_str(), "MQTT Password", "", "default: admin");
ACText(mqttState, "MQTT-State: none", "");
ACSubmit(save, "Save", AUX_SAVE_URI);
ACSubmit(discard, "Discard", "/");
ACElement(newline, "<hr>");

// Declare the custom Web page as /mqtt_setting and contains the AutoConnectElements
AutoConnectAux mqtt_setting(AUX_SETTING_URI, "MQTT Settings", true, {
  style,
  header,
  caption,
  newline,
  inMqttserver,
  inMqttport,
  newline,
  inMqttuser,
  inMqttpw,
  newline,
  mqttState,
  newline,
  save,
  discard
});


// Declare AutoConnectElements for the page as /mqtt_save
ACText(caption2, "<h4>Parameters available as:</h4>", "text-align:center;color:#2f4f4f;padding:10px;");
ACText(parameters);
ACSubmit(back2config, "Back", AUX_SETTING_URI);

// Declare the custom Web page as /mqtt_save and contains the AutoConnectElements
AutoConnectAux mqtt_save(AUX_SAVE_URI, "MQTT Settings", false, {
  caption2,
  parameters,
  back2config
});


// Declare AutoConnectElements for the page for testing
ACText(testHeader, "<h2>Coffee Machine Test</h2>", "text-align:center;color:#2f4f4f;padding:10px;");
ACSubmit(testOn, "ON", "/on");
ACSubmit(testOff, "OFF", "/off");

// Declare the custom Web page as /mqtt_setting and contains the AutoConnectElements
AutoConnectAux coffee_test(AUX_TEST_URI, "Coffee TEST", true, {
  style,
  testHeader,
  testOn,
  testOff
});

bool mqttConnect() {
  String clientId = "CoffeeMachine-" + String(GET_CHIPID(), HEX);

  uint8_t retry = 3;
  while (!mqttClient.connected()) {
    if (mqttServer.length() <= 0)
      break;

    mqttClient.setServer(mqttServer.c_str(), atoi(mqttPort.c_str()));
    mqttClient.setCallback(callback);
    
    debug.println(String("Attempting MQTT broker:") + mqttServer);

    if (mqttClient.connect(clientId.c_str(), mqttUser.c_str(), mqttPW.c_str())) {
      debug.println("Established:" + clientId);
      mqttClient.publish("coffee/status", "ESP_STARTUP 1.5");
      mqttClient.subscribe("coffee/command/#");
      mqttClient.subscribe("coffee/set/#");
      mqttState.value = "MQTT-State: <b style=\"color: green;\">Connected</b>";
      return true;
    }
    else {
      debug.println("Connection failed:" + String(mqttClient.state()));
      mqttState.value = "MQTT-State: <b style=\"color: red;\">Disconnected</b>";
      if (!--retry){
        break;
      }
      //delay(3000);
    }
  }
  return false;
}

// This functions is executed when some device publishes a message to a topic that your ESP8266 is subscribed to
// Change the function below to add logic to your program, so when a device publishes a message to a topic that
// your ESP8266 is subscribed you can actually do something
void callback(String topic, byte* message, int length) {

  char messageTemp[100];
  strncpy(messageTemp, (char*)message, length);
  messageTemp[length] = '\0';

  debug.printf("Message arrived on topic: %s Message: %s\n", topic.c_str(), messageTemp);
  
  if (topic == "coffee/set/switch") {
    set_switch(messageTemp);
  } else if (topic == "coffee/set/state") {
    set_state(messageTemp);
  } else if (topic == "coffee/set/brew") {
    set_brew(messageTemp);
  } else if (topic == "coffee/set/water_level") {
    set_water_level(messageTemp);  
  } else if (topic == "coffee/set/strength_level") {
    set_strength_level(messageTemp);
  } else if (topic == "coffee/command/custom") {
    //For custom hex commands
    runCustomCommand(messageTemp, length);
  } else {
    //For predefined commands
    int count = atoi(messageTemp);
    if (count < 0 || count > 99) {
      debug.println("Count out of range");
    } else if (topic == "coffee/command/powerOn") {
      powerOn(count);
    } else if (topic == "coffee/command/powerOff") {
      serialSend(powerOff, count);
    } else if (topic == "coffee/command/hotWater") {
      serialSend(hotWater, count);
    } else if (topic == "coffee/command/espresso") {
      serialSend(espresso, count);
    } else if (topic == "coffee/command/coffee") {
      serialSend(coffee, count);
    } else if (topic == "coffee/command/steam") {
      serialSend(steam, count);
    } else if (topic == "coffee/command/coffeePulver") {
      serialSend(coffeePulver, count);
    } else if (topic == "coffee/command/coffeeWater") {
      serialSend(coffeeWater, count);
    } else if (topic == "coffee/command/calcNclean") {
      serialSend(calcNclean, count);
    } else if (topic == "coffee/command/aquaClean") {
      serialSend(aquaClean, count);
    } else if (topic == "coffee/command/startPause") {
      serialSend(startPause, count);
    } else if (topic == "coffee/command/detect") {
      serialDetect();
    } else if (topic == "coffee/command/restart") {
      debug.println("Restarting...");
      ESP.restart();
    } else if (topic == "coffee/command/delay") {
      debug.println("Delaying...");
      delay(count * 1000UL);
      debug.println(".");
    } else if (topic == "coffee/command/display") {
      debug.println("Delaying...");
      delay(3500);
      byte cmd3[] = {0xd5, 0x55, 0x00, 0x01, 0x03, 0x00, 0x0d, 0x00, 0x00, 0x20, 0xe2, 0x75};
      serialSend(cmd3, count);

      // // d555000103000d0000000212
      debug.println("done.");
    }
    
  }
}

///
void powerOn(int count) {
  // d5550a0103000d0000001d36 
  // d555010103000d0000003603
  // delay 4000
  // d555000103000d000020e275

  // d5550a0103000d0000001d36
  // d555010103000d0000003603

  /// gg: d5 55 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 39 0d 
  /// d555000103000d0000000212


  byte cmd1[] = {0xd5, 0x55, 0x0a, 0x01, 0x03, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x1d, 0x36};
  byte cmd2[] = {0xd5, 0x55, 0x01, 0x01, 0x03, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x36, 0x03};

  //byte powerOnCmd[] =      {0xd5, 0x55, 0x01, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x35, 0x05};
  digitalWrite(GND_BREAKER_PIN, LOW);
  debug.println("Power ON");
  serialSend(cmd1, count);
  serialSend(cmd2, count);
  
  delay(300);
  digitalWrite(GND_BREAKER_PIN, HIGH);

}

void serialDetect() {
  debug.println("Detecting...");
  long rate = Serial.detectBaudrate(1000UL);
  debug.println("Detected baudrate: " + String(rate));
}

//Executes a custom command (hex string) received via mqtt
void runCustomCommand(const char* customCmd, int length) {
  byte data2send[length];
  for (int i = 0; i < length / 2; i++) {
    byte extract;
    char a = (char)customCmd[2 * i];
    char b = (char)customCmd[2 * i + 1];
    extract = convertCharToHex(a) << 4 | convertCharToHex(b);
    data2send[i] = extract;
  }
  Serial.write(data2send, length / 2);
}
//Sends a command via serial to the coffee machine
void serialSend(const byte command[12], int sendCount) {
  debug.printf("Sending %d times: ", sendCount);
  for (int i = 0; i < 12; i++) {
    debug.printf("%02x", command[i]);
  }
  debug.println();
  
  for (int i = 0; i <= sendCount; i++) {
    Serial.write(command, 12);
  }

}

//Checks if the status of the coffee machine had changed. If so, the new status is published via mqtt
void serialReadPublish(char newStatus[]) {
  if (strcmp(newStatus, oldStatus) != 0) {
    mqttClient.publish("coffee/status", newStatus);
    strncpy(oldStatus, newStatus, 19);
  }
}

String loadParams() {
  File param = SPIFFS.open(PARAM_FILE, "r");
  if (param) {
    if(mqtt_setting.loadElement(param)){
      mqttServer = mqtt_setting["inMqttserver"].value;
      mqttServer.trim();
      mqttPort = mqtt_setting["inMqttport"].value;
      mqttPort.trim();
      mqttUser = mqtt_setting["inMqttuser"].value;
      mqttUser.trim();
      mqttPW = mqtt_setting["inMqttpw"].value;
      mqttPW.trim();
    }
    param.close();
  }
  else
    debug.println(PARAM_FILE " open failed");
  return String("");
}

// Retreive the value of each element entered by '/mqtt_setting'.
String saveParams(AutoConnectAux& aux, PageArgument& args) {
  mqttServer = inMqttserver.value;
  mqttServer.trim();

  mqttPort = inMqttport.value;
  mqttPort.trim();

  mqttUser = inMqttuser.value;
  mqttUser.trim();

  mqttPW = inMqttpw.value;
  mqttPW.trim();

  // The entered value is owned by AutoConnectAux of /mqtt_setting.
  // To retrieve the elements of /mqtt_setting, it is necessary to get
  // the AutoConnectAux object of /mqtt_setting.
  File param = SPIFFS.open(PARAM_FILE, "w");
  mqtt_setting.saveElement(param, { "inMqttserver", "inMqttport", "inMqttuser", "inMqttpw"});
  param.close();
  
  // Echo back saved parameters to AutoConnectAux page.
  String echo = "Server: " + mqttServer + "<br>";
  echo += "Port: " + mqttPort + "<br>";
  echo += "User: " + mqttUser + "<br>";
  echo += "Password: " + mqttPW + "<br>";
  parameters.value = echo;
  mqttClient.disconnect();
  return String("");
}

void sendPowerOff(){
  serialSend(powerOff, 5);
  redirect(AUX_TEST_URI);
}

void sendPowerOn(){
  powerOn(2);
  redirect(AUX_TEST_URI);
}
void handleRoot() {
  redirect("/_ac");
}

void redirect(String uri) {
  WiFiWebServer&  webServer = portal.host();
  webServer.sendHeader("Location", String("http://") + webServer.client().localIP().toString() + uri);
  webServer.send(302, "text/plain", "");
  webServer.client().flush();
  webServer.client().stop();
}

void setup() {
  delay(1000);
  debug.begin(); // debug port for debugging purposes
  Serial.begin(115200); // Serial communication with coffee machine (Main Controller + ESP)
  swSer .begin(115200);  // Serial communication to read display TX and give it over to Serial
  Serial.setTimeout(100);
  swSer.setTimeout(100);

  SPIFFS.begin();
  config.title = "Philips MQTT Coffee Machine";
  config.apid = "esp-coffee-machine";
  config.psk  = "";
  config.portalTimeout = 100; // Not sure the parameter to set
  config.retainPortal = true; 
  portal.config(config);
  loadParams();
  // Join the custom Web pages and register /mqtt_save handler
  portal.join({ mqtt_setting, mqtt_save, coffee_test });
  portal.on(AUX_SAVE_URI, saveParams);

  debug.print("WiFi ");
  if (portal.begin()) {
    debug.println("connected:" + WiFi.SSID());
    debug.println("IP:" + WiFi.localIP().toString());
    //Setup OTA Update
  }
  else {
    debug.println("connection failed:" + String(WiFi.status()));
  }

  otastart();
  WiFiWebServer&  webServer = portal.host();
  webServer.on("/", handleRoot);
  webServer.on("/on", sendPowerOn);
  webServer.on("/off", sendPowerOff);

  pinMode(GND_BREAKER_PIN, OUTPUT);
  digitalWrite(GND_BREAKER_PIN, HIGH);
}

int mqtt_attempts = 5;

void loop() {
  
  WIFI_Connect();
  ArduinoOTA.handle();

  swSerialInput2Mqtt();
//portal.handleClient(); 

  if (WiFi.status() == WL_CONNECTED) {
    //The following things can only be handled if wifi is connected
    if (mqtt_attempts > 0) {
      if (!mqttClient.connected()) {
        delay(1000);
        mqttConnect();
        mqtt_attempts--;
      } else {
        mqttClient.loop();
      }
    }
    serialInput2Mqtt();
  }
  
  


}