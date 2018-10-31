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

#include "LinkedList.h"

struct QueryResult;

//Wifi connection STA
class WiFiConnection {

public:
    static String getHostname();

    WiFiConnection() {}

    void setup(Stream &dbgstream);

    void process();

    // QueryResult query();
    // QueryResult query(String service, String proto);

    // void announceTheDevice(unsigned int server_port=80, baseutils::StringArray attributes=baseutils::StringArray());

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
