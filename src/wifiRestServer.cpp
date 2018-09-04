// #include "config.h"

// #include <WifiClient.h>
// #ifdef ESP32
// #include <Update.h>
// #endif

// #include <SPIFFSEditor.h>
// #include <AsyncJson.h>

#include "wifiRestServer.h"

// #include "asynccallbackjsonwebhandler.h"

// #include "WiFiConnection.h"

// #define RESTSERVER_PORT_DEFAULT   80

// extern WiFiConnection connection;

// static AsyncEventSource _events("/events"); // event source (Server-Sent events to browser)

// static SPIFFSEditor _theSPIFFSEditor(SPIFFS);

// static void _printToResponseHandler(AsyncWebServerRequest *request);
// static void _showHelp(AsyncWebServerRequest *request);
// static void _onNotFoundHandler(Stream* dbgstream, AsyncWebServerRequest *request);
// static void _onBody(Stream* dbgstream, AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
// static void _onUpload(Stream* dbgstream, AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
// static void _onScanWiFi(AsyncWebServerRequest *request) ;
// static void _onFwUpdate1(Stream* dbgstream, AsyncWebServerRequest *request) ;
// static void _onFwUpdate2(Stream* dbgstream, AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) ;

  
WifiRestServer::WifiRestServer() 
: _RestApiModule("WifiRestApiModule", "rest api")//, enable(true)//, webServer(NULL)
{  
  // this->dbgstream=NULL;
  // this->_server_port = RESTSERVER_PORT_DEFAULT;
}
WifiRestServer::~WifiRestServer()
{
  // this->dbgstream=NULL;
  // if(this->webServer) delete this->webServer;
}

_Error WifiRestServer::restApiMethodSetup() 
{
  // base API handlers
  _Error ret =_RestApiModule::restApiMethodSetup();
  if(ret!=_NoError) return ret;

  // TODO additional module API handlers
  return _NoError;
}

/*
void WifiRestServer::setup(Stream &dbgstream)
{  
  this->dbgstream = &dbgstream;

  JsonObject & root = config.getJsonRoot();   

  this->enable = root["rest"]["enable"];   
  this->_server_port=root["rest"]["server_port"];

  //TODO request authentication not implemented
  const char* _server_auth_username = root["rest"]["server_auth"]["username"];
  const char* _server_auth_password = root["rest"]["server_auth"]["password"];

  Stream_printf(dbgstream, F(">RESTServer SETUP: enable: %u, server_port: %u, server_auth_username: %s, server_auth_password: %s\n"),
    enable, _server_port, REPLACE_NULL_STR(_server_auth_username), REPLACE_NULL_STR(_server_auth_password) );

  if(this->enable)
  {
    if(!_server_port) _server_port=RESTSERVER_PORT_DEFAULT;
          
    webServer = new AsyncWebServer(_server_port);    

    // Setup the server handlers
    _setupHandlers();

    // CORS headers
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Server","ESP Async Web Server");

    // Start the server
    webServer->begin();
    this->dbgstream->print(F(">RESTServer SETUP: started on port:")); this->dbgstream->println(this->_server_port);

    baseutils::StringArray attributes;
    attributes.add(String("pippo"));
    attributes.add(String("pluto"));
    connection.announceTheDevice(this->_server_port, attributes);
  }
  
}

bool WifiRestServer::process() 
{  
  return false; //unused ASYNC server
}
*/

/*
void WifiRestServer::addRestApiMethod(const char* uri, RestHandlerCallback callback, bool isGetMethod )
{
  if(isGetMethod)
  {
    // GET 

    webServer->on(uri, HTTP_GET, [callback](AsyncWebServerRequest *request)
    {
      AsyncJsonResponse * response = new AsyncJsonResponse();
      response->setContentType("application/json");
      JsonObject& responseObjectRoot = response->getRoot();

      callback( NULL, &responseObjectRoot );

      response->setLength();
      request->send(response);          
    });
  }
  else
  {
    // POST
    
    AsyncCallbackJsonWebHandler* jsonHandler = new AsyncCallbackJsonWebHandler(uri, [callback] (AsyncWebServerRequest *request, JsonVariant &json) 
    {
      AsyncJsonResponse * response = new AsyncJsonResponse();
      response->setContentType("application/json");
      JsonObject& responseObjectRoot = response->getRoot();

      JsonObject& requestObjectRoot = json.as<JsonObject>();
      callback( &requestObjectRoot, &responseObjectRoot );

      response->setLength();
      request->send(response);   
    });
    webServer->addHandler(jsonHandler);

//     webServer->on( uri, HTTP_POST, [callback](AsyncWebServerRequest *request)
//     {
//       AsyncJsonResponse * response = new AsyncJsonResponse();
//       response->setContentType("application/json");
//       JsonObject& responseObjectRoot = response->getRoot();
      

// //TODO parse request
//       callback( NULL, &responseObjectRoot );

//       response->setLength();
//       request->send(response);          
//     },    
//     std::bind(_onUpload, this->dbgstream, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6),
//     std::bind(_onBody, tis->dbgstream, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5) );

  }
}
*/


/*
void WifiRestServer::_onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len)
{
  //Handle WebSocket event
}




void WifiRestServer::_setupHandlers() 
{
  //Server-Sent EVENT HANDLERS 
  // _events.onConnect([](AsyncEventSourceClient *client){
  //   client->send("hello!",NULL,millis(),1000);
  // });
  // webServer->addHandler(&_events);

  // REQUEST HANDLERS 

  webServer->on("/api/plain/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  webServer->on("/api/restart", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String("Restarting ESP.."));
    ESP.restart();
  });

  this->addRestApiMethod("/api/heap", [](JsonObject* requestPostBody,  JsonObject* responseBody) {
    JsonObject& root = (*responseBody);
    root["heap"] = ESP.getFreeHeap();
    root["ssid"] = WiFi.SSID();
  });

//TEST POST
  this->addRestApiMethod("/api/heapPost", [this](JsonObject* requestPostBody,  JsonObject* responseBody) {
    requestPostBody->prettyPrintTo((*this->dbgstream));

    JsonObject& root = (*responseBody);
    root["heap"] = ESP.getFreeHeap();
    root["ssid"] = WiFi.SSID();
  }, false);


  webServer->on("/api/help", HTTP_GET, _showHelp);

  webServer->on("/api/echo", HTTP_ANY, _printToResponseHandler);

  webServer->on("/api/scanWifi", HTTP_GET, _onScanWiFi);

  webServer->on("/api/firmwareUpdate", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", "<form method='POST' action='/firmwareUpdate' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>");
  });
  // ArRequestHandlerFunction _onFwUpdate=
  // webServer->on("/update", HTTP_POST,_onFwUpdate1, _onFwUpdate2);

  // attach filesystem root at URL /
  webServer->serveStatic("/", SPIFFS, "/");

  // Web SPIFFS Editor
  webServer->addHandler(& _theSPIFFSEditor );

  // Catch-All Handlers
  // Any request that can not find a Handler that canHandle it
  // ends in the callbacks below.  
  webServer->onNotFound( std::bind(_onNotFoundHandler, this->dbgstream, std::placeholders::_1) );
  webServer->onFileUpload( std::bind(_onUpload, this->dbgstream, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6) );
  webServer->onRequestBody( std::bind(_onBody, this->dbgstream, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5) );

}


static void _printToResponseHandler(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response = request->beginResponseStream("text/html");
  response->addHeader("Server","ESP Async Web Server");
  response->printf("<!DOCTYPE html><html><head><title>Webpage at %s</title></head><body>", request->url().c_str());

  response->print("<h2>Hello ");
  response->print(request->client()->remoteIP());
  response->print("</h2>");

  response->print("<h3>General</h3>");
  response->print("<ul>");
  response->printf("<li>Version: HTTP/1.%u</li>", request->version());
  response->printf("<li>Method: %s</li>", request->methodToString());
  response->printf("<li>URL: %s</li>", request->url().c_str());
  response->printf("<li>Host: %s</li>", request->host().c_str());
  response->printf("<li>ContentType: %s</li>", request->contentType().c_str());
  response->printf("<li>ContentLength: %u</li>", request->contentLength());
  response->printf("<li>Multipart: %s</li>", request->multipart()?"true":"false");
  response->print("</ul>");

  response->print("<h3>Headers</h3>");
  response->print("<ul>");
  int headers = request->headers();
  for(int i=0;i<headers;i++){
    AsyncWebHeader* h = request->getHeader(i);
    response->printf("<li>%s: %s</li>", h->name().c_str(), h->value().c_str());
  }
  response->print("</ul>");

  response->print("<h3>Parameters</h3>");
  response->print("<ul>");
  int params = request->params();
  for(int i=0;i<params;i++){
    AsyncWebParameter* p = request->getParam(i);
    if(p->isFile()){
      response->printf("<li>FILE[%s]: %s, size: %u</li>", p->name().c_str(), p->value().c_str(), p->size());
    } else if(p->isPost()){
      response->printf("<li>POST[%s]: %s</li>", p->name().c_str(), p->value().c_str());
    } else {
      response->printf("<li>GET[%s]: %s</li>", p->name().c_str(), p->value().c_str());
    }
  }
  response->print("</ul>");

  response->print("</body></html>");
  //send the response last
  request->send(response);
}

static void _onNotFoundHandler(Stream* dbgstream, AsyncWebServerRequest *request)
{
  dbgstream->printf("NOT_FOUND: ");
  if(request->method() == HTTP_GET)
    dbgstream->printf("GET");
  else if(request->method() == HTTP_POST)
    dbgstream->printf("POST");
  else if(request->method() == HTTP_DELETE)
    dbgstream->printf("DELETE");
  else if(request->method() == HTTP_PUT)
    dbgstream->printf("PUT");
  else if(request->method() == HTTP_PATCH)
    dbgstream->printf("PATCH");
  else if(request->method() == HTTP_HEAD)
    dbgstream->printf("HEAD");
  else if(request->method() == HTTP_OPTIONS)
    dbgstream->printf("OPTIONS");
  else
    dbgstream->printf("UNKNOWN");
  dbgstream->printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

  if(request->contentLength()){
    dbgstream->printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
    dbgstream->printf("_CONTENT_LENGTH: %u\n", request->contentLength());
  }

  int headers = request->headers();
  int i;
  for(i=0;i<headers;i++){
    AsyncWebHeader* h = request->getHeader(i);
    dbgstream->printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
  }

  int params = request->params();
  for(i=0;i<params;i++){
    AsyncWebParameter* p = request->getParam(i);
    if(p->isFile()){
      dbgstream->printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
    } else if(p->isPost()){
      dbgstream->printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
    } else {
      dbgstream->printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
    }
  }

  // You will still need to respond to the OPTIONS method 
  // for CORS pre-flight in most cases. (unless you are only using GET)
  if (request->method() == HTTP_OPTIONS) {
		request->send(200);
	} else {
		request->send(404);
	}
};

static void _onBody(Stream* dbgstream, AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
{
  //Handle body
  if(!index)
    dbgstream->printf("BodyStart: %u\n", total);
  
  dbgstream->printf("%s", (const char*)data);

  if(index + len == total)
    dbgstream->printf("BodyEnd: %u\n", total);
}

static void _onUpload(Stream* dbgstream, AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  //Handle upload
  if(!index)
    dbgstream->printf("UploadStart: %s\n", filename.c_str());
    
  dbgstream->printf("%s", (const char*)data);

  if(final)
    dbgstream->printf("UploadEnd: %s (%u)\n", filename.c_str(), index+len);
}


static void _onScanWiFi(AsyncWebServerRequest *request) 
{
  //First request will return 0 results unless you start scan from somewhere else (loop/setup)
  //Do not request more often than 3-5 seconds

  String json = "[";
  int n = WiFi.scanComplete();
  if(n == -2){
    WiFi.scanNetworks(true, true);
  } else if(n){
    for (int i = 0; i < n; ++i){
      if(i) json += ",";
      json += "{";
      json += "\"rssi\":"+String(WiFi.RSSI(i));
      json += ",\"ssid\":\""+WiFi.SSID(i)+"\"";
      json += ",\"bssid\":\""+WiFi.BSSIDstr(i)+"\"";
      json += ",\"channel\":"+String(WiFi.channel(i));
      json += ",\"secure\":"+String(WiFi.encryptionType(i));
      // json += ",\"hidden\":"+String(WiFi.isHidden(i)?"true":"false");
      json += "}";
    }
    WiFi.scanDelete();
    if(WiFi.scanComplete() == -2){
      WiFi.scanNetworks(true);
    }
  }
  json += "]";
  request->send(200, "application/json", json);
  json = String();
}

static void _showHelp(AsyncWebServerRequest *request)
{
    String help ="";
    help.concat("******************************************************\r\n");  
    #ifdef ESP32
    help.concat("* ESP32 GATEWAY Web (REST) Server ");     
    #else
    help.concat("* ESP8266 GATEWAY Telnet Server ");     
    #endif
    String info = Config::getDeviceInfoString("\r\n");
    help.concat(info);
    help.concat("******************************************************\r\n");  
    
    request->send(200, "text/plain", help );

}

*/


  // Firmware Update 
  
  // static void _onFwUpdate1(Stream* dbgstream, AsyncWebServerRequest *request) 
  // {
  //   bool shouldReboot = !Update.hasError();
  //   AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", shouldReboot?"OK":"FAIL");
  //   response->addHeader("Connection", "close");
  //   request->send(response);

  // };
  // static void _onFwUpdate2(Stream* dbgstream, AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) 
  // {
  //   if(!index){
  //     dbgstream->printf("Update Start: %s\n", filename.c_str());
  //     Update.runAsync(true);
  //     if(!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)){
  //       Update.printError(*dbgstream);
  //     }
  //   }
  //   if(!Update.hasError()){
  //     if(Update.write(data, len) != len){
  //       Update.printError(*dbgstream);
  //     }
  //   }
  //   if(final){
  //     if(Update.end(true)){
  //       dbgstream->printf("Update Success: %uB\n", index+len);
  //     } else {
  //       Update.printError(*dbgstream);
  //     }
  //   }
  // }
