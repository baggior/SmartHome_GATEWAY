#include "config.h"

#include <ArduinoUtil.h>

#define CONFIG_FILE_PATH "/config.json"


Config::Config() 
{

}
Config::~Config() 
{
    
}

void Config::load()
{
    if(!SPIFFS.begin())
    {
        DPRINTF("Error opening SPIFFS filesystem\n");   
    }
    
    DPRINTF("Using config file: %s \n",CONFIG_FILE_PATH);   
    ArduinoUtil au;    
    String configJsonString = au.readTextFile(CONFIG_FILE_PATH); 

    #ifdef DEBUG_OUTPUT
    print(DEBUG_OUTPUT);
    #endif

    this->jsonObject = jsonBuffer.parseObject(configJsonString);

    SPIFFS.end();
}


// JsonObject& Config::getJsonRoot()
// {
//     return this->jsonObject;
// }

void Config::persist()
{
    if(!SPIFFS.begin())
    {
        DPRINTF("Error opening SPIFFS filesystem\n");   
    }

    ArduinoUtil au;    
    //TODO
    // String configJsonString= "";
    // bool ret = au.writeTextFile(CONFIG_FILE_PATH, configJsonString); 


    SPIFFS.end();
}

void Config::print(Stream& stream) 
{
    stream.println("Configuration: ");
    getJsonRoot().prettyPrintTo(stream);
    stream.println();
}