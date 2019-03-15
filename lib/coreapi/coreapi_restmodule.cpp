#include "coreapi.h"

#ifdef ESP32
#include <Update.h>
#endif

#include <SPIFFSEditor.h>

#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>

// #include "WiFiConnection.h"

#define RESTSERVER_PORT_DEFAULT   80


_Error _RestApiModule::setup() 
{
  bool on = false;
  const char* _server_auth_username= NULL;
  const char* _server_auth_password= NULL;

  // configuration
  const JsonObject& root = this->theApp->getConfig().getJsonObject("rest");  
  if(!root.isNull()) 
  {
    on = root["enable"];   
    this->_server_port=root["server_port"];
    if(this->_server_port==0) this->_server_port=RESTSERVER_PORT_DEFAULT;
    
    //TODO request authentication not implemented
    _server_auth_username = root["server_auth"]["username"];
    _server_auth_password = root["server_auth"]["password"];
  }

  this->theApp->getLogger().printf( F("\t%s config: enable: %u, server_port: %u, server_auth_username: %s, server_auth_password: %s\n"),
    this->getTitle().c_str(), on, 
    this->_server_port, REPLACE_NULL_STR(_server_auth_username), REPLACE_NULL_STR(_server_auth_password) );

  if(on)
  {
    this->webServer = new AsyncWebServer(this->_server_port);   
    
    // CORS headers
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Server","ESP Async Web Server");

    // Setup the server handlers
    _Error err = this->restApiMethodSetup();
    if(err!=_NoError) {
      return err;
    }


    // Start the server
    this->webServer->begin();   
    this->theApp->getLogger().printf(F("\t%s: WebServer started on port:%d\n"),
      this->getTitle().c_str(), this->_server_port);

    return _NoError;
  }
  else
  {
    return _Disable;
  }  
}

void _RestApiModule::shutdown() 
{
  if(this->webServer) 
  {   
      delete this->webServer;
      this->webServer = NULL;
  }

  this->setEnabled(false);
}



void _RestApiModule::addRestApiMethod(const char* uri, RestHandlerCallback callback, bool isGetMethod )
{
  if(isGetMethod)
  {
    // GET 

    webServer->on(uri, HTTP_GET, [callback](AsyncWebServerRequest *request)
    {
      AsyncJsonResponse * response = new AsyncJsonResponse();
      response->setContentType("application/json");
      const JsonObject& responseObjectRoot = response->getRoot();

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
      JsonObject responseObjectRoot = response->getRoot();

      JsonObject requestObjectRoot = json.as<JsonObject>();
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









////////////////////////////////////////////////////////////////////////
// handlers
static SPIFFSEditor _theSPIFFSEditor(SPIFFS);

static void _showHelp(AsyncWebServerRequest *request)
{
    String help ="";
    help.concat("******************************************************\r\n");  
    #ifdef ESP32
    help.concat("* ESP32 GATEWAY Web (REST) Server ");     
    #else
    help.concat("* ESP8266 GATEWAY Telnet Server ");     
    #endif
    String info = _ApplicationConfig::getDeviceInfoString("\r\n");
    help.concat(info);
    help.concat("******************************************************\r\n");  
    
    request->send(200, "text/plain", help );

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
  if (dbgstream) 
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
  if(dbgstream) {
    //Handle body
    if(!index)
      dbgstream->printf("BodyStart: %u\n", total);
    
    dbgstream->printf("%s", (const char*)data);

    if(index + len == total)
      dbgstream->printf("BodyEnd: %u\n", total);
  }
}

static void _onUpload(Stream* dbgstream, AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  if(dbgstream) {

    //Handle upload
    if(!index)
      dbgstream->printf("UploadStart: %s\n", filename.c_str());
      
    dbgstream->printf("%s", (const char*)data);

    if(final)
      dbgstream->printf("UploadEnd: %s (%u)\n", filename.c_str(), index+len);

  }
}

_Error _RestApiModule::additionalRestApiMethodSetup() 
{
  return _NoError;
}

_Error _RestApiModule::restApiMethodSetup() 
{
  // Basic handlers  
  this->webServer->on("/api/restart", HTTP_GET, [this](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String("Restarting ESP.."));
    //ESP.restart();
    this->theApp->restart();
  });

  this->webServer->on("/api/plain/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });
  this->addRestApiMethod("/api/heap", [](const JsonObject* requestPostBody, const JsonObject* responseBody) {
    const JsonObject& root = (*responseBody);
    root["heap"] = ESP.getFreeHeap();
    root["ssid"] = WiFi.SSID();
  });
  
  this->webServer->on("/api/help", HTTP_GET, _showHelp);
  this->webServer->on("/api/scanWifi", HTTP_GET, _onScanWiFi);  
  this->webServer->on("/api/echo", HTTP_ANY, _printToResponseHandler);
  
  // attach filesystem root at URL /
  this->webServer->serveStatic("/", SPIFFS, "/");

  // Web SPIFFS Editor
  this->webServer->addHandler(& _theSPIFFSEditor );
  
  //FIRMWARE UPDATE
  this->webServer->on("/api/firmwareUpdate", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", "<form method='POST' action='/firmwareUpdate' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>");
  });
  // ArRequestHandlerFunction _onFwUpdate=
  // webServer->on("/update", HTTP_POST,_onFwUpdate1, _onFwUpdate2);

  // Catch-All Handlers
  // Any request that can not find a Handler that canHandle it
  // ends in the callbacks below.  
  this->webServer->onNotFound( std::bind(_onNotFoundHandler, this->theApp->getLogger().getStream(), std::placeholders::_1) );
  this->webServer->onFileUpload( std::bind(_onUpload, this->theApp->getLogger().getStream(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6) );
  this->webServer->onRequestBody( std::bind(_onBody, this->theApp->getLogger().getStream(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5) );

  return this->additionalRestApiMethodSetup();
  //return _NoError;
}


 void _RestApiModule::beforeModuleAdded()
 {
    //remove core rest api if exists
    this->theApp->removeModule(  this->theApp->getBaseModule( ENUM_TO_STR(_CoreRestApiModule) ) );
 }