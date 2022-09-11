//#include "arduino.h"
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ArduinoOTA.h>
#include <AsyncMqttClient.h>
#include <TelnetStream.h>

#define MQTT_HOST IPAddress(192, 168, 0, 224)
#define MQTT_PORT 1883

#define debug TelnetStream

const char *hostname = "heatpump";
const char *ssid = "TP-Link_D5BA";
const char *password = "wifipassword654";

const int LED = 2;
bool led_state = 0;

//*******constructors***********

AsyncMqttClient mqttClient;

//***********globals************
int mqtt_status = false;
String power_state = "off";
int offtimer = 0;
char payload[100] = {0};
char topic[50] = {0};
bool validData = 0;


void setup() {
  Serial.begin(9600, SERIAL_8E1); // 9600 baud, 8 data bits, even parity, 1 stop bit
  //Serial.begin(4800); // veon: thats not true
  Serial.swap(); //remap tp pins d8,15(tx) and d7,13(rx) for connection to ac unit
  TelnetStream.begin();
  
  //debug.begin(115200); //debug port on pin2 (tx only)
  Serial.setTimeout(100);
  debug.println("boot");
  pinMode(LED, OUTPUT);
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  //mqttClient.setClientId(hostname); + WiFi.localIP()
  mqttClient.setClientId("heatpump");

  WIFI_Connect();
  mqtt_connect();
  otastart();
  digitalWrite(LED, true);
  start_handshake();
  delay(1000);
  doinit();
}

void loop() {
  mqtt_connect();
  WIFI_Connect();
  ArduinoOTA.handle();
  serialHandler();
  watchdog();
  offmode();
  parse();
}

void offmode() {
  if (offtimer != 0) {
    if ((millis() - offtimer) > 3600000) { //   60 minute
      offtimer = 0;
      char val[4] = "off";
      stateControl(val);
    }
  }
}

void watchdog() {
  static long int timer = 0;
  if ((millis() - timer) > 60000) { //   1 minute
    timer = millis();
    dowatchdog();
  }
}

void serialHandler() {
  static int state = 0;
  int a = 0;
  if (!Serial.available()) {
    return;
  }
    
  switch (state) {
    case 0: //look for header
      a = Serial.read();
      debug.println();
      debug.print("*I*");
      debug.print(a);
      if (a == 2) state = 1;
      break;

    case 1: //look for header
      a = Serial.read();
      debug.print(a);
      state = 0;
      if (a == 0) state = 2;
      break;

    case 2: //look for header
      a = Serial.read();
      debug.print(a);
      state = 0;
      if (a == 3) state = 3;
      break;

    case 3:
      byte datah[64] = {0};
      byte data[64] = {0};
      state = 0;
      Serial.readBytes(datah, 4); //header
      int more = datah[3]; //length of remaining data
      if (more <= 30) {
        int len = Serial.readBytes(data, more + 1);
        for (int i = 0; i < len; i++) {
          debug.print(' ');
          debug.print(data[i], DEC);
        }
        debug.print(" *I* more: ");
        debug.print(more);
        debug.print(" len: ");
        debug.println(len);
        

        if (len == 10) {
          data[5] = data[7];
          data[6] = data[8];
        }

        if (len == 8 || len == 10) {

          if (data[5] == 187) { //14
            String sub1 = "/roomtemp";
            mqtt_int(sub1, data[6]);
          }
          if (data[5] == 190) {
            String sub1 = "/outdoortemp";
            mqtt_int(sub1, int_to_signed(data[6]));
          }
          if (data[5] == 179) {
            String sub1 = "/setpoint/state";
            mqtt_int(sub1, int_to_signed(data[6]));
          }
          if (data[5] == 128) {
            String sub1 = "/state/state";
            String state = inttostate(data[6]);
            mqtt_int(sub1, state);
            power_state = state;
            if (state == "off") {
              // when power state is OFF, sent unit mode also as "off"
              mqtt_int("/mode/state", "off");
            }
          }
          if (data[5] == 160) {
            String sub1 = "/fanmode/state";
            mqtt_int(sub1, inttofanmode(data[6]));
          }
          if (data[5] == 163) { //14
            String sub1 = "/swingmode/state";
            mqtt_int(sub1, inttoswing(data[6]));
          }
          if (data[5] == 176) {
            String mode = "off";
            String sub1 = "/mode/state";
            // report actual mode when unit is running or "off" when it's not
            if (power_state == "on") mode = inttomode(data[6]);
            mqtt_int(sub1, mode);
          }
          break;

        }
      }
  }
  
}

void mqtt_int(String sub1, int param) {
  String subs = hostname + sub1;
  char val[10] = {0};
  itoa(param, val, 10);
  MQTTSend(subs.c_str(), val);
}

void mqtt_int(String sub1, String val) {
  String subs = hostname + sub1;
  MQTTSend(subs.c_str(), val.c_str());
}

void start_handshake() {
  debug.println("start handshake");
  int a = 8;
  int bootlist1[a] = {2, 255, 255, 0, 0, 0, 0, 2};
  doArray(bootlist1, a);
  a = 9;
  int bootlist2[a] = {2, 255, 255, 1, 0, 0, 1, 2, 254};
  doArray(bootlist2, a);
  a = 10;
  int bootlist3[a] = {2, 0, 0, 0, 0, 0, 2, 2, 2, 250};
  doArray(bootlist3, a);
  a = 10;
  int bootlist4[a] = {2, 0, 1, 129, 1, 0, 2, 0, 0, 123};
  doArray(bootlist4, a);
  a = 10;
  int bootlist5[a] = {2, 0, 1, 2, 0, 0, 2, 0, 0, 254};
  doArray(bootlist5, a);
  a = 7;
  int bootlist6[a] = {2, 0, 2, 0, 0, 0, 0, 254};
  doArray(bootlist6, a);
  delay(1800);
  //aftershake
  a = 10;
  int bootlist7[a] = {2, 0, 2, 1, 0, 0, 2, 0, 0, 251};
  doArray(bootlist7, a);
  a = 10;
  int bootlist8[a] = {2, 0, 2, 2, 0, 0, 2, 0, 0, 250};
  doArray(bootlist8, a);
}

void doArray(int array[], int a) {
  debug.print('doArray: ');
  for (int i = 0; i < a; i++) {
    debug.print("*");
    debug.print(array[i]);
    Serial.write(array[i]);
  }
  debug.println(' ');
  delay(200);
}

int modetoint( char *msg) {
  int retint = 0;
  if (strcmp(msg, "auto") == 0) retint = 65;
  if (strcmp(msg, "cool") == 0) retint = 66;
  if (strcmp(msg, "heat") == 0) retint = 67;
  if (strcmp(msg, "dry") == 0) retint = 68;
  if (strcmp(msg, "fan_only") == 0) retint = 69;
  return retint;
}

String inttomode (byte val) {
  String reply;
  if (val == 65) reply = "auto";
  if (val == 66) reply = "cool";
  if (val == 67) reply = "heat";
  if (val == 68) reply = "dry";
  if (val == 69) reply = "fan_only";
  return reply;
}

int fanmodetoint(char *msg) {
  int retint = 0;
  if (strcmp(msg, "quiet") == 0) retint = 49;
  if (strcmp(msg, "lvl_1") == 0) retint = 50;
  if (strcmp(msg, "lvl_2") == 0) retint = 51;
  if (strcmp(msg, "lvl_3") == 0) retint = 52;
  if (strcmp(msg, "lvl_4") == 0) retint = 53;
  if (strcmp(msg, "lvl_5") == 0) retint = 54;
  if (strcmp(msg, "auto") == 0) retint = 65;
  return retint;
}

String inttofanmode (byte val) {
  String reply;
  if (val == 49) reply = "quiet";
  if (val == 50) reply = "lvl_1";
  if (val == 51) reply = "lvl_2";
  if (val == 52) reply = "lvl_3";
  if (val == 53) reply = "lvl_4";
  if (val == 54) reply = "lvl_5";
  if (val == 65) reply = "auto";
  return reply;
}


int swingtoint(char *msg) {
  int retint = 0;
  if (strcmp(msg, "off") == 0) retint = 49;
  if (strcmp(msg, "on") == 0) retint = 65;
  return retint;
}

String inttoswing (byte val) {
  String reply;
  if (val == 49) reply = "off";
  if (val == 65) reply = "on";
  return reply;
}

int statetoint(char *msg) {
  int retint = 0;
  if (strcmp(msg, "off") == 0) retint = 49;
  if (strcmp(msg, "on") == 0) retint = 48;
  return retint;
}

String inttostate (byte val) {
  String reply;
  if (val == 49) reply = "off";
  if (val == 48) reply = "on";
  return reply;
}

int int_to_signed(int val) {
  if (val > 127) val = (256 - val) * -1;
  return val;
}

int checksum(int msg, int function) {
  int retval = 0;
  int numb = 434 - msg - function;
  if (numb > 256) {
    retval = numb - 256;
  } else {
    retval = numb;
  }
  return retval;
}

void otastart() {
  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.setPort(8266);
  ArduinoOTA.onStart([]() {
    debug.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    debug.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    debug.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    debug.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) debug.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) debug.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) debug.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) debug.println("Receive Failed");
    else if (error == OTA_END_ERROR) debug.println("End Failed");
  });
  ArduinoOTA.begin();
  debug.println("Ready");
  debug.print("IP address: ");
  debug.println(WiFi.localIP());
}

void WIFI_Connect() {
  if (WiFi.status() != WL_CONNECTED) {
    //WiFi.config(ip, gateway, subnet);
    debug.println("Connect WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    debug.println(WiFi.macAddress());

    for (int i = 0; i < 50; i++) {
      if (WiFi.status() != WL_CONNECTED) {
        delay(500);
        debug.print(".");
        delay(500);
        digitalWrite(LED, led_state);
        led_state != led_state;
      }
    }
    debug.println(WiFi.localIP());
    if (WiFi.status() != WL_CONNECTED) {
      ESP.restart();
    }
  }
}


void onMqttConnect(bool sessionPresent) {
  debug.print("Connected to MQTT.");
  debug.print("Session present: ");
  debug.println(sessionPresent);
  
  mqttClient.publish(hostname, 1, true, "hello", 5);
  
  String sub1 = "/#";
  String subs = hostname + sub1;
  mqttClient.subscribe(subs.c_str(), 1);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  debug.println("Disconnected from MQTT.");
  mqtt_status = false;
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  debug.print("Subscribe acknowledged.");
  debug.print("  packetId: ");
  debug.print(packetId);
  debug.print("  qos: ");
  debug.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
  debug.print("Unsubscribe acknowledged.");
  debug.print("  packetId: ");
  debug.println(packetId);
}

void mqtt_connect() {
  if (mqtt_status == false && WiFi.status() == WL_CONNECTED) {
    debug.println("Connecting to MQTT...");
    mqttClient.connect();
    mqtt_status = true;
  }
}

void onMqttMessage(char *topic1, char *payload1,
                   AsyncMqttClientMessageProperties properties, size_t len, size_t index,
                   size_t total) {
  Serial.println(topic1);
  Serial.println(payload1);
  Serial.println(len);
  strncpy(topic, topic1, 49);
  if (len < 99) {
    strncpy(payload, payload1, len);
    payload[len] = '\0';
    validData = 1;
  }
}

void parse() {
  if (validData != 1) {
    return;
  }

  validData = 0;
  //    Serial.println(topic);
  //    Serial.println(payload);

  if ( strcmp(topic, (hostname + String("/setpoint/set")).c_str()) == 0) {
    debug.print("setpoint ");
    debug.println(payload);
    setpointVal(payload);
  }
  if ( strcmp(topic, (hostname + String("/state/set")).c_str()) == 0) {
    debug.print("state ");
    debug.println(payload);
    stateControl(payload);
  }
  if ( strcmp(topic, (hostname + String("/fanmode/set")).c_str()) == 0) {
    debug.print("fanmode ");
    debug.println(payload);
    fanControl(payload);
  }
  if ( strcmp(topic, (hostname + String("/swingmode/set")).c_str()) == 0) {
    debug.print("swingmode ");
    debug.println(payload);
    swingControl(payload);
  }
  if ( strcmp(topic, (hostname + String("/mode/set")).c_str()) == 0) {
    debug.print("mode ");
    debug.println(payload);
    modeControl(payload);
  }
  if ( strcmp(topic, (hostname + String("/doinit")).c_str()) == 0) {
    debug.print("doinit ");
    debug.println(payload);
    doinit();
  }
  if ( strcmp(topic, (hostname + String("/restart")).c_str()) == 0) {
    debug.print("restart ");
    debug.println(payload);
    ESP.restart();
  }
  if ( strcmp(topic, (hostname + String("/watchdog")).c_str()) == 0) {
    debug.print("watchdog ");
    debug.println(payload);
  }
  if ( strcmp(topic, (hostname + String("/timer")).c_str()) == 0) {
    debug.print("timer ");
    offtimer = millis();
  }
}

void onMqttPublish(uint16_t packetId) {
  debug.print("Publish acknowledged.   packetId: ");
  debug.println(packetId);
}

void MQTTSend (String topic, String payload) {
  uint8_t Length;
  Length = payload.length();
  mqttClient.publish(topic.c_str(), 1, true, payload.c_str(), Length);
}

void swingControl(char message[]) {
  int function_code = 163;
  int function_value = swingtoint(message);
  int rcv_code = 17;
  send_code(function_code, function_value, rcv_code);
}

void modeControl(char message[]) {
  int function_code = 176;
  int function_value = modetoint(message);
  if (function_value != 0) {
    int rcv_code = 4;
    send_code(function_code, function_value, rcv_code);
  }
}


void fanControl(char message[]) {
  int function_code = 160;
  int function_value = fanmodetoint(message);
  int rcv_code = 20;
  send_code(function_code, function_value, rcv_code);
}

void stateControl(char message[]) {
  int function_code = 128;
  int function_value = statetoint(message);
  int rcv_code = 52;
  send_code(function_code, function_value, rcv_code);
}

void setpointVal(char message[]) {
  int function_code = 179;
  int function_value = atoi(message);
  int rcv_code = 1;
  send_code(function_code, function_value, rcv_code);
}

void doinit() {
  getcode(128, 52); //state
  getcode(176, 4); //on/off
  getcode(179, 1); //setpoint
  getcode(160, 20); //fanmode
  getcode(135, 45); //??
  getcode(163, 17); //swing mode
  getcode(187, 249); //roomtemp
  getcode(190, 246); //out temp
  getcode(203, 233); //??
  getcode(136, 44); //??
  //getcode(134,46);
  getcode(144, 36); //??
  getcode(148, 32); //??
}

void dowatchdog() {
  getcode(187, 249); //roomtemp
  getcode(190, 246); //out temp
  getcode(128, 52); //state
  getcode(176, 4); //on/off
  getcode(179, 1); //setpoint
  getcode(160, 20); //fanmode
  getcode(163, 17); //swing mode
}

void getcode(int function_code, int rcv_code) {
  int getlist[14] = {2, 0, 3, 16, 0, 0, 6, 1, 48, 1, 0, 1, 0, 0};
  getlist[12] = function_code;
  getlist[13] = rcv_code;
  report(getlist, sizeof(getlist) / sizeof(getlist[0]));
  delay(100);
}

void send_code(int function_code, int function_value, int rcv_code) {
  int mylist[15] = {2, 0, 3, 16, 0, 0, 7, 1, 48, 1, 0, 2, 0, 0, 0};
  mylist[12] = function_code;
  mylist[13] = function_value;
  mylist[14] = checksum(function_value, function_code);
  report(mylist, sizeof(mylist) / sizeof(mylist[0]));
  getcode(function_code, rcv_code);
}

void report(int msg[], int siz) {
  debug.print("*O*");
  for (int i = 0; i < siz; i++) {
    Serial.write(msg[i]);
    debug.print(msg[i], DEC);
    debug.print('.');
  }
  debug.println("*O*");
  delay(100);
}
