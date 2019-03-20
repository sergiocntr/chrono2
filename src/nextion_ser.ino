//#define DEBUGMIO

//collega il pin lontano dall'usb
#include "myIP.h"
#include "debugNex.h"
#include "SPI.h"               //package builtin configuration file
#include "SD.h"               //package builtin configuration file
#include "topic.h"
#include "myFunctions.h"
#include <Arduino.h>
#include <ESP8266WiFi.h>          //builtin library for ESP8266 Arduino Core
#include "Nextion.h"
#include <ArduinoOTA.h>
/*************************************************************************************************************************
  Database variables
*************************************************************************************************************************/
uint8_t db_array_value[3] = {0};
char buffer[15]={0};
unsigned long wifi_check_time = 30000L;
unsigned long wifi_reconnect_time;
unsigned long mqtt_reconnect_time;
uint8_t mqtt_reconnect_tries=0;
uint8_t check=0;
bool MQTTLOG = false;
NexText Nset_temp   = NexText(0, 2, "Nset_temp");
NexText Ntcurr      = NexText(0, 3, "Ntcurr");
NexText Nout_temp   = NexText(0, 4, "Nout_temp");
NexCrop Nwater_on   = NexCrop(0, 5, "Nwater_on");
NexText Nout_hum    = NexText(0, 6, "Nout_hum");
NexText Nin_hum     = NexText(0, 7, "Nin_hum");
NexText Ncurr_hour  = NexText(0, 8, "Ncurr_hour");
NexButton Nb_up     = NexButton(0, 9, "Nb_up");
NexButton Nb_down   = NexButton(0, 10, "Nb_down");
NexText Ncurr_water_temp  = NexText(0, 11, "Nwater_temp");
NexText Nday  = NexText(0, 12, "Nday");
NexCrop Nrisc_on    = NexCrop(0, 13, "Nrisc_on");
NexCrop Nalarm    = NexCrop(0, 14, "Nalarm");

NexTouch *nex_listen_list[] ={
  &Nrisc_on,
  &Nwater_on,
  &Nb_up,
  &Nb_down,
  NULL
};

void setup() {
  #ifdef DEBUGMIO
    delay(3000);
    mydbSerial.begin(9600);
    DEBUG_PRINT("Booting!!");
  #endif
  nex_routines();
  delay(10);
  setIP(ipChrono,chronoId);
  client.setCallback(callback);
  wifi_reconnect_time = millis();
  mqtt_reconnect_time = millis();
  check  = connectWiFi();
  delay(1000);
  check=connectMQTTmia();
  delay(1000);
  client.loop();
  setupOTA();
}
void smartDelay(unsigned long mytime){
  unsigned long adesso = millis();
  while((millis()-adesso)<mytime){
    client.loop();
    nexLoop(nex_listen_list);
  }
}
void keeplive(){
    check=0;

  if((millis() - wifi_reconnect_time) > wifi_check_time){//se sono passati piu x secondi dall ultimo controllo
    //DEBUG_PRINT("Controllo WiFi");
    client.publish(logTopic, "WIFI OK");
    wifi_reconnect_time = millis(); //questo è il tempo dell'ultimo controllo
    if(WiFi.isConnected()) {
      //visMes("WOK");
      wifi_check_time = 15000L; //se sono connesso,controllo ogni 15 secondi che la connessione ci sia
    }else { //se non connesso cerca di connettere e collegarti a MQTT
      check=connectWiFi();
      delay(1000);
    }
    smartDelay(100);
    client.loop();
    if (client.connected()==0){ //MQTT non è collegato 0 = f
      //DEBUG_PRINT("MQTT KO!! " + String(client.connected()));
      mqtt_reconnect_tries++;
      smartDelay(100);
      connectMQTTmia();
    }
    else {
      mqtt_reconnect_tries=0; //se collegato azzero
      //DEBUG_PRINT("MQTT OK!!");
      if(MQTTLOG) client.publish(logTopic, "MQTT OK");
    }
    if(mqtt_reconnect_tries > 3){
      wifi_check_time = 300000L;
      smartDelay(100);
      spegniChr();
    }
  }
}
int8_t connectMQTTmia(){
  if(client.state()==0){
    //DEBUG_PRINT("MQTT_ALREADY_CONNECTED");
    return 0;
  }
  //DEBUG_PRINT("MQTT_inizio");
  client.setServer(mqtt_server, mqtt_port);
  //DEBUG_PRINT("MQTT id: " + String(mqttID));
  delay(10);
  int check =0;
  //for (char i = 0; i < 10; i++)
  //{
    //DEBUG_PRINT("Attempting MQTT connection...");
  if (client.connect(mqttID,mqttUser,mqttPass)){
    //DEBUG_PRINT("connected");
    delay(10);
    client.publish(logTopic, mqttID);
    resub();
    client.loop();
    client.publish(logTopic, "publish fatto");
  }
    /*else
    {
      check = client.state();
      switch (check) {
          case -4:
            DEBUG_PRINT("MQTT_CONNECTION_TIMEOUT");
            break;
          case -3:
            DEBUG_PRINT("MQTT_CONNECTION_LOST");
            break;
          case -2:
            DEBUG_PRINT("MQTT_CONNECT_FAILED");
            break;
          case -1:
            DEBUG_PRINT("MQTT_DISCONNECTED");
            break;
          case 0:
            DEBUG_PRINT("MQTT_CONNECTED");
            break;
          case 1:
            DEBUG_PRINT("MQTT_CONNECT_BAD_PROTOCOL");
            break;
          case 2:
            DEBUG_PRINT("MQTT_CONNECT_BAD_CLIENT_ID");
            break;
          case 3:
            DEBUG_PRINT("MQTT_CONNECT_UNAVAILABLE");
            break;
          case 4:
            DEBUG_PRINT("MQTT_CONNECT_BAD_CREDENTIALS");
            break;
          case 5:
            DEBUG_PRINT("MQTT_CONNECT_UNAUTHORIZED");
            break;
          default:
            DEBUG_PRINT("MQTT_UNKNOWN_ERROR");
            break;
        }*/
      //DEBUG_PRINT(" try again in 5 seconds");
      //delay(5000);
    //}
  //}
  return check;
}
void loop(){
  smartDelay(3000);
  keeplive();
}
void spegniChr(){
  //DEBUG_PRINT("Spengo Nextion");
  sendCommand("thup=1");
  sendCommand("sleep=1");
  smartDelay(100);

}
void nex_routines(){
  nexInit();
  smartDelay(100);
  sendCommand("dim=20");
  Nrisc_on.attachPush(Nrisc_onPushCallback);
  Nwater_on.attachPush(Nwater_onPushCallback);
  Nb_up.attachPush(Nb_upPushCallback);
  Nb_down.attachPush(Nb_downPushCallback);
  update_buttons();
}
uint8_t resub() {
      check =client.subscribe(systemTopic);
      //if(check)   visMes("1--");//DEBUG_PRINT("systemTopic OK");
      client.loop();
      client.subscribe(casaSensTopic);
      //if(check)   DEBUG_PRINT("casaSensTopic OK");
      client.loop();
      client.subscribe(extSensTopic);
      //if(check)   DEBUG_PRINT("extSensTopic OK");
      client.loop();
      client.subscribe(acquaTopic);
      //if(check)   DEBUG_PRINT("acquaTopic OK");
      client.loop();
      client.subscribe(riscaldaTopic);
      //if(check)   DEBUG_PRINT("riscaldaTopic OK");
      client.loop();
      delay(10);

}
void callback(char* topic, byte* payload, unsigned int length){
  bool trovatoTopic=false;
  #ifdef DEBUGMIO
  DEBUG_PRINT("Message arrived [" + String(topic) + "]");
  #endif
  if(strcmp(topic,"MQTTUPDATE") == 0){

    doOTA();
    trovatoTopic=true;
  }
  if(strcmp(topic, acquaTopic) == 0 ) {
    trovatoTopic=true;
    if (char(payload[0]) == '0') {
      //DEBUG_PRINT(db_array_value[2]);
      db_array_value[2] = 0;
      Nwater_on.setPic(0);
      //const char* Nex_Day = doc["A_0"];
      //DEBUG_PRINT("Acqua Spenta");

    }else{
      db_array_value[2] = 1;
      Nwater_on.setPic(1);
      //DEBUG_PRINT("Acqua Accesa");
      //const char* Nex_Day = doc["A_1"];
    }
  }
  else if(strcmp(topic, riscaldaTopic) == 0 ) {
    trovatoTopic=true;
    if (char(payload[0]) == '0') {
      db_array_value[1] = 0;
      Nrisc_on.setPic(0);
      //DEBUG_PRINT("Risc Accesa");
      //const char* Nex_Day = doc["R_1"];
    }else{
      db_array_value[1] = 1;
      Nrisc_on.setPic(1);
      //DEBUG_PRINT("Risc Spento");
      //const char* Nex_Day = doc["R_0"];
    }
  }
  if(trovatoTopic)  return;
  smartDelay(100);
  const int capacity = 1000;
  StaticJsonDocument<capacity> doc;
  smartDelay(100);
  DeserializationError err = deserializeJson(doc, payload);
  smartDelay(100);
  if (err) {
    //non oggetto JSON
    //DEBUG_PRINT("DeserializeJson() failed with code " + String(err.c_str()) );

    smartDelay(100);
  }else{
  const char* msg_Topic = doc["topic"];
  //DEBUG_PRINT("inTopic " + String(msg_Topic) );
  if(strcmp(topic, systemTopic) == 0 ) {

    if(strcmp(msg_Topic, "UpTime") == 0) {
      smartDelay(100);
      const char* Nex_Time = doc["hours"];
      const char* Nex_Day = doc["Day"];
      Ncurr_hour.setText(Nex_Time);
      Nday.setText(Nex_Day);
    }
  }
  else if(strcmp(topic, casaSensTopic) == 0 ) {
    if(strcmp(msg_Topic, "DHTCamera") == 0) {
      smartDelay(100);
      const char* Nex_inHm = doc["Hum"];
      const char* Nex_inTemp = doc["Temp"];
      Ntcurr.setText(Nex_inTemp);
      Nin_hum.setText(Nex_inHm);
    }
  }
  else if(strcmp(topic, extSensTopic) == 0 ) {
    if(strcmp(msg_Topic, "Caldaia") == 0) {
      smartDelay(100);
      //DEBUG_PRINT("acqua [" + String(topic) + "]");
      auto Nex_wt = doc["acqua"].as<float>();
      //char buffer[8];
      int ret = snprintf(buffer, sizeof buffer, "%f", Nex_wt);
      if (ret < 0) {
          //DEBUG_PRINT("Qualcosa di storto uffa");
      }
      //if (ret >= sizeof buffer) {
          //DEBUG_PRINT("Buffer troppo corto");
      //}
      //const char* Nex_wt = doc["acqua"];
      //DEBUG_PRINT("acqua [" + String(Nex_wt) + "]");
      Ncurr_water_temp.setText(buffer);
    }
    else if(strcmp(msg_Topic, "Terrazza") == 0){
      smartDelay(100);
      const char* Nex_outHm = doc["Hum"];
      const char* Nex_outTemp = doc["Temp"];
      Nout_temp.setText(Nex_outTemp);
      Nout_hum.setText(Nex_outHm);
  }
}
}
}
/*
  char jsonChar[100];
  StaticJsonBuffer<100> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(payload);

  if(strcmp(topic, systemTopic) == 0 ) {
    String msg_Topic = root["topic"];
    if(msg_Topic == "UpTime") {
      const char* Nex_Time = root["hours"];
      const char* Nex_Day = root["Day"];
      Ncurr_hour.setText(Nex_Time);
      Nday.setText(Nex_Day);
    }
  }
  else if(strcmp(topic, casaSensTopic) == 0 ) {
    String msg_Topic = root["topic"];
    if(msg_Topic == "DHTCamera") {
      const char* Nex_inHm = root["Hum"];
      const char* Nex_inTemp = root["Temp"];
      Ntcurr.setText(Nex_inTemp);
      Nin_hum.setText(Nex_inHm);
    }
  }
  else if(strcmp(topic, extSensTopic) == 0 ) {
    String msg_Topic = root["topic"];
    if(msg_Topic == "Caldaia") {
      const char* Nex_wt = root["acqua"];
      Ncurr_water_temp.setText(Nex_wt);
    }
    else if (msg_Topic == "Terrazza"){
      const char* Nex_outHm = root["Hum"];
      const char* Nex_outTemp = root["Temp"];
      Nout_temp.setText(Nex_outTemp);
      Nout_hum.setText(Nex_outHm);
    }
  }
  else if(strcmp(topic, acquaTopic) == 0 ) {
    if (char(payload[0]) == '0') {
      DEBUG_PRINT(db_array_value[2]);
      db_array_value[2] = 0;
      Nwater_on.setPic(0);

    }else{
      db_array_value[2] = 1;
      Nwater_on.setPic(1);
    }
  }
  else if(strcmp(topic, riscaldaTopic) == 0 ) {
    if (char(payload[0]) == '0') {
      db_array_value[1] = 0;
      Nrisc_on.setPic(0);

    }else{
      db_array_value[1] = 1;
      Nrisc_on.setPic(1);
    }
  }

}
*/
/*************************************************************************************************************************
  NEXTION
*************************************************************************************************************************/
void update_buttons(){
    Nrisc_on.setPic(db_array_value[1]);
    Nwater_on.setPic(db_array_value[2]);
    //Nt_up.setPic(db_array_value[3]);
    //Nt_down.setPic(db_array_value[4]);
}
void Nb_upPushCallback(void *ptr){
  double  number;
  memset(buffer, 0, sizeof(buffer));

  /* Get the text value of button component [the value is string type]. */
  Nset_temp.getText(buffer, sizeof(buffer));
  number = strtod (buffer,NULL);
  number += 0.5;
  dtostrf(number, 4, 1, buffer);

  /* Set the text value of button component [the value is string type]. */
  Nset_temp.setText(buffer);
}
void Nb_downPushCallback(void *ptr){
    double  number;
    memset(buffer, 0, sizeof(buffer));
    Nset_temp.getText(buffer, sizeof(buffer));

    number = strtod (buffer,NULL);
    number -= 0.5;
    dtostrf(number, 4, 1, buffer);
    Nset_temp.setText(buffer);
}
void Nrisc_onPushCallback(void *ptr){
    uint32_t number = toggle_button(1);
    //Nrisc_on.setPic(number);
    if (number == 0) {
      send(riscaldaTopic, "0");
    } else {
      send(riscaldaTopic, "1");
    }
}
void Nwater_onPushCallback(void *ptr){
  uint32_t number = toggle_button(2);
    Nwater_on.setPic(number);
    if (number == 0) {
      send(acquaTopic, "0");
    } else {
      send(acquaTopic, "1");
    }
}
uint8_t toggle_button(int value){
  if (db_array_value[value] == 1) {
    db_array_value[value] = 0;
    return 0;
  } else {
    db_array_value[value] = 1;
    return 1;
  }
}
