#ifndef wificonnection_h
#define wificonnection_h

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
//#include <ESP8266LLMNR.h>
#elif defined (ESP32)
#include <WiFi.h>
#include <ESPmDNS.h>
#endif

struct QueryResult;

//Wifi connection STA
class WiFiConnection{
public:

    WiFiConnection() {}

    void setup(Stream &dbgstream);

    void announceTheDevice(); 

    void process();

    QueryResult query();
    QueryResult query(String service, String proto);

private:
    Stream *  dbgstream;

    void wifiManagerOpenConnection();
    void DEBUG_printDiagWiFI();  

    void restartESP();
};

struct QueryResult {
public:
    uint16_t port;
    String host;
    IPAddress ip;
};

#endif
