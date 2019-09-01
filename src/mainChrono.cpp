#include <mainChrono.h>
void setup() {
  #ifdef DEBUGMIO
    Serial.begin(9600);
    delay(3000);
    //DEBUG_PRINT("Booting");
  #else
    nex_routines();
  #endif
  
  yield();
  setIP(ipChrono,chronoId);
  int8_t checkmio=0;
  checkmio = connectWiFi();
  //DEBUG_PRINT("Wifi: " +String(checkmio));
  delay(10);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  delay(10);
  checkmio = connectMQTT();
  //DEBUG_PRINT("MQTT: "+ String(checkmio));
  reconnect();
  irrecv.enableIRIn();  // Start the receiver
  wifi_reconnect_time = millis();
  wifi_check_time = 60000;
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
  if(client.connected()) {
    client.publish(logTopic, mqttId);
      // ... and resubscribe
    client.subscribe(systemTopic);
    client.loop();
    client.subscribe(casaSensTopic);
    //if(check)   //DEBUG_PRINT("casaSensTopic OK");
    client.loop();
    uint8_t check =client.subscribe(extSensTopic);
    //DEBUG_PRINT("extSensTopic OK");
    client.loop();
    client.subscribe(acquaTopic);
    //if(check)   //DEBUG_PRINT("acquaTopic OK");
    client.loop();
    client.subscribe(riscaldaTopic);
    //if(check)   //DEBUG_PRINT("riscaldaTopic OK");
    check = client.subscribe(updateTopic);
    client.loop();
    delay(10);
    if(check==0) {
      sendCommand("sleep=0");
      stampaDebug(3);
      //DEBUG_PRINT("sleep 0");
    }
    else stampaDebug(2);
  }
}
void loop() {
  smartDelay(1000);
  if((millis() - wifi_reconnect_time) > wifi_check_time){    //se sono passati piu x secondi dall ultimo controllo
    wifi_reconnect_time = millis(); //questo è il tempo dell'ultimo controllo
    connectWiFi();
    delay(100);
    if (!client.connected()){
      mqtt_reconnect_tries++;
      connectMQTT();
      reconnect();
      wifi_check_time = 15000; //ogni 15 secondi
    }else {
      mqtt_reconnect_tries=0;
      wifi_check_time = 60000; //ogni 1 minuto
    }
    if ((mqtt_reconnect_tries > 2) && (!client.connected())) spegniChr();
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