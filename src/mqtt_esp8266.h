#ifndef mqtt_esp8266_H
#define mqtt_esp8266_H

#include "password.h"
#include "topic.h"
#include "myIP.h"
#include "debugNex.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
WiFiClient espClient;
PubSubClient client(espClient);
char setup_wifi() {
  if((WiFi.status() == WL_CONNECTED)) return 0;
  delay(10);
  // We start by connecting to a WiFi network
  DEBUG_PRINT(ssid);

  WiFi.begin(ssid, password);
  WiFi.config(ipChrono, gateway, subnet,dns1);
  unsigned long wifi_initiate = millis();
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    if ((millis() - wifi_initiate) > 5000L) return 1;
    delay(500);

  }

  DEBUG_PRINT("");
  DEBUG_PRINT("WiFi connected");
  DEBUG_PRINT("IP address: ");
  DEBUG_PRINT(WiFi.localIP());
  return 0;
}
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    DEBUG_PRINT("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("provaprova",mqttUser,mqttPass)) {
      DEBUG_PRINT("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe(systemTopic);
      client.loop();
      client.subscribe(casaSensTopic);
      client.loop();
      client.subscribe(extSensTopic);
      client.loop();
      client.subscribe(acquaTopic);
      client.loop();
      client.subscribe(riscaldaTopic);
      client.loop();
    } else {
      DEBUG_PRINT("failed, rc=");
      DEBUG_PRINT(client.state());
      DEBUG_PRINT(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
int send(const char* topic, String message){
  int check = client.publish(topic, message.c_str(), true);
  client.loop();
  return check;
}
int send(const char* topic, const char* message){
  int check = client.publish(topic, message, true);
  client.loop();
  return check;
}
#endif
