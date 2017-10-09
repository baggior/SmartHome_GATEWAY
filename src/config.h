#ifndef config_h
#define config_h

#include <Arduino.h>

#include <ArduinoJson.h>

#define     _TASK_STD_FUNCTION
#define     _TASK_SLEEP_ON_IDLE_RUN
//#define     _TASK_STATUS_REQUEST
#define     _TASK_WDT_IDS
#define     _TASK_LTS_POINTER

#include <TaskSchedulerDeclarations.h>

#include <pragmautils.h>
#include <dbgutils.h>
#include <baseutils.h>
#include <Blinker.h>

#include "ObjectModel.h"

class Config
{
    DynamicJsonBuffer jsonBuffer;
    JsonVariant jsonObject;
    Blinker blinker; 
    
public:
    
    Config();
    ~Config() ;
    void load();
    void persist();
    
    inline JsonObject& getJsonRoot() {return jsonObject;} 
    void printConfigFileTo(Stream& stream) ;
    
    inline Blinker& getBlinker() {return blinker;}

    static inline String getSoftwareVersion() { return SW_VERSION; }
    static String getDeviceInfoString(const char* crlf="\n");
};


extern Config config;

#endif