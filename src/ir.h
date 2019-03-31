#ifndef irr_h
#define irr_h
#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRutils.h>
const uint16_t kRecvPin = 14;
const uint16_t kCaptureBufferSize = 256;
IRrecv irrecv(kRecvPin, kCaptureBufferSize, kTimeout, true);
decode_results results;  // Somewhere to store the results;
void startIr(){
  irrecv.enableIRIn();  // Start the receiver
}
#endif
