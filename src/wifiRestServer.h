#ifndef wifirestserver_h
#define wifirestserver_h


#include <ESPAsyncWebServer.h>



class WifiRestServer {

public:

  typedef std::function<void(JsonObject* requestPostBody,  JsonObject* responseBody)> RestHandlerCallback;


  WifiRestServer(const uint8_t listenport=80);

  void setup(Stream &serial);
  
  bool process() ;

  void addRestApiMethod(const char* uri, RestHandlerCallback callback, bool isGetMethod=true );


private:
  
  AsyncWebServer webServer;
  Stream *  dbgstream;

  uint8_t _listenport;

  void _setupHandlers();
  
  void _onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);

};

#endif