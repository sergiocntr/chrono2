#include <ArduinoJson.h>
#include <Int64String.h>
#include "SoftwareSerial.h"
#include "nextion_ser.h"
#include "myIP.h"
#include "irnextion.h"
#include "password.h"
#include "topic.h"
#include <ESP8266WiFi.h>
SoftwareSerial mydbSerial(4, 5); // RX, TX
const uint16_t versione = 26;
const char* mqttId="Chrono";
//uint8_t check=0;
//long lastMsg = 0;
//char msg[50];
//int value = 0;
void spegniChr();
void checkForUpdates();
void reconnect();
void callback(char* topic, byte* payload, unsigned int length);