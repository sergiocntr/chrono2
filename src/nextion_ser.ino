/*************************************************************************************************************************
*    HomeAutomation 2016
*    Version 3.0
*    Created By: Waqas Ahmed
*         Email: ahmed@hobbytronics.com.pk
*    All Rights Reserved © 2016 HobbyTronics Pakistan
*
*    Configure connection parameters in config.h
*    Any modification in Functions, init.h tabs and whereever mentioned may leads towards failure of software
*    Download necessary libraries from the links mentioned before the library including statement
*
*
*************************************************************************************************************************
  Required Libraries
*************************************************************************************************************************/

#include <ESP8266WiFi.h>          //builtin library for ESP8266 Arduino Core
#include <PubSubClient.h>         //https://github.com/knolleary/pubsubclient
//#include <ESP8266httpUpdate.h>    //builtin library for ESP8266 Arduino Core
//#include <ESP8266HTTPClient.h>    //builtin library for ESP8266 Arduino Core
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include "FS.h"                   //builtin library for ESP8266 Arduino Core
#include "config.h"               //package builtin configuration file
#include "init.h"                 //package builtin configuration file
#include "Nextion.h"
//#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

/*************************************************************************************************************************
  Database variables
*************************************************************************************************************************/

String db_array[] = {"Reboot", "q0", "q1", "q2", "q3"};
int db_array_len = 5;
/*
 * Declare a crop object [page id:0,component id:1, component name: "q0"].
 */

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

/*
   Register object textNumber, buttonPlus, buttonMinus, to the touch event list.
*/
NexTouch *nex_listen_list[] =
{
  &Nrisc_on,
  &Nwater_on,
  &Nb_up,
  &Nb_down,
  NULL
};

void update_buttons()
{
    Nrisc_on.setPic(db_array_value[1]);
    Nwater_on.setPic(db_array_value[2]);
    //Nt_up.setPic(db_array_value[3]);
    //Nt_down.setPic(db_array_value[4]);
}
void Nb_upPushCallback(void *ptr)
{
  uint16_t len;
  double  number;
  NexButton *btn = (NexButton *)ptr;
  dbSerialPrintln("Nb_upPopCallback");
  dbSerialPrint("ptr=");
  dbSerialPrintln((uint32_t)ptr);

  memset(buffer, 0, sizeof(buffer));

  /* Get the text value of button component [the value is string type]. */
  Nset_temp.getText(buffer, sizeof(buffer));
  number = strtod (buffer,NULL);
  number += 0.5;
  dtostrf(number, 4, 1, buffer);

  /* Set the text value of button component [the value is string type]. */
  Nset_temp.setText(buffer);
}
void Nb_downPushCallback(void *ptr)
{
    uint16_t len;
    double  number;
    NexButton *btn = (NexButton *)ptr;
    dbSerialPrintln("Nb_downPopCallback");
    dbSerialPrint("ptr=");
    dbSerialPrintln((uint32_t)ptr);
    memset(buffer, 0, sizeof(buffer));

    /* Get the text value of button component [the value is string type]. */
    Nset_temp.getText(buffer, sizeof(buffer));

    number = strtod (buffer,NULL);
    number -= 0.5;
    dtostrf(number, 4, 1, buffer);
    //memset(buffer, 0, sizeof(buffer));
    //snprintf(buffer, sizeof(buffer), "%g", number);

    /* Set the text value of button component [the value is string type]. */
    Nset_temp.setText(buffer);
}

void Nrisc_onPushCallback(void *ptr)
{
    uint32_t number = toggle_button(1);
    //Nrisc_on.setPic(number);
    if (number == 0) {
      send(riscaldaTopic, "0");
    } else {
      send(riscaldaTopic, "1");
    }
}

void Nwater_onPushCallback(void *ptr)
{
  Serial.println("Nwater_onPushCallback sparato");
    uint32_t number = toggle_button(2);
    Nwater_on.setPic(number);
    if (number == 0) {
      send(acquaTopic, "0");
    } else {
      send(acquaTopic, "1");
    }
}

int toggle_button(int value)
{
  //char buffer [10];
  //itoa (db_array_value[value],buffer,10);
  //Serial.print("Prima toggle ");
  //Serial.println( buffer);
  if (db_array_value[value] == 1) {
    db_array_value[value] = 0;
    return 0;
  } else {
    db_array_value[value] = 1;
    return 1;
  }

}

/*************************************************************************************************************************
  Setup Function
*************************************************************************************************************************/

void setup() {
  nexInit();
  boot();     //necessary to call at first during setup function for proper functioning
  sendCommand("dim=20");
  //sendCommand("thsp=30");
  //sendCommand("thup=1");
  //nexSerial.write(0xFF);
  //nexSerial.write(0xFF);
  //nexSerial.write(0xFF);
  //nexSerial.flush();
  Nrisc_on.attachPush(Nrisc_onPushCallback);
  Nwater_on.attachPush(Nwater_onPushCallback);
  Nb_up.attachPush(Nb_upPushCallback);
  Nb_down.attachPush(Nb_downPushCallback);
  update_buttons();

  connectWiFi();  //necessary to call at the last during setup function for proper functioning
  setupOTA();
}

/*************************************************************************************************************************
  MQTT Incoming Message Handler
*************************************************************************************************************************/

void callback(char* topic, byte* payload, unsigned int length)
{
  if(strcmp(topic, acquaTopic) == 0 ) {
    /*char buffer [10];
    itoa (db_array_value[2],buffer,10);
    Serial.print("Prima callbak ");
    Serial.println( buffer);
    itoa (payload[0],buffer,10);
    Serial.print("Payload 0= ");
    Serial.println( buffer);
    //Serial.println("PRima: " + (char)db_array_value[2]);
*/
    if (char(payload[0]) == '0') {
      //Serial.println(db_array_value[2]);
      db_array_value[2] = 0;
      Nwater_on.setPic(0);

    }else{

      //Serial.println("payload[1]");
      db_array_value[2] = 1;
      Nwater_on.setPic(1);
    }
    /*itoa (db_array_value[2],buffer,10);
    Serial.print("Dopo callbak ");
    Serial.println( buffer);
    */
  }
  if(strcmp(topic, riscaldaTopic) == 0 ) {
    if (char(payload[0]) == '0') {
      db_array_value[1] = 0;
      Nrisc_on.setPic(0);

    }else{
      db_array_value[1] = 1;
      Nrisc_on.setPic(1);
    }
  }
//*************************************************************************************************************************
//Any modification to the portion below may leads towards system failure                                                  *
//                                                                                                                        *
  String message = String();
  for (int i = 0; i < length; i++) {  //A loop to convert incomming message to a String
    char input_char = (char)topic[i];
    message += input_char;
  }
  Serial.print("mqtt topic received (");
  Serial.print(message);
  Serial.println(")");
  message="";
  for (int i = 0; i < length; i++) {  //A loop to convert incomming message to a String
    char input_char = (char)payload[i];
    message += input_char;
  }
  Serial.print("mqtt payload received (");
  Serial.print(message);
  Serial.println(")");
  send("Message recevied on ESP8266 Unit: [" + message + "]");
  incomming(message);                 //Performing system fucntions such as OTA
//                                                                                                                        *
//Any modification to the portion above may leads towards system failure                                                  *                                             *
//*************************************************************************************************************************
//add your custom code for incomming message below this line

  char jsonChar[100];
  message.toCharArray(jsonChar, message.length()+1);
  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(jsonChar);
  if(strcmp(topic, systemTopic) == 0 ) {
    //Serial.println("topic: " + ((String)topic));
    String msg_Topic = root["topic"];
    //Serial.println("msg_Topic: " + msg_Topic);
    if(msg_Topic == "UpTime") {
      const char* Nex_Time = root["hours"];
      const char* Nex_Day = root["Day"];
      Ncurr_hour.setText(Nex_Time);
      Nday.setText(Nex_Day);
      //Serial.println("regolata ora " + ((String)Nex_Time));
    }
  }
  if(strcmp(topic, casaSensTopic) == 0 ) {
    String msg_Topic = root["topic"];
    if(msg_Topic == "DHTCamera") {
      const char* Nex_inHm = root["Hum"];
      const char* Nex_inTemp = root["Temp"];
      Ntcurr.setText(Nex_inTemp);
      Nin_hum.setText(Nex_inHm);
    }
  }
  if(strcmp(topic, extSensTopic) == 0 ) {
    String msg_Topic = root["topic"];
    if(msg_Topic == "Caldaia") {
      const char* Nex_wt = root["acqua"];
      //dtostrf(root["acqua"], 4, 1, buffer);
      Ncurr_water_temp.setText(Nex_wt);
      //const char* Nex_inTemp = root["Temp"];
      //Ncurr_water_temp.setText(water_temp);
    }
    else if (msg_Topic == "Terrazza"){
      const char* Nex_outHm = root["Hum"];
      const char* Nex_outTemp = root["Temp"];
      Nout_temp.setText(Nex_outTemp);
      Nout_hum.setText(Nex_outHm);
    }
  }



}

/*************************************************************************************************************************
  Loop Function
*************************************************************************************************************************/

void loop()
{
  keeplive();   //necessary to call keep alive for proper functioning
  nexLoop(nex_listen_list);
}



/*************************************************************************************************************************
  End of the file
*************************************************************************************************************************/
