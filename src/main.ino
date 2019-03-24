#include <ArduinoJson.h>
#include "nextion_ser.h"
#include "password.h"
#include "topic.h"
#include <ESP8266WiFi.h>
const uint16_t versione = 2;
const char* mqttID="Chrono";
uint8_t check=0;
long lastMsg = 0;
char msg[50];
int value = 0;
uint8_t mqtt_reconnect_tries = 0;
unsigned long wifi_reconnect_time;
unsigned long wifi_check_time = 6000L;;
IPAddress ip(192, 168, 1, 49);
void setup() {
  nex_routines();
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void setup_wifi() {
  WiFi.mode(WIFI_STA);
  WiFi.forceSleepWake();
  WiFi.config(ip, gateway, subnet,dns1);
  delay(500);
  wifi_reconnect_time = millis();
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    if ((millis() - wifi_reconnect_time) > wifi_check_time) {
      spegniChr();
    }
  }
}
void spegniChr(){
  wifi_check_time = 600000L;
  sendCommand("thup=1");
  sendCommand("sleep=1");
  wifi_set_sleep_type(LIGHT_SLEEP_T);
  WiFi.mode(WIFI_OFF); //energy saving mode if local WIFI isn't connected
  WiFi.forceSleepBegin();
  delay(1000);
}
void checkForUpdates() {
  client.disconnect();
  smartDelay(1000);
  String fwURL = String( fwUrlBase );
  fwURL.concat( mqttID );
  String fwVersionURL = fwURL;
  fwVersionURL.concat( "/version.php" );
  //Serial.print( "Firmware version URL: " );
  //Serial.println( fwVersionURL );

  String fwImageURL = fwURL;
  fwImageURL.concat( "/firmware.bin" );
  //Serial.print( "Firmware  URL: " );
  //Serial.prconst char* mqttID;intln( fwImageURL );
//#ifdef HTTP_ON
  HTTPClient httpClient;
  httpClient.begin( c,fwVersionURL );
  int httpCode = httpClient.GET();
  if( httpCode == 200 ) {
    String newFWVersion = httpClient.getString();
    int newVersion = newFWVersion.toInt();

    if( newVersion > versione ) {
    //  Serial.println( "Preparing to update" );
      t_httpUpdate_return ret = ESPhttpUpdate.update( c,fwImageURL );
      switch(ret) {
        case HTTP_UPDATE_FAILED:
          Ntcurr.setText("U_F");
          //check=1; //Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
          break;

        case HTTP_UPDATE_NO_UPDATES:
          Ntcurr.setText("N_U");
          //check=2;//Serial.println("HTTP_UPDATE_NO_UPDATES");
          break;
        case HTTP_UPDATE_OK:
          //Serial.println("[update] Update ok."); // may not called we reboot the ESP
          break;
      }
    }
    else {
      Ntcurr.setText("S_V");
      //check=0;//Serial.println( "Already on latest version" );
    }
  }
  else {
    //Serial.print( "Firmware version check failed, got HTTP response code " );
    //Serial.println( httpCode );
    //check= httpCode;
    Ntcurr.setText("A_E");
  }
  httpClient.end();
  //return check;
}
/*void callbackold(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();



}
void callback(char* topic, byte* payload, unsigned int length){
  check=0;
  if(strcmp(topic,updateTopic) == 0){
    delay(10);
    check=1;
    Ntcurr.setText("UPMQ");
    checkForUpdates();
    Ntcurr.setText("----");

  }
  else if(strcmp(topic, acquaTopic) == 0 ) {
    check=1;
    if (char(payload[0]) == '0') {
      //DEBUG_PRINT(db_array_value[2]);
      db_array_value[2] = 0;
      Nwater_on.setPic(0);

    }else{
      db_array_value[2] = 1;
      Nwater_on.setPic(1);
    }
  }
  else if(strcmp(topic, riscaldaTopic) == 0 ) {
    check=1;
    if (char(payload[0]) == '0') {
      db_array_value[1] = 0;
      Nrisc_on.setPic(0);

    }else{
      db_array_value[1] = 1;
      Nrisc_on.setPic(1);
    }
  }
  if(check) return;
  //char jsonChar[100];
  //message.toCharArray(jsonChar, message.length()+1);
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
  delay(10);
  client.loop();
}*/
void reconnect() {
  String clientId = "ESP8266Client-";
  clientId += String(random(0xffff), HEX);
  while (!client.connected()) {
    //Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(clientId.c_str(),mqttUser,mqttPass)) {
      //Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(logTopic, "hello world");
      // ... and resubscribe
      //client.subscribe("inTopic");
      client.subscribe(systemTopic);
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
      client.subscribe(updateTopic);
      client.loop();
      delay(10);
    } else {
      //Serial.print("failed, rc=");
      //Serial.print(client.state());
      //Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void loop() {
  if((millis() - wifi_reconnect_time) > wifi_check_time){//se sono passati piu x secondi dall ultimo controllo
    wifi_reconnect_time = millis(); //questo è il tempo dell'ultimo controllo
    if (!client.connected()) {
      mqtt_reconnect_tries++;
      reconnect();
    }else mqtt_reconnect_tries=0;
  }
  smartDelay(2000);
  if ((mqtt_reconnect_tries > 5) && (!client.connected())) spegniChr();
}
void callback(char* topic, byte* payload, unsigned int length){
  check=0;
  if(strcmp(topic,updateTopic) == 0){
    delay(10);
    check=1;
    Ntcurr.setText("UPMQ");
    checkForUpdates();
    Ntcurr.setText("----");


  }
  else if(strcmp(topic, acquaTopic) == 0 ) {
    check=1;
    if (char(payload[0]) == '0') {
      //DEBUG_PRINT(db_array_value[2]);
      db_array_value[2] = 0;
      Nwater_on.setPic(0);

    }else{
      db_array_value[2] = 1;
      Nwater_on.setPic(1);
    }
  }
  else if(strcmp(topic, riscaldaTopic) == 0 ) {
    check=1;
    if (char(payload[0]) == '0') {
      db_array_value[1] = 0;
      Nrisc_on.setPic(0);

    }else{
      db_array_value[1] = 1;
      Nrisc_on.setPic(1);
    }
  }
  if(check) return;
  String message = String();
  char jsonChar[100];
  for (unsigned int i = 0; i < length; i++) {  //A loop to convert incomming message to a String
    char input_char = (char)payload[i];
    message += input_char;
  }
  message.toCharArray(jsonChar, message.length()+1);
  StaticJsonBuffer<100> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(jsonChar);
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
  delay(10);
  client.loop();
}
