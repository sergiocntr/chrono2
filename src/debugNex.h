#ifndef DEBUGUTILS_H
#define DEBUGUTILS_H
//#include <Arduino.h>
#include "SoftwareSerial.h"
SoftwareSerial mydbSerial(4, 5); // RX, TX
//*****ENABLE FOR DEBUG
//#define DEBUGMIO
//#define DEBUGMQTT
  #ifdef DEBUGMIO
  #define __FILENAME__ (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)


  #define DEBUG_PRINT(str)    \
     mydbSerial.print(millis());     \
     mydbSerial.print(": ");    \
     mydbSerial.print(__PRETTY_FUNCTION__); \
     mydbSerial.print(' ');      \
     mydbSerial.print(__FILENAME__);     \
     mydbSerial.print(':');      \
     mydbSerial.print(__LINE__);     \
     mydbSerial.print(' ');      \
     mydbSerial.println(str); \

  #else
  #define DEBUG_PRINT(str)
  #endif

#endif
