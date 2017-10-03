#ifndef config_h
#define config_h

#include <Arduino.h>

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
    void print(Stream& stream) ;
};


extern Config config;

#endif