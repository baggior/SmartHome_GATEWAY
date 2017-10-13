#pragma once

#include <ArduinoOta.h>
#include <ESP8266mDNS.h>

class Ota 
{
    Stream * dbgstream;
    bool enable;    
    
    Task taskprocessOta;
public:
    void begin(Stream &dbgstream);

    void process();
};