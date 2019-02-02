/*************************************************************************************************************************
  Function definitions: Any modification may leads towards system failure
*************************************************************************************************************************
*
*
*************************************************************************************************************************
  Booting the system
*************************************************************************************************************************/
//#include <ESP8266WiFi.h>          //builtin library for ESP8266 Arduino Core

#include "topic.h"
//#include "debugNex.h"
void boot()
{
  //pinMode(16, OUTPUT);
  //digitalWrite(16, HIGH);
  //Serial.begin(9600);
  //SPIFFS.begin();
  //DEBUG_PRINT("System started");

  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);
  WiFi.mode(WIFI_STA);
}

/*************************************************************************************************************************
  Keep Aliving Loop
*************************************************************************************************************************/

void keeplive()
{
  client.loop();
  if((WiFi.status() != WL_CONNECTED) && ((millis() - wifi_reconnect_time) > wifi_check_time)) {
      wifi_check_time = 15000L;
      wifi_reconnect_time = millis();
      connectWiFi();
    }
  if ((WiFi.status() == WL_CONNECTED) && (!client.connected()) && ((millis() - mqtt_reconnect_time) > 5000L)) {
      mqtt_reconnect_time = millis();
      ++ mqtt_reconnect_tries;
      connectMQTT();
      if ((mqtt_reconnect_tries > 5) && (!client.connected())) {
        //updateDatabase(db_array[0], millis());
        scheduled_reboot = true;
      }
    }
  //scheduleReboot();

  delay(10);
}

/*************************************************************************************************************************
  Connect to WiFi network
*************************************************************************************************************************/

void connectWiFi()
{
  WiFi.forceSleepWake();
  delay(10);
  WiFi.mode(WIFI_STA);
  ++ wifi_reconnect_tries;
  if (mqtt_reconnect_tries != 0) {
    mqtt_reconnect_tries = 0;
  }
  boolean networkScan = false;
  int n = WiFi.scanNetworks();
  delay(300);
  for (int i = 0; i < n; ++i) {

    if (WiFi.SSID(i) == WIFI_SSID) {
      String this_print = String(WIFI_SSID) + " is available";
      //DEBUG_PRINT(this_print);
      networkScan = true;
      break;
    }
  }
  if(networkScan) {
    if (wifi_reconnect_tries > 1) {
      //DEBUG_PRINT("Retrying:: ");
    }
    String this_print = "Connecting to " + String(WIFI_SSID);
    //DEBUG_PRINT(this_print);
    long wifi_initiate = millis();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    WiFi.config(ip, gateway, subnet); // Set static IP (2,7s) or 8.6s with DHCP  + 2s on batteryWiFi.config(ip, gateway, subnet); // Set static IP (2,7s) or 8.6s with DHCP  + 2s on battery
    while (WiFi.status() != WL_CONNECTED) {
      //DEBUG_PRINT(".");
      if (WiFi.status() == WL_CONNECTED) {
        sendCommand("sleep=0");
        break;
      }
      if ((millis() - wifi_initiate) > 15000L) {
        break;
      }
      delay(500);
    }
    if (WiFi.status() == WL_CONNECTED) {
      //DEBUG_PRINT(" Connected!!");
      //DEBUG_PRINT("");
      //DEBUG_PRINT(WiFi.localIP());
      scheduled_reboot = false;
      //updateDatabase(db_array[0], 0);
      db_array_value[0]= 0;
      wifi_reconnect_tries = 0;
    } else if ((WiFi.status() != WL_CONNECTED) && (wifi_reconnect_tries > 3)) {
      String this_print = " Failed to connect to " + String(WIFI_SSID) + " Rebooting...";
      //DEBUG_PRINT(this_print);
      scheduled_reboot = true;
    }
  } else {
    //DEBUG_PRINT(WIFI_SSID);
    //DEBUG_PRINT(" is offline");
    //DEBUG_PRINT("");
    if (wifi_reconnect_tries > 3) {
      wifi_check_time = 300000L;
      wifi_reconnect_tries = 0;
      //DEBUG_PRINT("System will try again after 5 minutes");
      sendCommand("thup=1");
      sendCommand("sleep=1");
      WiFi.mode( WIFI_OFF );
      WiFi.forceSleepBegin();
      delay( 10 );
    }
  }
}

/*************************************************************************************************************************
  Connect to MQTT Broker
*************************************************************************************************************************/

void connectMQTT()
{
  if (mqtt_reconnect_tries > 1) {
    //DEBUG_PRINT("Retrying:: ");
  }
  //DEBUG_PRINT("Connecting to mqtt server: ");
  //DEBUG_PRINT(MQTT_SERVER);
  //if (client.connect(nodeID,mqttUser,mqttPass,"",0,1,""))
  client.connect(SKETCH_ID, MQTT_USER, MQTT_PASSWORD,"prova",0,1,"prova");
  delay(500);
  if (client.connected()) {
    send("units-bootup", String(SKETCH_ID) + "-bootup"); // Initial system status publish to server
    client.subscribe(MQTT_SUBSCRIBE_TOPIC); // Subscribe to your MQTT topic
    client.subscribe("subunit-ping"); // Subscribe to Ping
    client.subscribe(systemTopic); // Subscribe to Ping
    client.subscribe(casaSensTopic); // Subscribe to Ping
    client.subscribe(extSensTopic); // Subscribe to Ping
    client.subscribe(riscaldaTopic); // Subscribe to Ping
    client.subscribe(acquaTopic); // Subscribe to Ping
    ////DEBUG_PRINT(".. Connected!!");
    mqtt_reconnect_tries = 0;
    //scheduled_reboot = false;
    //updateDatabase(db_array[0], 0);
    db_array_value[0] = 0;
  } else {

    //DEBUG_PRINT("Failed to connect to mqtt server, rc=");
    //DEBUG_PRINT(client.state());
    //DEBUG_PRINT("");
  }
}

/*************************************************************************************************************************
  MQTT Incomming message handling
*************************************************************************************************************************/


/*************************************************************************************************************************
  Over The Air Update (OTA)
*************************************************************************************************************************/

/*void update()
{
  //DEBUG_PRINT("starting system update");
  client.publish(MQTT_PUBLISH_TOPIC, "starting system update", true);
  send("current sketch: " + String(SKETCH_ID) + "_" + String(SKETCH_VERSION) + ".cpp.nodemcu.bin");
  String link = "/";
  link += String(SKETCH_ID) + "_" + String(SKETCH_VERSION) + ".cpp.nodemcu.bin";
  ESPhttpUpdate.update(MQTT_SERVER, 82, link);
}*/
void setupOTA() {
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    ////DEBUG_PRINT("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    ////DEBUG_PRINT("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    ////DEBUG_PRINT("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    ////DEBUG_PRINT("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      dbSerialPrint("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      ////DEBUG_PRINT("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      ////DEBUG_PRINT("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      ////DEBUG_PRINT("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      ////DEBUG_PRINT("End Failed");
    }
    send(logTopic,String(error));
  });
}
void send(const char* topic, String message)
{
  client.publish(topic, message.c_str(), true);
}
