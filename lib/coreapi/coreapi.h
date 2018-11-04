#ifndef _coreapi_h
#define _coreapi_h

#include "coreapi_config.h"

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

#include <list.h>

// #include "../../.piolibdeps/Embedded Template Library_ID930/src/list.h"


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
extern _Error _Disable;

class _BaseModule {
    friend _Application;
public:    
    _BaseModule(String _title, String _descr, bool _executeInMainLoop=true): title(_title), descr(_descr), executeInMainLoop(_executeInMainLoop) {}
    virtual ~_BaseModule() {}

    virtual _Error setup()=0;
    virtual void shutdown()=0;
    
    inline String getTitle() {return this->title; }
    inline String getDescr() {return this->descr; }
    
    inline virtual String info() {return title + " (" + descr + ")";}
    inline virtual void setEnabled(bool _enabled) { this->enabled = _enabled; }
    inline bool isEnabled() {return this->enabled;}

protected:       
    virtual void loop() =0;    
    inline virtual void beforeModuleAdded(_Application* app){this->theApp=app;}
    inline virtual void afterModuleRemoved(){}

    _Application* theApp = NULL;
    bool enabled = false;

    String title;
    String descr;
    
private:
    const bool executeInMainLoop;
};


class _WifiConnectionModule final : public _BaseModule 
{
public:    
    inline _WifiConnectionModule() : _BaseModule(("_CoreWifiConnectionModule"), ("Core Wifi Connection Api module")) {} ;
    inline virtual ~_WifiConnectionModule() {}

protected:
    virtual _Error setup() ;
    virtual void shutdown() ;
    virtual void loop() ; 
    virtual void beforeModuleAdded(_Application* app) override ;


    _Error wifiManagerOpenConnection();
};


class _TaskModule : public _BaseModule 
{
public:    
    _TaskModule(String _title, String _descr, unsigned int _taskLoopTimeMs=10) ;
    virtual ~_TaskModule();

    virtual void setEnabled(bool _enabled) override ;

protected:    
    virtual void shutdown();
    
    inline virtual void loop() { } //task loop    
    
    unsigned int taskLoopTimeMs;
    Task loopTask;
};

class AsyncWebServer;
/*
- This is fully asynchronous server and as such does not run on the loop thread.
- You can not use yield or delay or any function that uses them inside the callbacks
- The server is smart enough to know when to close the connection and free resources
- You can not send more than one response to a single request
*/
class _RestApiModule : public _BaseModule
{
public:
    typedef std::function<void(JsonObject* requestPostBody,  JsonObject* responseBody)> RestHandlerCallback;
    
    inline _RestApiModule(): _BaseModule(("_CoreRestApiModule"), ("Core Rest Api module")) {}
    inline _RestApiModule(String _title, String _descr) : _BaseModule(_title, _descr) {}
    inline virtual ~_RestApiModule() { this->shutdown(); }

protected:
    virtual _Error setup() ;
    virtual void shutdown() ;    
    inline virtual void loop() {} //nothing, unused for ASYNC server
    virtual void beforeModuleAdded(_Application* app) override ;

    virtual _Error restApiMethodSetup();
    void addRestApiMethod(const char* uri, RestHandlerCallback callback, bool isGetMethod=true );


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
    inline void flush() const { if(dbgstream) dbgstream->flush(); }

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

    const JsonObject& getJsonObject(const char* node=NULL)const;
    void printConfigTo(Stream* stream)const ;

    static inline String getSoftwareVersion() { return SW_VERSION; }
    static String getDeviceInfoString(const char* crlf="\n");

private:
    _Error load(_ApplicationLogger& logger, bool formatSPIFFSOnFails=false);

    const JsonObject* jsonObject=NULL;
    // String configJsonString;
};

///////////////////////////////////////////////////////
class _NetServices {
    friend _Application;
public:

    static String getHostname();
    static void printDiagWifi(Stream * dbgstream);

    //MDNS
    struct MdnsQueryResult {    
        uint16_t port;
        String host;
        IPAddress ip;
    };
    struct MdnsAttribute {
        String name;
        String value;
    };
    typedef etl::list<MdnsAttribute, MAX_MDNS_ATTRIBUTES> MdnsAttributeList;
    
    MdnsQueryResult mdnsQuery(String service, String proto);
    bool announceTheDevice(unsigned int server_port=80);
    bool announceTheDevice(unsigned int server_port, const MdnsAttributeList & attributes);

private:
    inline _NetServices(_Application& _theApp) : theApp(_theApp) {}

    _Application& theApp;
};
///////////////////////////////////////////////////////

class _Application {

public:
    typedef std::function<void ()> IdleLoopCallback;

    // Explicitly disable copy constructor and assignment
    _Application(const _Application& that) = delete;
    _Application& operator=(const _Application& that) = delete;

    _Application();
    ~_Application();
    
    void restart();

    _Error setup();
    void addModule(_BaseModule* module);
    void removeModule(_BaseModule* module);    
    _BaseModule* getModule(String title);

    void loop();

    inline bool isDebug() {return this->debug;}

    inline unsigned long millisSinceStartup() const {return millis() - this->startupTimeMillis;} 
    inline const _ApplicationLogger& getLogger() const {return this->logger;}
    inline const _ApplicationConfig& getConfig() const {return this->config;}

    inline Scheduler& getScheduler() {return this->runner;}
    inline _NetServices& getNetServices() {return this->netSvc;}

    inline void setIdleLoopCallback(IdleLoopCallback _idleLoopCallback_fn) {this->idleLoopCallback_fn=_idleLoopCallback_fn;}

private:    
    void addCoreModules();
    void idleLoop();
    
    void shutdown();

    unsigned long startupTimeMillis=0;

    typedef etl::list<_BaseModule*, MAX_MODULES> ModuleListType;

    _NetServices netSvc;    
    _ApplicationConfig config;
    ModuleListType modules;
    Scheduler runner;
    _ApplicationLogger logger;

    bool debug=false;

    IdleLoopCallback idleLoopCallback_fn=NULL;
    long loopcnt = 0;
};


#endif // _coreapi_h

