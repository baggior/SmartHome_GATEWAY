#ifndef wifirestserver_h
#define wifirestserver_h


// Import required libraries
#include <WifiServer.h>
#include <WifiClient.h>
#include <ESPAsyncWebServer.h>



class WifiRestServer {

private:

  // Create RestServer instance
  AsyncWebServer webServer;
  Stream *  dbgstream;

  uint8_t _listenport;

  void _setupHandlers();
  void _onBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
  void _onUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
  void _onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);

public:
  WifiRestServer(const uint8_t listenport=80);

  void setup(Stream &serial);
  
  bool process() ;

  void send(const String& s_msg);

};

#endif