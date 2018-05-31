#ifndef wifiRestServerJsonHandler_h
#define wifiRestServerJsonHandler_h


/*
  Async Response to use with ArduinoJson and AsyncWebServer
  Written by Andrew Melvin (SticilFace) with help from me-no-dev and BBlanchon.

  Example of callback in use

   server.on("/json", HTTP_ANY, [](AsyncWebServerRequest * request) {

    AsyncJsonResponse * response = new AsyncJsonResponse();
    JsonObject& root = response->getRoot();
    root["key1"] = "key number one";
    JsonObject& nested = root.createNestedObject("nested");
    nested["key1"] = "key number one";

    response->setLength();
    request->send(response);
  });

  --------------------

  Async Request to use with ArduinoJson and AsyncWebServer
  Written by Arsène von Wyss (avonwyss)

  Example

  AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/rest/endpoint");
  handler->onRequest([](AsyncWebServerRequest *request, JsonVariant &json) {
    JsonObject& jsonObj = json.as<JsonObject>();
    // ...
  });
  server.addHandler(handler);
  
*/

#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>

static const char* JSON_MIMETYPE = "application/json";

typedef std::function<void(AsyncWebServerRequest *request, JsonVariant &json)> ArJsonRequestHandlerFunction;

class AsyncCallbackJsonWebHandler: public AsyncWebHandler {
private:
protected:
  const String _uri;
  WebRequestMethodComposite _method;
  ArJsonRequestHandlerFunction _onRequest;
  int _contentLength;
  int _maxContentLength;
public:
  AsyncCallbackJsonWebHandler(const String& uri, ArJsonRequestHandlerFunction onRequest) : _uri(uri), _method(HTTP_POST|HTTP_PUT|HTTP_PATCH), _onRequest(onRequest), _maxContentLength(16384) {}
  void setMethod(WebRequestMethodComposite method){ _method = method; }
  void setMaxContentLength(int maxContentLength){ _maxContentLength = maxContentLength; }
  void onRequest(ArJsonRequestHandlerFunction fn){ _onRequest = fn; }

  virtual bool canHandle(AsyncWebServerRequest *request) override final{
    if(!_onRequest)
      return false;

    if(!(_method & request->method()))
      return false;

    if(_uri.length() && (_uri != request->url() && !request->url().startsWith(_uri+"/")))
      return false;

    if (!request->contentType().equalsIgnoreCase(JSON_MIMETYPE))
      return false;

    request->addInterestingHeader("ANY");
    return true;
  }

  virtual void handleRequest(AsyncWebServerRequest *request) override final {
    if(_onRequest) {
      if (request->_tempObject != NULL) {
        DynamicJsonBuffer jsonBuffer;
        JsonVariant json = jsonBuffer.parse((uint8_t*)(request->_tempObject));
        if (json.success()) {
          _onRequest(request, json);
          return;
        }
      }
      request->send(_contentLength > _maxContentLength ? 413 : 400);
    } else {
      request->send(500);
    }
  }
  virtual void handleUpload(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) override final {
  }
  virtual void handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) override final {
    if (_onRequest) {
      _contentLength = total;
      if (total > 0 && request->_tempObject == NULL && total < _maxContentLength) {
        request->_tempObject = malloc(total);
      }
      if (request->_tempObject != NULL) {
        memcpy((uint8_t*)(request->_tempObject) + index, data, len);
      }
    }
  }
  virtual bool isRequestHandlerTrivial() override final {return _onRequest ? false : true;}
};
#endif
