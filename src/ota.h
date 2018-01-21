#pragma once

#include <ArduinoOta.h>
#ifdef ESP8266
#include <ESP8266mDNS.h>
#else
#include <ESP32mDNS.h>
#endif

class Ota 
{
    Stream * dbgstream;
    bool enable;    
    
    Task taskprocessOta;
public:
    void begin(Stream &dbgstream);

    void process();
};