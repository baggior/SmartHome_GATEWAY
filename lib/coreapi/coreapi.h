#ifndef _coreapi_h
#define _coreapi_h


#include <Arduino.h>
#include <stdint.h>
#include <ArduinoJson.h>

#define     _TASK_STD_FUNCTION
//#define     _TASK_SLEEP_ON_IDLE_RUN
//#define     _TASK_STATUS_REQUEST
#define     _TASK_WDT_IDS
//#define     _TASK_LTS_POINTER
#include <TaskSchedulerDeclarations.h>

#include <pragmautils.h>
#include <dbgutils.h>
#include <baseutils.h>

#include "../../.piolibdeps/Embedded Template Library_ID930/src/list.h"


class _Application;

class _Error {
public:
    inline _Error() {}
    inline _Error(int _errorCode, String _message): errorCode(_errorCode), message(_message) {}
    int errorCode=0;
    String message;

    inline bool operator==(const _Error& that)const {return that.errorCode==this->errorCode;}
    inline bool operator!=(const _Error& that)const {return !(that==*this);}
};

extern _Error _NoError;

class _BaseModule {
    friend _Application;
public:    
    _BaseModule(String _title, String _descr): title(_title), descr(_descr) {}
    virtual ~_BaseModule() {}

    virtual _Error setup()=0;
    virtual void shutdown()=0;
    virtual void loop() =0;
    
    inline String getTitle() {return this->title; }
    inline String getDescr() {return this->descr; }
    
    inline virtual String info() {return title + "\n" + descr;}
    inline void setEnabled(bool _enabled) {this->enabled = _enabled;}
    inline bool isEnabled() {return this->enabled;}

protected:   
    const _Application* theApp = NULL;
    bool enabled = false;

    String title;
    String descr;
};

class _Module : public _BaseModule {
public:    
    inline _Module(String _title, String _descr) : _BaseModule(_title, _descr) {}
    virtual ~_Module() {}

protected:
    virtual _Error setup()  { this->setEnabled(true); return _NoError;}
    virtual void shutdown() { this->setEnabled(false);}
    virtual void loop() {}
    
};

class AsyncWebServer;
class _RestApiModule : public _Module 
{
public:
    typedef std::function<void(JsonObject* requestPostBody,  JsonObject* responseBody)> RestHandlerCallback;
    
    inline _RestApiModule(): _Module(("_RestApiModule"), ("Rest Api module")) {}
    virtual ~_RestApiModule() {}

protected:
    virtual _Error setup() ;
    virtual void shutdown() ;
    virtual void loop() ;

    void addRestApiMethod(const char* uri, RestHandlerCallback callback, bool isGetMethod=true );
    virtual _Error restApiMethodSetup();

    AsyncWebServer * webServer = NULL;
    unsigned int _server_port = 0;    

};


///////////////////////////////////////////////////////
class _ApplicationLogger {
    friend _Application;
public:
    void setup(HardwareSerial& hwserial);
    void setup(Stream* dbgstream);
    virtual ~_ApplicationLogger();

    void printf(const char *fmt, ...) const;
    void printf(const __FlashStringHelper *fmt, ...) const;

    inline Stream* getStream()const {return this->dbgstream; }
private:
    Stream * dbgstream = NULL;
};

///////////////////////////////////////////////////////
class _ApplicationConfig {
    friend _Application;
public:
    _ApplicationConfig();
    virtual ~_ApplicationConfig();

    JsonObject* getJsonObject(const char* node=NULL);
    void printConfigFileTo(Stream* stream)const ;

    static inline String getSoftwareVersion() { return SW_VERSION; }
    static String getDeviceInfoString(const char* crlf="\n");

private:
    _Error load(_ApplicationLogger& logger, bool formatSPIFFSOnFails=false);

    JsonObject* jsonObject=NULL;
    String configJsonString;
};

///////////////////////////////////////////////////////

class _Application {

public:
    // Explicitly disable copy constructor and assignment
    _Application(const _Application& that) = delete;
    _Application& operator=(const _Application& that) = delete;

    _Application();
    ~_Application();
    
    _Error setup();
    void addModule(_BaseModule* module);
    void removeModule(_BaseModule* module);
    _BaseModule* getModule(String title);

    void loop();

    inline unsigned long millisSinceStartup() const {return millis() - this->startupTimeMillis;} 
    inline const _ApplicationLogger& getLogger() const {return this->logger;}

    inline void addTask(Task& task) { this->runner.addTask(task); }
    inline Scheduler& getScheduler() {return this->runner;}

private:    
    void addCoreModules();

    _Error webServerSetup();
    void shutdown();

    unsigned long startupTimeMillis=0;
    _ApplicationConfig config;
    etl::list<_BaseModule*, 100> modules;
    Scheduler runner;
    _ApplicationLogger logger;

    bool debug=false;
};



#endif // _coreapi_h