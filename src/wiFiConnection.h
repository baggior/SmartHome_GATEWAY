#ifndef wificonnection_h
#define wificonnection_h

#include <ESP8266WiFi.h>

//Wifi connection STA
void wifiManagerOpenConnection(Stream& serial);

void DEBUG_printDiagWiFI(Stream& serial);

#endif