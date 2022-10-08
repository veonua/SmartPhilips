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
    debug.printf("%02x", command[i]);
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
}