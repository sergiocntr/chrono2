#pragma once
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include "FS.h"
#include <Int64String.h>
#include "SoftwareSerial.h"
#include "EspSaveCrash.h"
//#include "nextion_ser.h"
#include "myIP.h"
#include "irnextion.h"
#include "password.h"
#include "topic.h"
#include <ESP8266WiFi.h>
#include "SPI.h"               //package builtin configuration file
#include "SD.h"               //package builtin configuration file
#include "Nextion.h"
uint8_t db_array_value[3] = {0};
char buffer[15]={0};
NexText Nset_temp         = NexText(0, 2, "Nset_temp");
NexText Ntcurr            = NexText(0, 3, "Ntcurr");
NexText Nout_temp         = NexText(0, 4, "Nout_temp");
NexCrop Nwater_on         = NexCrop(0, 5, "Nwater_on");
NexText Nout_hum          = NexText(0, 6, "Nout_hum");
NexText Nin_hum           = NexText(0, 7, "Nin_hum");
NexText Ncurr_hour        = NexText(0, 8, "Ncurr_hour");
NexText Ncurr_water_temp  = NexText(0, 11, "Nwater_temp");
NexText Nday              = NexText(0, 12, "Nday");
NexButton Nb_up           = NexButton(0, 9, "Nb_up");
NexButton Nb_down         = NexButton(0, 10, "Nb_down");
NexCrop Nrisc_on          = NexCrop(0, 13, "Nrisc_on");
NexCrop Nalarm            = NexCrop(0, 14, "Nalarm");
NexTouch *nex_listen_list[] ={
  &Nrisc_on,
  &Nwater_on,
  &Nb_up,
  &Nb_down,
  NULL
};
EspSaveCrash SaveCrash;
//SoftwareSerial mydbSerial(4, 5); // RX, TX
WiFiClient mywifi;
WiFiClient c;
PubSubClient client(c);
const uint16_t versione = 42;
uint8_t mqtt_reconnect_tries=0;
uint8_t mqttOK=0;
uint32_t wifi_initiate =0;
uint32_t wifi_check_time=600000L;
const char* mqttId="Chrono";
void spegniChr();
void checkForUpdates();
void reconnect();
void checkConn();
void callback(char* topic, byte* payload, unsigned int length);
void nex_routines();
void Nwater_onPushCallback(void *ptr);
void Nrisc_onPushCallback(void *ptr);
void Nb_downPushCallback(void *ptr);
void Nb_upPushCallback(void *ptr);
void update_buttons();
uint8_t toggle_button(int value);
void stampaDebug(int8_t intmess);
void smartDelay(unsigned long mytime);
void handleCrash();
void sendCrash();