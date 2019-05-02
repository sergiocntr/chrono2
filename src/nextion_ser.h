
#include "topic.h"
#include "myFunctions.h"
#include "SPI.h"               //package builtin configuration file
#include "SD.h"               //package builtin configuration file
#include "Nextion.h"
#include <ArduinoJson.h>
#include "password.h"
uint8_t db_array_value[3] = {0};
char buffer[15]={0};
NexText Nset_temp         = NexText(0, 2, "Nset_temp");
NexText Ntcurr            = NexText(0, 3, "Ntcurr");
NexText Nout_temp         = NexText(0, 4, "Nout_temp");
NexCrop Nwater_on         = NexCrop(0, 5, "Nwater_on");
NexText Nout_hum          = NexText(0, 6, "Nout_hum");
NexText Nin_hum           = NexText(0, 7, "Nin_hum");
NexText Ncurr_hour        = NexText(0, 8, "Ncurr_hour");
NexButton Nb_up           = NexButton(0, 9, "Nb_up");
NexButton Nb_down         = NexButton(0, 10, "Nb_down");
NexText Ncurr_water_temp  = NexText(0, 11, "Nwater_temp");
NexText Nday              = NexText(0, 12, "Nday");
NexCrop Nrisc_on          = NexCrop(0, 13, "Nrisc_on");
NexCrop Nalarm            = NexCrop(0, 14, "Nalarm");
NexTouch *nex_listen_list[] ={
  &Nrisc_on,
  &Nwater_on,
  &Nb_up,
  &Nb_down,
  NULL
};
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
  delay(2000);
}

void smartDelay(unsigned long mytime){
  unsigned long adesso = millis();
  while((millis()-adesso)<mytime){
    client.loop();
    nexLoop(nex_listen_list);
    delay(10);
  }
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
      send(riscaldaTopic, "0");
    } else {
      send(riscaldaTopic, "1");
    }
}
void Nwater_onPushCallback(void *ptr){
  uint32_t number = toggle_button(2);
    Nwater_on.setPic(number);
    if (number == 0) {
      send(acquaTopic, "0");
    } else {
      send(acquaTopic, "1");
    }
}
void nex_routines(){
  nexInit();
  delay(10);
  sendCommand("dim=20");
  Nrisc_on.attachPush(Nrisc_onPushCallback);
  Nwater_on.attachPush(Nwater_onPushCallback);
  Nb_up.attachPush(Nb_upPushCallback);
  Nb_down.attachPush(Nb_downPushCallback);
  update_buttons();
  }
