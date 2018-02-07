#ifndef config_h
#define config_h

#ifdef ESP32
#define LED_PIN 0 // esp32dev no builtin LED
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

#include "hw_config.h"
#include "objectModel.h"

class Config
{
    DynamicJsonBuffer jsonBuffer;    
    String configJsonString;
    Blinker blinker; 
    
public:
    
    Config();
    ~Config() ;
    int load(boolean formatSPIFFSOnFails=false);
    int persist();
    
    JsonObject& getJsonRoot(const char* node=NULL);
    void printConfigFileTo(Stream& stream) ;
    
    inline Blinker& getBlinker() {return blinker;}

    static inline String getSoftwareVersion() { return SW_VERSION; }
    static String getDeviceInfoString(const char* crlf="\n");
};


extern Config config;

#endif