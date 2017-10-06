#ifndef wifirestserver_h
#define wifirestserver_h

/*
  This a simple example of the aREST Library for the ESP8266 WiFi chip.
  See the README file for more details.

  Written in 2015 by Marco Schwartz under a GPL license.
*/

// Import required libraries
#include <WifiServer.h>
#include <WifiClient.h>
#include <aREST.h>



class WifiRestServer {


  // Create aREST instance
  aREST rest;
  
  // The port to listen for incoming TCP connections
  uint8_t listenport = 80;

  // Create an instance of the server
  WiFiServer server;

  // Variables to be exposed to the API
  int temperature;
  int humidity;

public:
  WifiRestServer(const uint8_t listenport=80);

  void setup(Stream &serial);
  
  bool process() ;

  void send(const String& s_msg);

};

#endif