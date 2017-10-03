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
    DPRINTF("config file: %s \n",CONFIG_FILE_PATH);   
    #ifdef DEBUG_OUTPUT
    print(DEBUG_OUTPUT);
    #endif
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
    stream.println("Configuration: ");
    getJsonRoot().prettyPrintTo(stream);
    stream.println();
}