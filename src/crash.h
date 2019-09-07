#ifndef crash_h
#define crash_h
//#define DEBUGMIO
//#define DEBUGESP
#include "myFunctions.h"
#include "FS.h"
#include "debugutils.h"
#include "EspSaveCrash.h"
EspSaveCrash SaveCrash;
void handleCrash(){
  bool result = SPIFFS.begin();
  DEBUG_PRINT(" SPIFFS opened: " + String(result));
  //SaveCrash.print();
  File f = SPIFFS.open(crashFilename, "a+");
  f.println("crash chrono");
  SaveCrash.print(f);
  delay(100);
  SaveCrash.clear();
  f.close();
  delay(100); 
}
void sendCrash(){
  HTTPClient http;
  File f = SPIFFS.open(crashFilename, "r");
    if (!f) 
    {
      DEBUG_PRINT("File doesn't exist yet.");
    } else {
      uint16_t fileSize =f.size()
      DEBUG_PRINT("File size: " + String(fileSize));
      if(fileSize<30) return;
      String line;
      while(f.available()) {
        //Lets read line by line from the file
         line =line + f.readStringUntil('\n') + '\n';
          yield();
        
       }
      f.close();
      SPIFFS.remove(crashFilename);
      delay(100);
      DEBUG_PRINT("Stiamo per inviare: " + line);
      int httpResponseCode=0;
      c.connect(host, httpPort);
      delay(100);
      http.begin(c,post_serverCrash);
      http.addHeader("Content-Type","text/plain");
      httpResponseCode = http.PUT(line);
      String response = http.getString();                       //Get the response to the request
      DEBUG_PRINT("Responso dal server: " + response);           //Print request answer
      DEBUG_PRINT("Code dal server: " + String(httpResponseCode));
      delay(100);
      http.end();
      delay(100);
    }
  
}
#endif