#pragma once

#include "coreapi.h"

#include <ArduinoOta.h>
#ifdef ESP8266
#include <ESP8266mDNS.h>
#elif defined(ESP32)
#include <ESPmDNS.h>
#endif

// TODO MIGRATE

class Ota 
{
    Stream * dbgstream;
    bool enable;    
    
    Task taskprocessOta;
public:
    void begin(Stream &dbgstream);

    void process();
};