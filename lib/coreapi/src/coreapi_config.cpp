#include "coreapi.h"

#include <StreamString.h>

#ifdef ESP8266
#include <ESP8266WiFi.h>
#else 
#include <WiFi.h>
#endif

#define CONFIG_FILE_PATH "/config.json"
// #define CONFIG_FILE_PATH "/config.sist.json"

#define JSON_BUFFER_SIZE 1024


static DynamicJsonDocument jsonBuffer(JSON_BUFFER_SIZE);

_ApplicationConfig::_ApplicationConfig(_Application& _theApp) : 
    theApp(_theApp)
{    
}
_ApplicationConfig::~_ApplicationConfig()
{  
}

_Error _ApplicationConfig::load(_ApplicationLogger& logger)
{
    logger.printf(F("Using config file: %s \r\n"), CONFIG_FILE_PATH);   
    const String configJsonString = baseutils::readTextFile(CONFIG_FILE_PATH); 

    jsonBuffer.clear();
    this->jsonObject.clear();

    DeserializationError error = deserializeJson(jsonBuffer, configJsonString);
    // JsonObject& jsonObject_parsed = jsonBuffer.parseObject(configJsonString);    // the input is read-only, the parser copies the input

    if(!error)
    {
        this->jsonObject = jsonBuffer.as<JsonObject>();
        this->printConfigTo(logger.asPrint());

        return _NoError; 
    }
    else
    {
        //  DPRINTF(F("Error parsing node %s of json:\r\n %s \r\n"), (node?node:"<ROOT>"), configJsonString.c_str() );
        return _Error(-1, String(F("Error parsing json config file: ")) + String(error.c_str()) );
    }    
}

void _ApplicationConfig::printConfigTo(Print* print) const
{
    if(print)
    {
        print->println(F("Configuration: "));
        if(! this->jsonObject.isNull())
        {
            size_t size = serializeJsonPretty(this->jsonObject, *print);
            // this->jsonObject->prettyPrintTo(*stream);
            print->println();
        }
        else 
        {
            print->println(F("<NULL>"));
        }
    }    
}





const JsonObject _ApplicationConfig::getJsonObject(const char* node)const
{   
    if(node) 
    {
        const JsonObject& jsonNode = ((this->jsonObject)[node]);
        if(! jsonNode.isNull() )
            return jsonNode;
            // return ((this->jsonObject)[node]);
    }
    return this->jsonObject;
}



_Error _ApplicationConfig::persist()
{
   
// // #ifdef ESP32
// //     bool b = SPIFFS.begin(true);
// // #elif defined ESP8266
// //     bool b = SPIFFS.begin();
// // #endif

//     if(!b)
//     {
//         DPRINTF(F("Error opening SPIFFS filesystem\r\n"));   
//         return _ConfigPersistError;
//     }

    //ArduinoUtil au;    
    //TODO
    // String configJsonString= "";
    // bool ret = au.writeTextFile(CONFIG_FILE_PATH, configJsonString); 
    
    SPIFFS.remove(CONFIG_FILE_PATH);

    File configFile = SPIFFS.open(CONFIG_FILE_PATH, "w");
    if (!configFile) 
    {
        DPRINTF(F("ERROR Preparing to write config file: %s \r\n"),CONFIG_FILE_PATH);      
        // SPIFFS.end();
        return _ConfigPersistError;     
    }
    else
    {
        DPRINTF(F("Prepared to write config file: %s \r\n"),CONFIG_FILE_PATH);   
        
        if(! this->jsonObject.isNull())
        {
            size_t size = serializeJsonPretty(this->jsonObject, configFile);
            // this->jsonObject->prettyPrintTo(configFile);        
        }
        else 
        {
            configFile.write((const uint8_t*)("{}"), 2);
        }
   
        configFile.close();
        // SPIFFS.end();   
    }   

    this->theApp.getLogger().printf(F("Configurazione salvata -> '%s'"),CONFIG_FILE_PATH);

    return _NoError;
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
#elif defined (ESP32)
        ret.concat(WiFi.getMode()); ret.concat(crlf);
#endif

        ret.concat("* Host: ");ret.concat(_NetServices::getHostname()); 

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