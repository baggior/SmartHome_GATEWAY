#include "coreapi.h"

#include <StreamString.h>

#ifdef ESP8266
#include <ESP8266WiFi.h>
#else 
#include <WiFi.h>
#endif

#define CONFIG_FILE_PATH "/config.json"
#define JSON_BUFFER_SIZE 1024


static DynamicJsonBuffer jsonBuffer(JSON_BUFFER_SIZE);

_ApplicationConfig::_ApplicationConfig() 
: jsonObject(NULL)
{    
}
_ApplicationConfig::~_ApplicationConfig()
{  
}

_Error _ApplicationConfig::load(_ApplicationLogger& logger, bool formatSPIFFSOnFails)
{
    bool b = false;
#ifdef ESP32
    b = SPIFFS.begin(formatSPIFFSOnFails);
#elif defined ESP8266
    b = SPIFFS.begin();
#endif

    if(!b)
    {
        logger.printf(F("Error opening SPIFFS filesystem\r\n"));   

        _Error err(-1, "Error opening SPIFFS filesystem");
        return err;
    }
    else 
    {
#ifdef ESP32
        logger.printf(F("SPIFFS filesystem open(%d used Bytes)\n\n"), SPIFFS.usedBytes() );  
#elif defined ESP8266   
        logger.printf(F("SPIFFS filesystem open\r\n"));
#endif  
        logger.printf(F("Using config file: %s \r\n"), CONFIG_FILE_PATH);   
        
        this->configJsonString = baseutils::readTextFile(CONFIG_FILE_PATH); 
        // JsonObject jsonObject = this->jsonBuffer.parseObject(configJsonString);
    
        SPIFFS.end();   

        jsonBuffer.clear();
        JsonObject& jsonObject_parsed = jsonBuffer.parseObject(this->configJsonString);    
        if(jsonObject_parsed.success())
        {
            this->jsonObject = &jsonObject_parsed;
            this->printConfigFileTo(logger.getStream());

            return _NoError; 
        }
        else
        {
            //  DPRINTF(F("Error parsing node %s of json:\r\n %s \r\n"), (node?node:"<ROOT>"), configJsonString.c_str() );
            return _Error(-1, "Error parsing json config file");
        }
    }    
    
}

void _ApplicationConfig::printConfigFileTo(Stream* stream) const
{
    if(stream)
    {
        stream->println(F("Configuration: "));
        if(this->jsonObject!=NULL)
        {
            this->jsonObject->prettyPrintTo(*stream);
            stream->println();
        }
        else 
        {
            stream->println(F("<NULL>"));
        }
    }    
}





JsonObject* _ApplicationConfig::getJsonObject(const char* node)
{   
    if(this->jsonObject)
    {
        if(node) 
        {
            JsonObject& jsonNode = ((*this->jsonObject)[node]);
            return &jsonNode;
        }
        return this->jsonObject;
    }
    else
    {
        //DPRINTF(F("Error parsing node %s of json:\r\n %s \r\n"), (node?node:"<ROOT>"), configJsonString.c_str() );
        // throw error TODO        
    } 
    return NULL;
}







///////////////////////////////////////////////////////

String _ApplicationConfig::getDeviceInfoString(const char* crlf)
{
    String ret;
    ret.concat("- software version: "); ret.concat(_ApplicationConfig::getSoftwareVersion()); ret.concat(crlf);
// #ifdef ESP8266
//     ret.concat("ESP8266 Chip ID: " + baseutils::getChipId() +crlf);
// #elif defined(ESP32)    
//     ret.concat("ESP32: Chip ID:" + baseutils::getChipId() + crlf);
// #else
//     //TODO other
// #endif
    String secRunning( millis() / 1000 );
    ret.concat("* Started up ");ret.concat(secRunning);ret.concat(" seconds ago.\n"); 

    #ifdef MY_DEBUG
    ret.concat("* DEBUG is ON "); 
    #ifdef DEBUG_OUTPUT
    ret.concat("Serial Port: "+ String(VALUE_TO_STRING(DEBUG_OUTPUT))); ret.concat(crlf);
    #endif
    #endif
    ret.concat(crlf);
    ret.concat("* Free Heap RAM: "); ret.concat(ESP.getFreeHeap()); ret.concat(crlf);
    ret.concat("* Mac address: "); ret.concat(WiFi.macAddress()); ret.concat(crlf);

    if(WiFi.isConnected())
    {
        ret.concat("* WiFI SSID: ");ret.concat(WiFi.SSID()); ret.concat(" channel: ");ret.concat(WiFi.channel()); ret.concat(" WiFiMode: ");ret.concat(WiFi.getMode()); ret.concat(" PhyMode: ");

#ifdef ESP8266
        ret.concat(WiFi.getPhyMode()); ret.concat(crlf);
        ret.concat("* Host:");ret.concat(WiFi.hostname()); 
#elif defined (ESP32)
        ret.concat(WiFi.getMode()); ret.concat(crlf);
        ret.concat("* Host: ");ret.concat(WiFi.getHostname()); 
#endif

        ret.concat(" IP: ");  ret.concat(WiFi.localIP().toString()); ret.concat(crlf);
        ret.concat("* subnet mask: ");  ret.concat(WiFi.subnetMask().toString()); ret.concat(" Gateway IP: ");  ret.concat(WiFi.gatewayIP().toString()); ret.concat(" DNS IP: ");  ret.concat(WiFi.dnsIP().toString()); ret.concat(crlf);        
    }   

    StreamString ss;
    baseutils::printBoardInfo(ss); ss.replace("\n", crlf);
    ret.concat(crlf);
    ret.concat(ss);
    ret.concat(crlf);
    
    return ret;
}