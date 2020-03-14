#ifndef IRNEXTION_H
#define IRNEXTION_H
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
const uint16_t kRecvPin = 14;
const uint64_t spegni = 0XFF2AD5;//sotto rosso R
const uint64_t acquaON = 0XFFAA55; // sotto verde G
const uint64_t eneOff = 0XFF22DD; //bianco con W
const uint64_t monnezza = 0XFF12ED; //sotto W
IRrecv irrecv(kRecvPin);
decode_results results;
#endif
