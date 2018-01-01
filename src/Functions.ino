/*************************************************************************************************************************
  Function definitions: Any modification may leads towards system failure
*************************************************************************************************************************
*
*
*************************************************************************************************************************
  Booting the system
*************************************************************************************************************************/
//#include <ESP8266WiFi.h>          //builtin library for ESP8266 Arduino Core
void boot()
{
  pinMode(16, OUTPUT);
  digitalWrite(16, HIGH);
  Serial.begin(9600);
  //Serial.swap();
  SPIFFS.begin();
  Serial.println("System started");
  if(!fetchDatabase()) {
    if(!creatDatabase()) {
      Serial.println("Problem generating database file");
      Serial.println("Formating file system");
      if(SPIFFS.format()) {
        Serial.println("Succeeded in formating file system");
        creatDatabase();
      }
    }
  }
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
        updateDatabase(db_array[0], millis());
        scheduled_reboot = true;
      }
    }
  scheduleReboot();
  delay(1);
}

/*************************************************************************************************************************
  Connect to WiFi network
*************************************************************************************************************************/

void connectWiFi()
{
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
      Serial.println(this_print);
      networkScan = true;
      break;
    }
  }
  if(networkScan) {
    if (wifi_reconnect_tries > 1) {
      Serial.print("Retrying:: ");
    }
    String this_print = "Connecting to " + String(WIFI_SSID);
    Serial.println(this_print);
    long wifi_initiate = millis();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      if (WiFi.status() == WL_CONNECTED) {
        break;
      }
      if ((millis() - wifi_initiate) > 15000L) {
        break;
      }
      delay(500);
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print(" Connected!!");
      Serial.println("");
      Serial.println(WiFi.localIP());
      scheduled_reboot = false;
      updateDatabase(db_array[0], 0);
      db_array_value[0]= 0;
      wifi_reconnect_tries = 0;
    } else if ((WiFi.status() != WL_CONNECTED) && (wifi_reconnect_tries > 3)) {
      String this_print = " Failed to connect to " + String(WIFI_SSID) + " Rebooting...";
      Serial.println(this_print);
      scheduled_reboot = true;
    }
  } else {
    Serial.print(WIFI_SSID);
    Serial.print(" is offline");
    Serial.println("");
    if (wifi_reconnect_tries > 3) {
      wifi_check_time = 300000L;
      wifi_reconnect_tries = 0;
      Serial.println("System will try again after 5 minutes");
    }
  }
}

/*************************************************************************************************************************
  Connect to MQTT Broker
*************************************************************************************************************************/

void connectMQTT()
{
  if (mqtt_reconnect_tries > 1) {
    Serial.print("Retrying:: ");
  }
  Serial.print("Connecting to mqtt server: ");
  Serial.println(MQTT_SERVER);
  client.connect(SKETCH_ID, MQTT_USER, MQTT_PASSWORD);
  delay(500);
  if (client.connected()) {
    send("units-bootup", String(SKETCH_ID) + "-bootup"); // Initial system status publish to server
    client.subscribe(MQTT_SUBSCRIBE_TOPIC); // Subscribe to your MQTT topic
    client.subscribe("subunit-ping"); // Subscribe to Ping
    client.subscribe(systemTopic); // Subscribe to Ping
    client.subscribe(casaSensTopic); // Subscribe to Ping
    client.subscribe(extSensTopic); // Subscribe to Ping
    Serial.println(".. Connected!!");
    mqtt_reconnect_tries = 0;
    scheduled_reboot = false;
    updateDatabase(db_array[0], 0);
    db_array_value[0] = 0;
  } else {

    Serial.print("Failed to connect to mqtt server, rc=");
    Serial.print(client.state());
    Serial.println("");
  }
}

/*************************************************************************************************************************
  MQTT Incomming message handling
*************************************************************************************************************************/

void incomming(String message)
{
  int message_length = message.length();

  if (message == "update") { // When we receive ota notification from MQTT, start the update
    update();
  } else if (message == "reboot") {
    reboot();
  } else if (message == "format") {
    format();
  } else if (message == "Ping") {
    send("units-bootup", String(SKETCH_ID) + "-ping");
  } else if (message == "info") {
    String ip_address = ipToString(WiFi.localIP());
    String firmware = String(SKETCH_ID) + "_" + String(SKETCH_VERSION) + ".cpp.nodemcu.bin";
    send(firmware + "@ " + ip_address);
  } else if ((strstr(message.c_str(), "gpio")) && (message_length == 8)) {
    int oneIndex = message.indexOf('-');
    int secondIndex = message.indexOf('-', oneIndex+1);
    String gpio_pin = message.substring(oneIndex+1, secondIndex);
    String gpio_state = message.substring(secondIndex+1);
    int gpio_pin_int = gpio_pin.toInt();
    int gpio_state_int = gpio_state.toInt();
    gpioDrive(gpio_pin_int, gpio_state_int);
  } else if (strstr(message.c_str(), "set")) {
    int oneIndex = message.indexOf('-');
    int secondIndex = message.indexOf('-', oneIndex+1);
    String setter = message.substring(oneIndex+1, secondIndex);
    String value = message.substring(secondIndex+1);
    int setter_int = setter.toInt();
    int value_int = value.toInt();
    set(setter_int, value_int);
  } else if (strstr(message.c_str(), "get")) {
    int oneIndex = message.indexOf('-');
    int secondIndex = message.indexOf('-', oneIndex+1);
    String job = message.substring(oneIndex+1, secondIndex);
    String entity = message.substring(secondIndex+1);
    int entity_int = entity.toInt();
    get(job, entity_int);
  } else if (message == "time") {
    updateDatabase(db_array[0], millis());
    send("Sytem time [" + String(millis()) + "] saved to database value " + String(db_array[0]));
  }
  else {
    send("MQTT command undefined");
  }
}

/*************************************************************************************************************************
  Over The Air Update (OTA)
*************************************************************************************************************************/

void update()
{
  Serial.println("starting system update");
  client.publish(MQTT_PUBLISH_TOPIC, "starting system update", true);
  send("current sketch: " + String(SKETCH_ID) + "_" + String(SKETCH_VERSION) + ".cpp.nodemcu.bin");
  String link = "/";
  link += String(SKETCH_ID) + "_" + String(SKETCH_VERSION) + ".cpp.nodemcu.bin";
  ESPhttpUpdate.update(MQTT_SERVER, 82, link);
}

/*************************************************************************************************************************
  Fetch GPIO / Variable Status
*************************************************************************************************************************/

int get(String job, int entity)
{
  if (job == "gpio") {
    int pin = db_array[entity].toInt();
    int read_state = digitalRead(pin);
    send("gpio [" + db_array[entity] + "] is retrieved as " + String(read_state));
  } else if (job == "var") {
    send("Database entry [" + db_array[entity] + "] is retrieved as " + String(db_array_value[entity]));
  }
}

/*************************************************************************************************************************
  Set custom value to a variable and update it over database
*************************************************************************************************************************/

int set(int var, int val)
{
  db_array_value[var] = val;
  updateDatabase(db_array[var], val);
  send("Database entry [" + String(db_array[var]) + "] is set to " + String(val));
}

/*************************************************************************************************************************
  IP Address to String conversion
*************************************************************************************************************************/

String ipToString(IPAddress ip){
  String s="";
  for (int i=0; i<4; i++)
    s += i  ? "." + String(ip[i]) : String(ip[i]);
  return s;
}

/*************************************************************************************************************************
  Sending message to builtin topic
*************************************************************************************************************************/

boolean send(String message)
{
  client.publish(MQTT_PUBLISH_TOPIC, message.c_str(), true);
}

/*************************************************************************************************************************
  Sending message to specific topic
*************************************************************************************************************************/

boolean send(const char* topic, String message)
{
  client.publish(topic, message.c_str(), true);
}

/*************************************************************************************************************************
  Loading Database
*************************************************************************************************************************/

boolean fetchDatabase()
{
  File databaseFile = SPIFFS.open("/config.json", "r");
  if (!databaseFile) {
    Serial.println("Failed to open database file");
    return false;
  }
  size_t size = databaseFile.size();
  if (size > 1024) {
    Serial.println("database file size is too large");
    return false;
  }
  std::unique_ptr<char[]> buf(new char[size]);
  databaseFile.readBytes(buf.get(), size);

  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& database = jsonBuffer.parseObject(buf.get());
  if (!database.success()) {
    Serial.println("Failed to parse database file");
    return false;
  }
  boolean update_database = false;
  for (int i = 0; i < db_array_len ; i++) {
    String this_array = "entry" + db_array[i];
    if (database.containsKey(this_array)) {
      db_array_value[i] = database[this_array];
      String this_print = "Database array[" + String(i) + "] i.e. " + String(db_array[i]) + ", it's value has been retrieved as [" + String(db_array_value[i]) + "]";
      Serial.println(this_print);
    } else {
      database[this_array] = db_array_default_value;
      db_array_value[i] = db_array_default_value;
      update_database = true;
      String this_print = "Database array[" + String(i) + "] i.e. " + String(db_array[i]) + ", it's value is not found in database and default value is adopted which is [" + String(db_array_value[i]) + "]";
      Serial.println(this_print);
    }
  }

  if (update_database) {
    File databaseFile = SPIFFS.open("/config.json", "w");
    database.printTo(databaseFile);
    Serial.println("Failure was detected in parsing and default value wrote to failed entities");
    return true;
  }
  Serial.println("database parsed successfully");
  return true;
}

/*************************************************************************************************************************
  Creating databse if not found
*************************************************************************************************************************/

boolean creatDatabase()
{
  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& database = jsonBuffer.createObject();
  for (int i = 0; i < db_array_len ; i++) {
    String this_array = "entry" + db_array[i];
    database[this_array] = db_array_default_value;
    db_array_value[i] = db_array_default_value;
    String this_print = "Database array[" + String(i) + "] i.e. " + String(db_array[i]) + ", it's value is not found in database and default value is adopted which is [" + String(db_array_value[i]) + "]";
    Serial.println(this_print);
  }
  File databaseFile = SPIFFS.open("/config.json", "w");
  if (!databaseFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }
  database.printTo(databaseFile);
  Serial.println("gpio default values wrote to database file");
  return true;
}

/*************************************************************************************************************************
  Update database with new values
*************************************************************************************************************************/

void updateDatabase(String entity, unsigned long state)
{
  File databaseFile = SPIFFS.open("/config.json", "r");
  size_t size = databaseFile.size();
  std::unique_ptr<char[]> buf(new char[size]);
  databaseFile.readBytes(buf.get(), size);
  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& database = jsonBuffer.parseObject(buf.get());
  String this_entity = "entry" + entity;
  if (database[this_entity] != state) {
    database[this_entity] = state;
    File databaseFile = SPIFFS.open("/config.json", "w");
    database.printTo(databaseFile);
    String this_print = "New value " + String(state) + " has been updated in database against " + String(entity);
    Serial.println(this_print);
  } else {
    String this_print = String(entity) + " is already preserved in database as " + String(state);
    Serial.println(this_print);
  }
}

/*************************************************************************************************************************
  Formating File System
*************************************************************************************************************************/

void format()
{
  Serial.println("initiating SPIFF file system format");
  send("initiating SPIFF file system format");
  if(SPIFFS.format()) {
    Serial.println("Succeeded to format file system");
    send("SPIFF file system format completed");
  }
}

/*************************************************************************************************************************
  Driving output pins
*************************************************************************************************************************/

void gpioDrive(int array, int state)
{
  int pin = db_array[array].toInt();

  if((state == 0) || (state == 1)) {
    if (digitalRead(pin) != state) {
      digitalWrite(pin, state);
      send("gpio-" + String(pin) + "-" + String(state));
      db_array_value[array] = state;
      updateDatabase(db_array[array], state);
    } else {
      String this_print = "gpio " + String(pin) + "is already set to " + String(state);
      Serial.println(this_print);
      send("gpio" + String(pin) + " is already set to " + String(state));
    }
  } else {
    String this_print = String(state) + " is not a valid state for gpio " + String(pin);
    Serial.println(this_print);
    send(String(state) + " is not a valid state for gpio: " + String(pin));
  }
}

/*************************************************************************************************************************
  Instantaneous Reboot
*************************************************************************************************************************/

void reboot()
{
  Serial.println("rebooting system");
  send("rebooting system");
  digitalWrite(16, LOW);
}

/*************************************************************************************************************************
  Reboot Schedule
*************************************************************************************************************************/

void scheduleReboot()
{
  if (((scheduled_reboot) && (millis() > 3600000) && (db_array_value[0] == 1)) || ((scheduled_reboot) && (db_array_value[0] == 0))) { // Reboot only if last reboot time is greater than 1 hour i.e. 3600000 MilliSeconds
    updateDatabase(db_array[0], 1);
    scheduled_reboot = false;
    Serial.println("Rebooting as per scheduled");
    delay(1000);
    digitalWrite(16, LOW);
  }
}
