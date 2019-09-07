#include <ArduinoJson.h>
#include "debugutils.h"
#include <Int64String.h>
#include "SoftwareSerial.h"
#include "myFunctions.h"
#include "nextion_ser.h"
#include "myIP.h"
#include "irnextion.h"
#include "password.h"
#include "topic.h"
#include <ESP8266WiFi.h>
#include <crash.h>
SoftwareSerial mydbSerial(4, 5); // RX, TX
const uint16_t versione = 36;
const char* mqttId="Chrono";
void spegniChr();
void checkForUpdates();
void reconnect();
void checkConn();
void callback(char* topic, byte* payload, unsigned int length);