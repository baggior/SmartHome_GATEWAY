#include "coreapi.h"

#ifdef ESP32
#include <Update.h>
#endif

#include <SPIFFSEditor.h>

#include <ESPAsyncWebServer.h>
 
// #define ARDUINOJSON_VERSION_MAJOR 5
#include <AsyncJson.h>

// #include "WiFiConnection.h"

#define RESTSERVER_PORT_DEFAULT   80

////////////////////
static SPIFFSEditor* _theSPIFFSEditor = NULL;
////////////////////

_Error _RestApiModule::setup() 
{
  bool on = false;
  const char* _server_auth_username= NULL;
  const char* _server_auth_password= NULL;

  bool _spiffs_editor_on = false;
  const char* _spiffs_editor_auth_username = NULL;
  const char* _spiffs_editor_auth_password = NULL;

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

    _spiffs_editor_on = root["spiffs_editor"]["enable"] | false; 
    _spiffs_editor_auth_username = root["spiffs_editor"]["username"]; 
    _spiffs_editor_auth_password = root["spiffs_editor"]["username"]; 
  }

  this->theApp->getLogger().info (("\t%s config: enable: %u, server_port: %u, server_auth_username: %s, server_auth_password: %s,"
    "SPIFFS_editor_enable: %u, SPIFFS_editor_auth_username: %s, SPIFFS_editor_auth_password: %s.\n"),
    this->getTitle().c_str(), on, 
    this->_server_port, REPLACE_NULL_STR(_server_auth_username), REPLACE_NULL_STR(_server_auth_password),
    _spiffs_editor_on, REPLACE_NULL_STR(_spiffs_editor_auth_username), REPLACE_NULL_STR(_spiffs_editor_auth_password) ) ;

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

    if( _spiffs_editor_on) {
      // Web SPIFFS Editor http://espressif.local/edit
      if(_spiffs_editor_auth_username && _spiffs_editor_auth_password)
        _theSPIFFSEditor = new SPIFFSEditor(SPIFFS, _spiffs_editor_auth_username, _spiffs_editor_auth_password);
      else 
        _theSPIFFSEditor = new SPIFFSEditor(SPIFFS);
      this->webServer->addHandler(_theSPIFFSEditor );

      this->theApp->getLogger().info(("\tSPIFFSEditor started on url:%s\n"), "/edit");
    }

    // Start the server
    this->webServer->begin();   
    this->theApp->getLogger().info(("\t%s: WebServer started on port:%d\n"),
      this->getTitle().c_str(), this->_server_port);

    //mdns announce rest service
    this->theApp->getNetServices().mdnsAnnounceService(this->_server_port, this->getTitle());

    return _NoError;
  }
  else
  {
    return _Disable;
  }  
}

void _RestApiModule::shutdown() 
{
  this->theApp->getLogger().info(("%s: RestApiModule shutdown..\n"), this->getTitle().c_str());

  if (_theSPIFFSEditor)
  {
    delete _theSPIFFSEditor;
    _theSPIFFSEditor = NULL;
  } 
  if(this->webServer) 
  {       
    // delete this->webServer; // TODO: CRASH bug in ESPsyncWebServer 
    free (this->webServer); // TODO: replace
    this->webServer = NULL;
  }

  this->setEnabled(false);
}



void _RestApiModule::addRestApiMethod(const char* uri, RestHandlerCallback callback, bool isGetMethod, size_t maxJsonBufferSize )
{
  if(isGetMethod)
  {
    // GET 

    webServer->on(uri, HTTP_GET, 
      [this, callback, uri, maxJsonBufferSize](AsyncWebServerRequest *request)
      {
        this->theApp->getLogger().debug("called rest api GET url: '%s'\n", uri);

        AsyncJsonResponse * response = new AsyncJsonResponse(maxJsonBufferSize);        
        response->setContentType(JSON_MIMETYPE);        
        const JsonObject& responseObjectRoot = response->getRoot();
        
        callback( this->theApp, NULL, &responseObjectRoot );

        response->setLength();       
        request->send(response);          
        
        // String ret;
        // serializeJsonPretty(responseObjectRoot, ret);
        // request->send(200, JSON_MIMETYPE, ret);         
      });
  }
  else
  {
    // POST
    
    AsyncCallbackJsonWebHandler* jsonHandler = new AsyncCallbackJsonWebHandler(uri,
      [this, callback, uri, maxJsonBufferSize] (AsyncWebServerRequest *request, JsonVariant &jsonInput) 
      {        
        this->theApp->getLogger().debug("called rest api POST url: '%s'\n", uri);

        AsyncJsonResponse * response = new AsyncJsonResponse(maxJsonBufferSize);
        response->setContentType(JSON_MIMETYPE);
        
        JsonObject requestObjectRoot = jsonInput.as<JsonObject>();
        JsonObject responseObjectRoot = response->getRoot();

        callback( this->theApp, &requestObjectRoot, &responseObjectRoot );

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
//(SPIFFS, "esp", "esp" );

static void _showInfoPlain(AsyncWebServerRequest *request)
{
    String help ="";
    help.concat("******************************************************\r\n");  
    #ifdef ESP32
    help.concat("* ESP32 GATEWAY Web (REST) Server \r\n");     
    #else
    help.concat("* ESP8266 GATEWAY Telnet Server \r\n");     
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
  request->send(200, JSON_MIMETYPE, json);
  // json = String();
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


static void _onNotFoundHandler(_ApplicationLogger& logger, AsyncWebServerRequest *request)
{
  logger.warn("NOT_FOUND: ");
  if(request->method() == HTTP_GET)
    logger.debug("GET");
  else if(request->method() == HTTP_POST)
    logger.debug("POST");
  else if(request->method() == HTTP_DELETE)
    logger.debug("DELETE");
  else if(request->method() == HTTP_PUT)
    logger.debug("PUT");
  else if(request->method() == HTTP_PATCH)
    logger.debug("PATCH");
  else if(request->method() == HTTP_HEAD)
    logger.debug("HEAD");
  else if(request->method() == HTTP_OPTIONS)
    logger.debug("OPTIONS");
  else
    logger.debug("UNKNOWN");
  logger.debug(" http://%s%s\n", request->host().c_str(), request->url().c_str());

  if(request->contentLength()){
    logger.debug("_CONTENT_TYPE: %s\n", request->contentType().c_str());
    logger.debug("_CONTENT_LENGTH: %u\n", request->contentLength());
  }

  int headers = request->headers();
  int i;
  for(i=0;i<headers;i++){
    AsyncWebHeader* h = request->getHeader(i);
    logger.debug("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
  }

  int params = request->params();
  for(i=0;i<params;i++){
    AsyncWebParameter* p = request->getParam(i);
    if(p->isFile()){
      logger.debug("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
    } else if(p->isPost()){
      logger.debug("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
    } else {
      logger.debug("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
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


static void _onBody(_ApplicationLogger& logger, AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
{
  //Handle body
  if(!index)
    logger.debug("BodyStart: %u\n", total);
  
  logger.debug("%s", (const char*)data);

  if(index + len == total)
    logger.debug("BodyEnd: %u\n", total);
  
}

static void _onUpload(_ApplicationLogger& logger, AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  //Handle upload
  if(!index)
    logger.debug("UploadStart: %s\n", filename.c_str());
    
  logger.debug("%s", (const char*)data);

  if(final)
    logger.debug("UploadEnd: %s (%u)\n", filename.c_str(), index+len);  
}

_Error _RestApiModule::additionalRestApiMethodSetup() 
{
  return _NoError;
}

_Error _RestApiModule::restApiMethodSetup() 
{
  // Basic handlers  

  // RESTART ESP Plain
  this->webServer->on("/api/plain/restart", HTTP_GET, [this](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String("Restarting ESP.."));
    this->theApp->restart();
  });

  // HEAP Plain
  this->webServer->on("/api/plain/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  // HEAP INFO Plain  
  this->webServer->on("/api/plain/info", HTTP_GET, _showInfoPlain);

  // HEAP echo
  this->webServer->on("/api/echo", HTTP_ANY, _printToResponseHandler);

  // HEAP REST
  this->addRestApiMethod("/api/heap", [](_Application* theApp, const JsonObject* requestPostBody, const JsonObject* responseBody) {
    
    const JsonObject& root = (*responseBody);
    root["heap"] = ESP.getFreeHeap();
    root["ssid"] = WiFi.SSID();    
    root["hostname"] = WiFi.getHostname();
    root["ip"] = WiFi.localIP().toString();

  });

  // CONFIG REST
  const size_t CONST_maxJsonBufferSize = 1024*10;
  this->addRestApiMethod("/api/config", [](_Application* theApp, const JsonObject* requestPostBody, const JsonObject* responseBody) {
    const JsonObject& root = (*responseBody);
    String out;
    serializeJsonPretty(theApp->getConfig().getJsonObject( ), out );
    bool success = root["config"].set( serialized(out));    
    if(!success) theApp->getLogger().error("Not enough reserved ram to print json configuration! Reserved maxJsonBufferSize: %u\n", CONST_maxJsonBufferSize);

  }, true, CONST_maxJsonBufferSize );
  
  // ScanWifi REST
  this->webServer->on("/api/scanWifi", HTTP_GET, _onScanWiFi);  
  
  // attach filesystem root at URL http://espressif.local/edit
  this->webServer->serveStatic("/spiffs/", SPIFFS, "/");

  
  //FIRMWARE UPDATE
  this->webServer->on("/api/firmwareUpdate", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", "<form method='POST' action='/firmwareUpdate' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>");
  });

  // TODO
  // ArRequestHandlerFunction _onFwUpdate=
  // webServer->on("/update", HTTP_POST,_onFwUpdate1, _onFwUpdate2);

  // Catch-All Handlers
  // Any request that can not find a Handler that canHandle it
  // ends in the callbacks below.  
  this->webServer->onNotFound( std::bind(_onNotFoundHandler, this->theApp->getLogger(), std::placeholders::_1) );
  this->webServer->onFileUpload( std::bind(_onUpload, this->theApp->getLogger(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6) );
  this->webServer->onRequestBody( std::bind(_onBody, this->theApp->getLogger(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5) );

  return this->additionalRestApiMethodSetup();
  //return _NoError;
}


 void _RestApiModule::beforeModuleAdded()
 {
    //remove core rest api if exists
    this->theApp->removeModule(  this->theApp->getBaseModule( ENUM_TO_STR(_CoreRestApiModule) ) );
 }