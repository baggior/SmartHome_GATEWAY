#ifndef config_h
#define config_h

#ifdef ESP32
#define LED_PIN 5   //TODO
#else
#define LED_PIN BUILTIN_LED
#endif

#include <Arduino.h>

#include <ArduinoJson.h>

#define     _TASK_STD_FUNCTION
//#define     _TASK_SLEEP_ON_IDLE_RUN
//#define     _TASK_STATUS_REQUEST
#define     _TASK_WDT_IDS
//#define     _TASK_LTS_POINTER
#include <TaskSchedulerDeclarations.h>


#include <pragmautils.h>
#include <dbgutils.h>
#include <baseutils.h>

#include <Blinker.h>

#include "ObjectModel.h"

class Config
{
    DynamicJsonBuffer jsonBuffer;    
    Blinker blinker; 
    String configJsonString;
    
public:
    
    Config();
    ~Config() ;
    void load();
    void persist();
    
    JsonObject& getJsonRoot(const char* node=NULL);
    void printConfigFileTo(Stream& stream) ;
    
    inline Blinker& getBlinker() {return blinker;}

    static inline String getSoftwareVersion() { return SW_VERSION; }
    static String getDeviceInfoString(const char* crlf="\n");
};


extern Config config;

#endif