#include <mainChrono.h>
/*void handleCrash(){
  bool result = SPIFFS.begin();
  //DEBUG_PRINT(" SPIFFS opened: " + String(result));
  //SaveCrash.print();
  File f = SPIFFS.open(crashFilename, "a+");
  f.println("crash chrono");
  SaveCrash.print(f);
  delay(100);
  SaveCrash.clear();
  f.close();
  delay(100); 
}
void sendCrash(){
  HTTPClient http;
  File f = SPIFFS.open(crashFilename, "r");
  if (f) 
  {
    uint16_t fileSize =f.size();
    //DEBUG_PRINT("File size: " + String(fileSize));
    if(fileSize<30) return;
    String line;
    while(f.available()) {
      //Lets read line by line from the file
        line =line + f.readStringUntil('\n') + '\n';
        delay(10);
      
      }
    f.close();
    SPIFFS.remove(crashFilename);
    delay(100);
    //DEBUG_PRINT("Stiamo per inviare: " + line);
    int httpResponseCode=0;
    c.connect(host, httpPort);
    delay(100);
    http.begin(c,post_serverCrash);
    http.addHeader("Content-Type","text/plain");
    httpResponseCode = http.PUT(line);
    String response = http.getString();                       //Get the response to the request
    //DEBUG_PRINT("Responso dal server: " + response);           //Print request answer
    //DEBUG_PRINT("Code dal server: " + String(httpResponseCode));
    delay(100);
    http.end();
    delay(100);
  }
}
*/
void blinkLed(uint8_t volte){
  for (uint8_t i = 0; i < volte; i++)
  {
    digitalWrite(LED_BUILTIN,LOW);
    delay(250);
    digitalWrite(LED_BUILTIN,HIGH);
    delay(250);
  }
}
void adessoDormo(){
  sendCommand("thup=1");
  sendCommand("sleep=1");
  client.disconnect();
  delay(10);
  WiFi.disconnect(true);
  wifi_set_sleep_type(LIGHT_SLEEP_T);
  WiFi.mode(WIFI_OFF); //energy saving mode if local WIFI isn't connected
  WiFi.forceSleepBegin();
  delay(180000);
  ESP.reset();
}
void setupWifi(){
  WiFi.persistent(false);   // Solve possible wifi init errors (re-add at 6.2.1.16 #4044, #4083)
  WiFi.disconnect(true);    // Delete SDK wifi config
  delay(200);
  WiFi.setOutputPower(17);        // 10dBm == 10mW, 14dBm = 25mW, 17dBm = 50mW, 20dBm = 100mW
  WiFi.mode(WIFI_OFF); //energy saving mode if local WIFI isn't connected
  delay(10);
  WiFi.hostname("chrono");      // DHCP Hostname (useful for finding device for static lease)
  WiFi.mode(WIFI_STA);
  WiFi.forceSleepWake();
  delay(10);
  WiFi.config(ipChrono, gateway, subnet,dns1); // Set static IP (2,7s) or 8.6s with DHCP  + 2s on battery
  delay(10);
  WiFi.begin(ssid, password);
}
void setup() {
  //handleCrash();
  nex_routines();
  pinMode(LED_BUILTIN,OUTPUT);
  digitalWrite(LED_BUILTIN,HIGH);
  irrecv.enableIRIn();  // Start the receiver
  
  setupWifi();
  delay(10);
  wifi_initiate = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if ((millis() - wifi_initiate) > 5000L) {
      adessoDormo();
      //dopo c'e' il restart
    }
    delay(500);
  }
  blinkLed(2);
  //sendCrash();
  String clientId = String(mqttId);
  clientId += String(random(0xffff), HEX);
  delay(10);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  delay(100);
  client.connect(clientId.c_str(),mqttUser,mqttPass);
  delay(10);
  wifi_initiate = millis();
  while (!client.connected()) {
    if ((millis() - wifi_initiate) > 5000L) {
      blinkLed(5);
      adessoDormo();
      //dopo c'e' il restart
    }
    delay(500);
  } 
  delay(50);
  reconnect();
  if(mqttOK){blinkLed(3);}
  delay(10);
  dht.setup(D4,dht.DHT22); // stesso del led
  wifi_initiate=millis();
  getLocalTemp();
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
void getLocalTemp(){
  delay(dht.getMinimumSamplingPeriod());
  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();
  //const char* errDHT = dht.getStatusString();
  smartDelay(100);
  //char humChar[8]; // Buffer big enough for 7-character float
  //char tempChar[8]; // Buffer big enough for 7-character float
  //dtostrf(humidity, 6, 1, humChar); // Leave room for too large numbers!
  //dtostrf(temperature, 6, 1, tempChar); // Leave room for too large numbers!
  StaticJsonBuffer<256> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  delay(10);
  root["topic"]="SalChr";
  root["hum"] = ((int)(humidity * 100 + .5) / 100.0);
  root["temp"] = ((int)(temperature * 100 + .5) / 100.0);
  char JSONmessageBuffer[256];
  root.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  smartDelay(100);
  mqttOK = client.publish(casaSensTopic,JSONmessageBuffer,sizeof(JSONmessageBuffer));
  smartDelay(100);
  //sendMySql(tempChar,humChar);
  //delay(10);
  
  //mqttOK = client.publish(logTopic,errDHT);
}
void reconnect() {
  smartDelay(50);
  client.publish(logTopic, "Crono connesso");
  delay(10);
  client.subscribe(systemTopic);
  delay(10);
  client.subscribe(casaSensTopic);
  delay(10);
  client.subscribe(extSensTopic);
  delay(10);
  client.subscribe(acquaTopic);
  delay(10);
  client.subscribe(riscaldaTopic);
  delay(10);
  client.subscribe(updateTopic);
  delay(10);
  mqttOK=client.subscribe(eneValTopic);
  smartDelay(50);
}
void loop() {
  smartDelay(2000);
  if((millis() - wifi_initiate) > wifi_check_time){ 
    wifi_initiate=millis();
    getLocalTemp();
    if(!mqttOK)
    {
      adessoDormo();
    }
  }
  if (irrecv.decode(&results)) {
    uint64_t infraredNewValue = results.value;
    switch (infraredNewValue) {
      case monnezza:
        mqttOK=client.publish(teleTopic, "monnezza", false);
        break;
      case spegni:
       mqttOK=client.publish(teleTopic,"spegni");
        break;
      case acquaON:
        mqttOK=client.publish(acquaTopic,"1");
        break;
      case eneOff:
        mqttOK=client.publish(eneTopic,"0");
        break;
      default:
        String s=int64String(infraredNewValue,HEX,false);
        mqttOK=client.publish(iRTopic,s.c_str());
        break;
    }
    irrecv.resume();  // Receive the next value
  }
  
}
void sendMySql(char* temp,char* hum){
  //WiFiClient mySqlclient;
  if (mywifi.connect(host, httpPort))
  {
    String s =String("GET /meteofeletto/chrono_logger.php?temp=" + String(temp) +
    +"&&pwd=" + webpass +
    +"&&hum=" + String(hum) +
    + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");
    smartDelay(100);
    mywifi.println(s);
    smartDelay(100);
    //mywifi.stop();
  }
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
  StaticJsonBuffer<100> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(payload);
  if(strcmp(topic, systemTopic) == 0 ) {
    String msg_Topic = root["topic"];
    if(msg_Topic == "UpTime") {
      //wifi_initiate=millis();
      delay(10);
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
  else if(strcmp(topic, eneValTopic) == 0 ) {
    String msg_Topic = root["topic"];
    if(msg_Topic == "EneMain") {
      const char* Nex_eneVal = root["e"];
      Nset_temp.setText(Nex_eneVal);
    }
  }
  smartDelay(200);
  #endif
  }

void smartDelay(unsigned long mytime){
  uint32_t adesso = millis();
  //if (!client.connected()) connectMQTT();
  while((millis()-adesso)<mytime){
    client.loop();
    nexLoop(nex_listen_list);
    delay(10);
  }
}
void stampaDebug(int8_t intmess){
  String myMess;
  switch (intmess) {
    case 0:
    myMess="W_OK";
      break;
    case 1:
      myMess="W_KO";

      break;
    case 2:
      myMess="M_OK";

      break;
    case 3:
      myMess="M_KO";

      break;
  }
  Ntcurr.setText(myMess.c_str());
  smartDelay(2000);
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
void update_buttons(){
    Nrisc_on.setPic(db_array_value[1]);
    Nwater_on.setPic(db_array_value[2]);
}
void Nb_upPushCallback(void *ptr){
  double  number;
  memset(buffer, 0, sizeof(buffer));
  Nset_temp.getText(buffer, sizeof(buffer));
  number = strtod (buffer,NULL);
  number += 0.5;
  dtostrf(number, 4, 1, buffer);
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
      client.publish(riscaldaTopic, "0");
    } else {
      client.publish(riscaldaTopic, "1");
    }
}
void Nwater_onPushCallback(void *ptr){
  uint32_t number = toggle_button(2);
    Nwater_on.setPic(number);
    if (number == 0) {
      mqttOK=client.publish(acquaTopic, "0");
    } else {
      mqttOK=client.publish(acquaTopic, "1");
    }
}
void nex_routines(){
  nexInit();
  delay(10);
  sendCommand("dim=20");
   Nset_temp.setText("");
  Ntcurr.setText("");
  Nout_temp.setText("");
  Nout_hum.setText("");
  Nin_hum.setText("");
  Ncurr_hour.setText("");
  Ncurr_water_temp.setText("");
  Nday.setText("");
  Nrisc_on.attachPush(Nrisc_onPushCallback);
  Nwater_on.attachPush(Nwater_onPushCallback);
  Nb_up.attachPush(Nb_upPushCallback);
  Nb_down.attachPush(Nb_downPushCallback);
  update_buttons();
  }
