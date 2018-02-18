#ifndef wifirestserver_h
#define wifirestserver_h


#include <ESPAsyncWebServer.h>



class WifiRestServer {

public:

  typedef std::function<void(JsonObject* requestPostBody,  JsonObject* responseBody)> RestHandlerCallback;

  WifiRestServer();
  ~WifiRestServer();

  void setup(Stream &serial);
  
  bool process() ;

  void addRestApiMethod(const char* uri, RestHandlerCallback callback, bool isGetMethod=true );


private:
  
  AsyncWebServer * webServer;
  Stream *  dbgstream;
  bool enable;

  unsigned int _server_port;

  void _setupHandlers();
  
  void _onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);

};

#endif