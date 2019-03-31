#include <ArduinoJson.h>
#include "SoftwareSerial.h"
SoftwareSerial mydbSerial(4, 5); // RX, TX
#include "nextion_ser.h"
#include "myIP.h"
#include "irnextion.h"
#include "password.h"
#include "topic.h"
#include <ESP8266WiFi.h>
const uint16_t versione = 6;
const char* mqttId="Chrono";
uint8_t check=0;
long lastMsg = 0;
char msg[50];
int value = 0;
uint8_t mqtt_reconnect_tries = 0;
unsigned long wifi_reconnect_time;
unsigned long wifi_check_time = 6000L;
//IPAddress ip(192, 168, 1, 49);
void setup() {
  nex_routines();
  String clientId = String(mqttId);
  clientId += String(random(0xffff), HEX);
  yield();
  setIP(ipChrono,clientId.c_str());
  //int8_t checkmio=0;
  //checkmio = connectWiFi();
  connectWiFi();
  delay(10);
  //if(checkmio == 0){
    //DEBUG_PRINT("WIFI OK");
  //}
  yield();
  mqttId=clientId.c_str();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  delay(10);
  //checkmio = connectMQTT();
  connectMQTT();
  /*switch (checkmio) {
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
  irrecv.enableIRIn();  // Start the receiver
  wifi_reconnect_time = millis();
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
  fwURL.concat( mqttId );
  yield();
  String fwVersionURL = fwURL;
  fwVersionURL.concat( "/version.php" );
  //Serial.print( "Firmware version URL: " );
  //Serial.println( fwVersionURL );
  yield();
  String fwImageURL = fwURL;
  fwImageURL.concat( "/firmware.bin" );
  //Serial.print( "Firmware  URL: " );
  //Serial.prconst char* mqttID;intln( fwImageURL );
//#ifdef HTTP_ON
  yield();
  HTTPClient httpClient;
  httpClient.begin( c,fwVersionURL );
  int httpCode = httpClient.GET();
  if( httpCode == 200 ) {
    String newFWVersion = httpClient.getString();
    int newVersion = newFWVersion.toInt();
    Ntcurr.setText(newFWVersion.c_str());
    delay(1000);
    if( newVersion > versione ) {
    //  Serial.println( "Preparing to update" );
      t_httpUpdate_return ret = ESPhttpUpdate.update( c,fwImageURL );
      yield();
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
      delay(1000);
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
void reconnect() {
  if(client.connected()) {
    client.publish(logTopic, mqttId);
      // ... and resubscribe
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
  }
}
void loop() {
  //delay(10);
  if((millis() - wifi_reconnect_time) > wifi_check_time){    //se sono passati piu x secondi dall ultimo controllo
    wifi_reconnect_time = millis(); //questo è il tempo dell'ultimo controllo
    connectWiFi();
    delay(100);
    if (!client.connected()) {
      mqtt_reconnect_tries++;
      connectMQTT();
      reconnect();
    }else {
      mqtt_reconnect_tries=0;
    }
    if ((mqtt_reconnect_tries > 5) && (!client.connected())) spegniChr();
  }
  if (irrecv.decode(&results)) {
    switch (results.value) {
      case spegni:
        send(teleTopic,"spegni");
        break;
      case acquaON:
        send(acquaTopic,"1");
        break;
    }
    //if(results.value==spegni){
    //  send(teleTopic,"spegni");
    //}//
    irrecv.resume();  // Receive the next value
  }
  smartDelay(200);
}
void callback(char* topic, byte* payload, unsigned int length){
  check=0;
  if(strcmp(topic,updateTopic) == 0){
    delay(10);
    check=1;
    Ntcurr.setText("UPMQ");
    checkForUpdates();
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
  yield();
  String message = String();
  char jsonChar[100];
  for (unsigned int i = 0; i < length; i++) {  //A loop to convert incomming message to a String
    char input_char = (char)payload[i];
    message += input_char;
  }
  yield();
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
