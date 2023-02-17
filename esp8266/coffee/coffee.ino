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
#include <AsyncMqttClient.h>
#include <AutoConnect.h>
#include <TelnetStream.h>
#include <queue>

#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#define debug TelnetStream
const char *hostname = "coffeemaker";


//SoftwareSerial to read Display TX (D5 = RX, D6 = unused)
SoftwareSerial swSer (D2, D1); // RX, TX

#define PARAM_FILE      "/param.json"
#define AUX_SETTING_URI "/mqtt_setting"
#define AUX_TEST_URI    "/test"

// Adjusting WebServer class with between ESP8266 and ESP32.
#if defined(ARDUINO_ARCH_ESP8266)
typedef ESP8266WebServer  WiFiWebServer;
#elif defined(ARDUINO_ARCH_ESP32)
typedef WebServer WiFiWebServer;
#endif

#define GND_BREAKER_PIN D5
bool displayOff = false;

AsyncMqttClient mqttClient;
ESP8266WebServer server(80);

//Definition of functions:
void serialSend(const byte command[], int sendCount);
void redirect(String uri);
// current status
const size_t ser_buf_size = 18;
char buff[ser_buf_size+2];

std::queue<std::array<byte, 12>> command_queue;
unsigned long lastPush = 0;

/*

1 espresso shot + water 75*

 55:on:00 es:00 hw:00 co:00 ca:38 am:00 st:38 gr:07 wl:38 ??:07 ac:00 cl:00 wt:00 er:00 br:00 **:2a


----------------------------------

2 espresso shots + water 75*

received d5 55:on:00 es:00 hw:00 co:00 ca:00 am:07 st:38 gr:07 wl:38 ??:07 ac:00 cl:00 wt:00 er:00 br:00 **:26


----

cappuccino 

received d5 55:on:00 es:00 hw:00 co:00 ca:07 am:38 st:00 gr:38 wl:38 ??:07 ac:00 cl:00 wt:00 er:00 br:00 **:3b


----


received d5 55:on:00 es:00 hw:00 co:00 ca:3f am:00 st:38 gr:07 wl:38 ??:07 ac:00 cl:00 wt:00 er:00 br:07 **:3c

*/

//                         baseline
//                          d5    55    00    01    03    00    0d    00    00    00    02    12
//                          d5    55    00    01    03    00    0d    00    00    01    0a    16
byte requestInfo[] =     {0xD5, 0x55, 0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x14};
byte requestInfo2[] =    {0xD5, 0x55, 0x00, 0x01, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x35, 0x34};

//                          d5    55.   00    01    03    00    0d    01    00    00    0e    1f.

byte powerOff[] =        {0xd5, 0x55, 0x00, 0x01, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x0d, 0x19};
byte powerOff2[] =       {0xD5, 0x55, 0x00, 0x01, 0x03, 0x00, 0x0E, 0x01, 0x00, 0x00, 0x39, 0x39};

byte espresso[] =        {0xD5, 0x55, 0x00, 0x01, 0x00, 0x00, 0x02, 0x02, 0x00, 0x00, 0x19, 0x0F};
byte espresso2[] =       {0xD5, 0x55, 0x00, 0x01, 0x03, 0x00, 0x0E, 0x02, 0x00, 0x00, 0x2D, 0x2F};

byte hotWater[] =        {0xD5, 0x55, 0x00, 0x01, 0x00, 0x00, 0x02, 0x04, 0x00, 0x00, 0x31, 0x23};
byte cappuccino2[] =     {0xD5, 0x55, 0x00, 0x01, 0x03, 0x00, 0x0E, 0x04, 0x00, 0x00, 0x05, 0x03};

byte coffee[] =          {0xD5, 0x55, 0x00, 0x01, 0x00, 0x00, 0x02, 0x08, 0x00, 0x00, 0x29, 0x3E};
byte coffee2[] =         {0xd5, 0x55, 0x00, 0x01, 0x03, 0x00, 0x0e, 0x08, 0x00, 0x00, 0x1d, 0x1e};

//byte cappuccino[] =      {0xD5, 0x55, 0x00, 0x01, 0x00, 0x00, 0x02, 0x10, 0x00, 0x00, 0x19, 0x04};
byte cappuccino[] =      {0xD5, 0x55, 0x00, 0x01, 0x03, 0x00, 0x0d, 0x10, 0x00, 0x00, 0x1a, 0x02};
byte latteMacchiato2[] = {0xD5, 0x55, 0x00, 0x01, 0x03, 0x00, 0x0E, 0x10, 0x00, 0x00, 0x2D, 0x24};

byte unknown[] =         {0xD5, 0x55, 0x00, 0x01, 0x03, 0x00, 0x0E, 0x20, 0x00, 0x00, 0x04, 0x15};

byte americano[] =       {0xD5, 0x55, 0x00, 0x01, 0x03, 0x00, 0x0E, 0x00, 0x01, 0x00, 0x39, 0x38};
byte coffeePulver[] =    {0xD5, 0x55, 0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x02, 0x00, 0x19, 0x0D};
byte coffeePulver2[] =   {0xD5, 0x55, 0x00, 0x01, 0x03, 0x00, 0x0E, 0x00, 0x02, 0x00, 0x2D, 0x2D};
byte coffeeWater[] =     {0xD5, 0x55, 0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x04, 0x00, 0x30, 0x27};
byte coffeeWater2[] =    {0xD5, 0x55, 0x00, 0x01, 0x03, 0x00, 0x0E, 0x00, 0x04, 0x00, 0x04, 0x07};
byte coffeeMilk2[] =     {0xD5, 0x55, 0x00, 0x01, 0x03, 0x00, 0x0E, 0x00, 0x08, 0x00, 0x1F, 0x16};
byte aquaClean[] =       {0xD5, 0x55, 0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x10, 0x00, 0x1D, 0x14};
byte aquaClean2[] =      {0xD5, 0x55, 0x00, 0x01, 0x03, 0x00, 0x0E, 0x00, 0x10, 0x00, 0x29, 0x34};
byte calcNclean[] =      {0xD5, 0x55, 0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x20, 0x00, 0x38, 0x15};
byte calcNclean2[] =     {0xD5, 0x55, 0x00, 0x01, 0x03, 0x00, 0x0E, 0x00, 0x20, 0x00, 0x0C, 0x35};

//   startPause[] =      {0xD5, 0x55, 0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x01, 0x09, 0x10};
//   startPause[] =      {0xD5, 0x55, 0x00, 0x01, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x01, 0x3D, 0x30};
byte startPause[] =      {0xD5, 0x55, 0x00, 0x01, 0x03, 0x00, 0x0D, 0x00, 0x00, 0x01, 0x0A, 0x16};

//
//commands
//byte powerOn[] =       {0xd5, 0x55, 0x01, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x35, 0x05};
//byte powerOn2[] =      {0xD5, 0x55, 0x0A, 0x01, 0x03, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x2A, 0x10};
//                       {0xd5, 0x55, 0x0a, 0x01, 0x03, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x1d, 0x36};


char oldStatus[19] = "";


// Declare AutoConnectElements for the page asf /mqtt_setting
ACStyle(style, "label+input,label+select{position:sticky;left:120px;width:230px!important;box-sizing:border-box;}");
ACText(header, "<h2>MQTT Settings</h2>", "text-align:center;color:#2f4f4f;padding:10px;");
ACText(caption, "MQTT Server Settings. The following MQTT commands are availabe:<br><br>coffee/command/powerOn<br>coffee/command/powerOff<br>coffee/command/hotWater<br>coffee/command/espresso<br>coffee/command/coffee<br>coffee/command/steam<br>coffee/command/coffeePulver<br>coffee/command/coffeeWater<br>coffee/command/calcNclean<br>coffee/command/aquaClean<br>coffee/command/startPause<br><br>Send the command with a count of repeats as value (typical 5).", "font-family:serif;color:#4682b4;");
ACSubmit(discard, "Discard", "/");
ACElement(newline, "<hr>");



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


//Sends a command via serial to the coffee machine
void serialSend(const byte command[12], int sendCount) {
  debug.printf("Sending %d times: ", sendCount);
  for (int i = 0; i < 12; i++) {
    debug.printf("%02x ", command[i]);
  }
  debug.println();
  
  for (int i = 0; i <= sendCount; i++) {
    Serial.write(command, 12);
  }

}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  debug.begin(); // debug port for debugging purposes
  Serial.begin(115200); // Serial communication with coffee machine (Main Controller + ESP)
  swSer .begin(115200);  // Serial communication to read display TX and give it over to Serial
  Serial.setTimeout(100);
  swSer.setTimeout(100);

  SPIFFS.begin();

  
  WIFI_Connect();
  mqtt_init();
  mqtt_connect();
  otastart();
  
  webserver_init();
  
  pinMode(GND_BREAKER_PIN, OUTPUT);
  digitalWrite(GND_BREAKER_PIN, HIGH);
}

void loop() {
  mqtt_connect();
  WIFI_Connect();
  ArduinoOTA.handle();
  server.handleClient();
  MDNS.update();

  swSerialInput2Mqtt();
  serialInput2Mqtt();
  if (displayOff) {
    displayOff = false;
    delay(300);
    digitalWrite(GND_BREAKER_PIN, HIGH);
    debug.println("Display re-enabled");
      
    lastPush = millis();
  }
  delay(5);
}