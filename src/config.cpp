#include "config.h"

#include <ArduinoUtil.h>

#define CONFIG_FILE_PATH "/config.json"


Config::Config() 
{
    SPIFFS.begin();
}
Config::~Config() 
{
    SPIFFS.end();
}

void Config::load()
{
    ArduinoUtil au;    
    configJsonString = au.readTextFile(CONFIG_FILE_PATH);  
    DPRINTF("config file: %s",configJsonString.c_str());    
}


JsonObject& Config::getJsonRoot()
{
    return jsonBuffer.parseObject(configJsonString);
}

void Config::persist()
{
    //TODO
}

void Config::print(Stream& stream) 
{
    stream.println("Config:");
    getJsonRoot().prettyPrintTo(stream);
    stream.println();
}