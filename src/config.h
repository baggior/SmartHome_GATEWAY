#ifndef config_h
#define config_h

#include <Arduino.h>

// #include <EspSaveCrash.h>

#include <ArduinoJson.h>

#include <pragmautils.h>
#include <dbgutils.h>

class Config
{
    DynamicJsonBuffer jsonBuffer;
    JsonVariant jsonObject;

public:
    
    Config();
    ~Config() ;
    void load();
    void persist();
    
    inline JsonObject& getJsonRoot() {return jsonObject;} 
    void printConfigFileTo(Stream& stream) ;
    
    static inline String getSoftwareVersion() { return SW_VERSION; }
    static String getDeviceInfoString(const char* crlf="\n");
};


extern Config config;

#endif