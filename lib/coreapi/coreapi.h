#ifndef _coreapi_h
#define _coreapi_h


#include <Arduino.h>
#include <stdint.h>

#include "coreapi_def.h"
#include "coreapi_errors.h"

#include <pragmautils.h>
#include <dbgutils.h>
#include <baseutils.h>

#include <list.h>

//3rd party libs
#include <ArduinoJson.h>
#include <ESPAsyncWiFiManager.h>

#define     _TASK_STD_FUNCTION
//#define     _TASK_SLEEP_ON_IDLE_RUN
//#define     _TASK_STATUS_REQUEST
#define     _TASK_WDT_IDS
//#define     _TASK_LTS_POINTER
#include <TaskSchedulerDeclarations.h>


class _Application;


class _BaseModule {

    friend _Application;

protected:           
    enum CoreModuleOrderEnum {
        Order_First, Order_BeforeNormal, Order_Normal, Order_AfterNormal, Order_Last
    };

public:    
    inline _BaseModule(String _title, String _descr, 
        bool _executeInMainLoop=true, CoreModuleOrderEnum _order=Order_Normal, bool _unique=false)
        : title(_title), descr(_descr), executeInMainLoop(_executeInMainLoop), order(_order), unique(_unique) {}
    inline virtual ~_BaseModule() {}
    
    inline String getTitle() const {return this->title; }
    inline String getDescr() const {return this->descr; }
    
    inline String info() const {return title + " (" + descr + ")";}
    inline virtual void setEnabled(bool _enabled) { this->enabled = _enabled; }
    inline bool isEnabled() const {return this->enabled;}

    inline virtual bool operator==(const _BaseModule& other) const { return this->getTitle().equals(other.getTitle()); }
       
protected:       
    
    virtual _Error setup()=0;    
    virtual void shutdown()=0;
    virtual void loop() =0;    

    //TODO remove
    inline virtual void beforeModuleAdded() {}
    inline virtual void afterModuleRemoved() {}

    _Application* theApp = NULL;

    String title;
    String descr;

private:
    
    bool enabled = false;  

    const bool executeInMainLoop;
    const CoreModuleOrderEnum order;
    const bool unique;
};

// class _ServiceModule : public _BaseModule
// {
// public:  

// protected:
//     inline _ServiceModule(String _title, String _descr) : _BaseModule(_title,_descr, false, Order_First) {}
    
//     virtual _Error setup(const JsonObject &root)=0;

// private:    
//     inline virtual void loop() final { } //task loop not used for a service module  
// };


class _WifiConnectionModule final : public _BaseModule 
{
public:    
    inline _WifiConnectionModule()  
#ifndef ESP32
    : _BaseModule( ENUM_TO_STR(_CoreWifiConnectionModule), ("Core Wifi Connection Api module"), true, Order_First) {}
#else
    // mDNS happens asynchronously on ESP32
    : _BaseModule( ENUM_TO_STR(_CoreWifiConnectionModule), ("Core Wifi Connection Api module"), false, Order_First) {}    
#endif    

protected:
    virtual _Error setup() override;
    virtual void shutdown() override;
    virtual void loop() override ; 
    virtual void beforeModuleAdded() override ;

    _Error wifiManagerOpenConnection();
};


class _TaskModule : public _BaseModule 
{
public:    
    _TaskModule(String _title, String _descr, unsigned int _taskLoopTimeMs=DEFAULT_TASK_LOOP_TIME_MS) ;
    inline virtual ~_TaskModule() { _TaskModule::shutdown(); }

    virtual void setEnabled(bool _enabled) override ;

protected:    
    virtual void shutdown() override;    
    // inline virtual void loop() override { }  //task loop    
    void taskloop();

    unsigned int taskLoopTimeMs;
    Task loopTask;    

private:
    long loopcnt = 0;

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
    
    inline _RestApiModule(): _BaseModule( ENUM_TO_STR(_CoreRestApiModule), ("Core Rest Api module"), false, Order_AfterNormal) {}
    inline virtual ~_RestApiModule() { this->shutdown(); }

protected:
    inline _RestApiModule(String _title, String _descr) : _BaseModule(_title, _descr, false) {}

    virtual _Error setup() override;
    virtual void shutdown() override;    
    inline virtual void loop() override final{} //nothing, unused for ASYNC server
    virtual void beforeModuleAdded() override ;

    virtual _Error additionalRestApiMethodSetup();
    void addRestApiMethod(const char* uri, RestHandlerCallback callback, bool isGetMethod=true );

private:
    _Error restApiMethodSetup();

    AsyncWebServer * webServer = NULL;
    unsigned int _server_port = 0;    

};


///////////////////////////////////////////////////////
class _ApplicationLogger {
    
public:
    void setup(HardwareSerial& hwserial);
    void setup(Stream* dbgstream);
    virtual ~_ApplicationLogger();

    void printf(const char *fmt, ...) const;
    void printf(const __FlashStringHelper *fmt, ...) const;
    inline void flush() const { if(dbgstream) dbgstream->flush(); }

    inline Stream* getStream()const {return this->dbgstream; }

private:
    friend _Application;

    Stream * dbgstream = NULL;
};

///////////////////////////////////////////////////////
class _ApplicationConfig {
    
public:
    _ApplicationConfig(_Application& _theApp);
    virtual ~_ApplicationConfig();

    const JsonObject& getJsonObject(const char* node=NULL)const;
    void printConfigTo(Stream* stream)const ;

    static inline String getSoftwareVersion() { return SW_VERSION; }
    static String getDeviceInfoString(const char* crlf="\n");

private:
    friend _Application;

    _Error load(_ApplicationLogger& logger, bool formatSPIFFSOnFails=false);

    _Error persist();

    _Application& theApp;
    const JsonObject* jsonObject=NULL;
};

///////////////////////////////////////////////////////
class _NetServices {

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
    friend _Application;

    inline _NetServices(_Application& _theApp) : theApp(_theApp) {}

    _Application& theApp;
};
///////////////////////////////////////////////////////

class _Application final {

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

    _BaseModule* getBaseModule(const String title) const;

    template<typename T, typename std::enable_if<std::is_base_of<_BaseModule, T>::value>::type* = nullptr> 
    inline T* getModule(const String title) const
    {
        return (T*) this->getBaseModule(title);
    }
    
    void loop();

    inline bool isDebug() { return this->debug; }
    inline bool isToLog() { return this->debug && this->toLog; }

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

    bool modules_comparator(const _BaseModule* modulea, const _BaseModule* moduleb) const;

    typedef etl::list<_BaseModule*, MAX_MODULES> ModuleListType;

    unsigned long startupTimeMillis=0;

    _NetServices netSvc;    
    _ApplicationConfig config;
    _ApplicationLogger logger;
    
    ModuleListType modules;
    Scheduler runner;

    bool debug=false;
    bool toLog=false;

    IdleLoopCallback idleLoopCallback_fn=NULL;
    long loopcnt = 0;
};





#endif // _coreapi_h

