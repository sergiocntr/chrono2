//#define DEBUGMIO
#include <mainChrono.h>
void setup() {
  WiFi.mode(WIFI_OFF);
  delay(10);
  handleCrash();
  #ifdef DEBUGMIO
  //
    setIP(ipChronoProva,provaId);
    Serial.begin(9600);
    delay(3000);
    //DEBUG_PRINT("Booting");
  #else
    setIP(ipChrono,chronoId);
    nex_routines();
    delay(10);
    irrecv.enableIRIn();  // Start the receiver
  #endif
  yield();
  int8_t checkmio=0;
  checkmio = connectWiFi();
  if(checkmio==0) sendCrash();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  connectMQTT();
  smartDelay(500);
  reconnect();
  wifi_reconnect_time=millis();
  
}
void spegniChr(){
  wifi_check_time = 300 * 1000; // 5 minuti
  sendCommand("thup=1");
  sendCommand("sleep=1");
  wifi_set_sleep_type(LIGHT_SLEEP_T);
  WiFi.mode(WIFI_OFF); //energy saving mode if local WIFI isn't connected
  WiFi.forceSleepBegin();
  delay(1000);
}
void checkForUpdates() {
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
  yield();
  WiFiClient myLocalConn;
  HTTPClient httpClient;
  httpClient.begin( myLocalConn,fwVersionURL );
  int httpCode = httpClient.GET();
  if( httpCode == 200 ) {
    String newFWVersion = httpClient.getString();
    int newVersion = newFWVersion.toInt();
    Ntcurr.setText(newFWVersion.c_str());
    smartDelay(1000);
    if( newVersion > versione ) {
    //  Serial.println( "Preparing to update" );
      client.disconnect();
      smartDelay(1000);
      t_httpUpdate_return ret = ESPhttpUpdate.update( myLocalConn,fwImageURL );
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
      smartDelay(1000);
    }
    else {
      Ntcurr.setText("S_V");
    }
  }
  else {
    //Serial.print( "Firmware version check failed, got HTTP response code " );
    //Serial.println( httpCode );
    //check= httpCode;
    Ntcurr.setText("A_E");
  }
  httpClient.end();
  myLocalConn.stop();
  }
void reconnect() {
  client.publish(logTopic, "Crono - prova connesso");
  client.subscribe(systemTopic);
  client.subscribe(casaSensTopic);
  client.subscribe(extSensTopic);
  client.subscribe(acquaTopic);
  client.subscribe(riscaldaTopic);
  client.subscribe(updateTopic);
  client.loop();
}
void loop() {
  smartDelay(2000);
  if((millis() - wifi_reconnect_time) > wifi_check_time){ 
    DEBUG_PRINT("Controllo WIFI");
    wifi_reconnect_time=millis();
    if (client.state()!=0) {  // se non connesso a MQTT
      DEBUG_PRINT("MQTT NON VA");
      mqtt_reconnect_tries++;
      connectWiFi();    //verifico connessione WIFI
      delay(100);
      connectMQTT();
      smartDelay(500);
      reconnect();
      wifi_check_time = 20000; //venti secondi
    }else {
      DEBUG_PRINT("WIFI OK");
      mqtt_reconnect_tries=0;
      wifi_check_time = 300000; //ogni 5 minuti
    }
    if ((mqtt_reconnect_tries > 2) && (!client.connected())) spegniChr();  //cinque minuti
    
  }
  if (irrecv.decode(&results)) {
    uint64_t infraredNewValue = results.value;
    switch (infraredNewValue) {
      case spegni:
        client.publish(teleTopic, "spegni", false);
        //send(teleTopic,"spegni");
        break;
      case acquaON:
        send(acquaTopic,"1");
        break;
      case eneOff:
        send(eneTopic,"0");
        break;
      default:
        String s=int64String(infraredNewValue,HEX,false);
        send(iRTopic,s);
        break;
    }
    irrecv.resume();  // Receive the next value
  }
  //smartDelay(200);
}
void callback(char* topic, byte* payload, unsigned int length){
  #ifdef DEBUGMIO
  Serial.print("Message arrived [");  
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  #else
  smartDelay(200);
  uint8_t check=0;
  if(strcmp(topic,updateTopic) == 0){
    if (char(payload[0]) == '0') {
      delay(10);
      check=1;
      Ntcurr.setText("UPMQ");
      checkForUpdates();
    }
  }
  else if(strcmp(topic, acquaTopic) == 0 ) {
    check=1;
    if (char(payload[0]) == '0') {
      ////DEBUG_PRINT(db_array_value[2]);
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
  smartDelay(200);
  char *jsonChar = (char*)payload;
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
  smartDelay(200);
  #endif
  }