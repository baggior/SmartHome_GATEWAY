#ifndef config_h
#define config_h

#include <Arduino.h>

#include <ArduinoJson.h>

#include <pragmautils.h>
#include <dbgutils.h>

class Config
{
    DynamicJsonBuffer jsonBuffer;
    String configJsonString;

public:
    
    Config();
    ~Config() ;
    void load();
    void persist();
    inline String& getJsonString() {return configJsonString;}
    JsonObject& getJsonRoot() ;
    void print(Stream& stream) ;
};


extern Config config;

#endif