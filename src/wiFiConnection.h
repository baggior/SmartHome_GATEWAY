#ifndef wificonnection_h
#define wificonnection_h

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
//#include <ESP8266LLMNR.h>

//Wifi connection STA
class WiFiConnection{
public:

    WiFiConnection() {}

    void setup(Stream &dbgstream);

    void announceTheDevice(); 

    void process();

    void query(String service);

private:
    Stream *  dbgstream;

    void wifiManagerOpenConnection();
    void DEBUG_printDiagWiFI();  

    void restartESP();
};


#endif