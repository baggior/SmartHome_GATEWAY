#include "config.h"

#include <BaseUtils.h>
#include <TaskScheduler.h>
#include <StreamString.h>

#ifdef ESP8266
#include <ESP8266WiFi.h>
#else 
#include <WiFi.h>
#endif


#define CONFIG_FILE_PATH "/config.json"
#define JSON_BUFFER_SIZE 1024

Config::Config() 
: blinker(LED_PIN),
  jsonBuffer(JSON_BUFFER_SIZE)
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
    
    DPRINTF("Using config file: %s \n", CONFIG_FILE_PATH);   
    
    this->configJsonString = baseutils::readTextFile(CONFIG_FILE_PATH); 
    // JsonObject jsonObject = this->jsonBuffer.parseObject(configJsonString);
 
    #ifdef DEBUG_OUTPUT
    this->printConfigFileTo(DEBUG_OUTPUT);
    #endif
    
    SPIFFS.end();    
}


JsonObject& Config::getJsonRoot(const char* node)
{   
    this->jsonBuffer.clear();
    
    JsonObject& jsonObject = this->jsonBuffer.parseObject(this->configJsonString);    
    if(jsonObject.success())
    {
        if(node) return jsonObject[node];
        return jsonObject;
    }
    else
    {
        DPRINTF("Error parsing node %s of json: \n %s \n", (node?node:"<ROOT>"), configJsonString.c_str() );
        // throw error TODO
    } 
    return jsonObject;
}

void Config::persist()
{
    if(!SPIFFS.begin())
    {
        DPRINTF("Error opening SPIFFS filesystem\n");   
    }

    //ArduinoUtil au;    
    //TODO
    // String configJsonString= "";
    // bool ret = au.writeTextFile(CONFIG_FILE_PATH, configJsonString); 
    
    SPIFFS.remove(CONFIG_FILE_PATH);

    File configFile = SPIFFS.open(CONFIG_FILE_PATH, "w");
    if (!configFile) 
    {
        DPRINTF("ERROR Writing config file: %s \n",CONFIG_FILE_PATH);           
    }
    else
    {
        DPRINTF("Written config file: %s \n",CONFIG_FILE_PATH);   
    }
   
    getJsonRoot().prettyPrintTo(configFile);
    //config.printTo(configFile);
    configFile.close();

    SPIFFS.end();
}

void Config::printConfigFileTo(Stream& stream) 
{
    stream.println("Configuration: ");
    getJsonRoot().prettyPrintTo(stream);
    stream.println();
}


String Config::getDeviceInfoString(const char* crlf)
{
    String ret;
#ifdef ESP8266
    ret.concat("ESP8266 Chip ID: " + baseutils::getChipId() +crlf);
#elif defined(ESP32)    
    ret.concat("ESP32: Chip ID:" + baseutils::getChipId() + crlf);
#else
    //TODO other
#endif
    ret.concat("- software version: "); ret.concat(Config::getSoftwareVersion()); ret.concat(crlf);
    #ifdef MY_DEBUG
    ret.concat("* DEBUG is ON "); 
    #ifdef DEBUG_OUTPUT
    ret.concat(" Serial Port: "+ String(VALUE_TO_STRING(DEBUG_OUTPUT))); ret.concat(crlf);
    #endif
    #endif
    ret.concat(crlf);
    ret.concat("* Free Heap RAM: "); ret.concat(ESP.getFreeHeap()); ret.concat(crlf);
    ret.concat("* Mac address:"); ret.concat(WiFi.macAddress()); ret.concat(crlf);

    if(WiFi.isConnected())
    {
        ret.concat("* WiFI SSID:");ret.concat(WiFi.SSID()); ret.concat(" channel:");ret.concat(WiFi.channel()); ret.concat(" WiFiMode:");ret.concat(WiFi.getMode()); ret.concat(" PhyMode:");

#ifdef ESP8266
        ret.concat(WiFi.getPhyMode()); ret.concat(crlf);
        ret.concat("* Host:");ret.concat(WiFi.hostname()); 
#elif defined (ESP32)
        ret.concat(WiFi.getMode()); ret.concat(crlf);
        ret.concat("* Host:");ret.concat(WiFi.getHostname()); 
#endif

        ret.concat(" IP:");  ret.concat(WiFi.localIP().toString()); ret.concat(crlf);
        ret.concat("* subnet mask:");  ret.concat(WiFi.subnetMask().toString()); ret.concat(" Gateway IP:");  ret.concat(WiFi.gatewayIP().toString()); ret.concat(" DNS IP:");  ret.concat(WiFi.dnsIP().toString()); ret.concat(crlf);        
    }   

    StreamString ss;
    baseutils::printBoardInfo(ss); ss.replace("\n", crlf);
    ret.concat(crlf);
    ret.concat(ss);
    ret.concat(crlf);
    
    return ret;
}