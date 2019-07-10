#ifndef wifirestserver_h
#define wifirestserver_h


#include "coreapi.h"

class WifiRestServerModule : public _RestApiModule 
{
public:

  // typedef std::function<void(JsonObject* requestPostBody,  JsonObject* responseBody)> RestHandlerCallback;

  WifiRestServerModule();  

  
  // void setup(Stream &serial);
  
  // bool process() ;

  // void addRestApiMethod(const char* uri, RestHandlerCallback callback, bool isGetMethod=true );


private:  
  // AsyncWebServer * webServer;
  // Stream *  dbgstream;
  // bool enable;

  // unsigned int _server_port;

  // void _setupHandlers();
  
  // void _onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);


  virtual _Error additionalRestApiMethodSetup() final override;
};

#endif