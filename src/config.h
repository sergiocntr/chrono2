/*************************************************************************************************************************
  Configure connection parameters
*************************************************************************************************************************/
#include <ESP8266WiFi.h>
const char* SKETCH_ID = "chrono";                  //Non-overlaping name of sketch
const char* SKETCH_VERSION = "0.1";                //Sketch version
const char* MQTT_PUBLISH_TOPIC = "nextion-out";     //Topic name to publish on mqtt
const char* MQTT_SUBSCRIBE_TOPIC = "nextion-in";    //Topic to subscribe at mqtt for message reception
const char* systemTopic = "/system/general"; // {"topic":"UpTime","hours": time_now,"Day":day};
//TOPIC COMANDO IN USCITA -> publish
//il cronotermostato invia questi dati:
//avvio acqua calda *** anche subscrive per ricevere il dato
const char* riscaldaTopic = "/casa/esterno/caldaia/relay"; // 0 o 1 per attivare il riscaldamento
//avvio riscaldamento *** anche subscrive per ricevere il dato
const char* acquaTopic ="/casa/esterno/caldaia/acqua";  //0 o 1 per attivare il preriscaldamento acqua
//sblocco caldaia
const char* resetTopic ="/casa/esterno/caldaia/reset"; //0 o 1 per premere pulsante reset
//logTopic
const char* logTopic ="/casa/esterno/caldaia/log"; // temp H20 caldaia
//TOPIC COMANDO IN INGRESSO -> subscrive
//il cronotermostato riceve questi dati:
//stato caldaia in blocco
const char* alarmTopic ="/casa/esterno/caldaia/alarm";   //ON OFF allarme blocco attivo
//temp H2O caldaia
const char* extSensTopic ="/casa/esterno/sensori"; // temp H20 caldaia
//temp CASA
const char* casaSensTopic = "/casa/interno/sensori";
//temp EXT
const char* extTempTopic = "/casa/esterno/terrazza_leo/sensori";
const char* WIFI_SSID = "TIM-23836387";
const char* WIFI_PASSWORD = "51vEBuMvmALxNQHVIHQKkn52";
const char* MQTT_SERVER = "192.168.1.100";          //IP Adress of Machine running MQTT Broker
const char* MQTT_USER = "sergio";                      //MQTT Broker User Name
const char* MQTT_PASSWORD = "donatella";           //MQTT Broker Password
const uint16_t MQTT_PORT = 8883;
IPAddress ip(192, 168, 1, 215); //Node static IP
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
/*************************************************************************************************************************
  Configure database parameters
*************************************************************************************************************************/
char buffer[30] = {0};
int db_array_default_value = 0;                    //default value to write in database e.g. default gpio state
