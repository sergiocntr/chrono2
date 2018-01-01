/*************************************************************************************************************************
  Variables required proper functioning: Any modification may leads towards system failure
*************************************************************************************************************************/

//WiFi

int wifi_reconnect_tries = 0;
int mqtt_reconnect_tries = 0;
long wifi_reconnect_time = 0L;
long mqtt_reconnect_time = 0L;
long wifi_check_time = 15000L;

//Database

unsigned long db_array_value[20];

//Reboot Scheduler
boolean scheduled_reboot = false;

//Instantiate libraries
WiFiClient espClient;
PubSubClient client(espClient);
