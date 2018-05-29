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
    virtual ~_BaseModule() {}
    virtual _Error setup()=0;
    virtual void shutdown()=0;
    virtual void loop() =0;
    virtual String info() =0;

protected:   
    const _Application* theApp = NULL;
    String title;
    String descr;

};

class _Module : public _BaseModule {
public:    
    virtual ~_Module() {}
    virtual _Error setup() {return _NoError;}
    virtual void shutdown() {}
    virtual void loop() {}
    virtual String info() {return title + "\n" + descr;}

};

class _RestApiModule : public _Module {
public:
    virtual ~_RestApiModule() {}
};


///////////////////////////////////////////////////////
class _ApplicationLogger {
    friend _Application;
public:
    void setup(HardwareSerial* hwserial);
    void setup(Stream* dbgstream);
    virtual ~_ApplicationLogger();

    void printf(const char *fmt, ...);
    void printf(const __FlashStringHelper *fmt, ...);
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
    void printConfigFileTo(Stream& stream) ;

    static inline String getSoftwareVersion() { return SW_VERSION; }
    static String getDeviceInfoString(const char* crlf="\n");

private:
    _Error load(bool formatSPIFFSOnFails=false);

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

    void loop();

    inline unsigned long millisSinceStartup() {return millis() - this->startupTimeMillis;} 
    inline Stream* getLoggerStream() {return this->logger.dbgstream;}

    inline void addTask(Task& task) { this->runner.addTask(task); }
    inline Scheduler& getScheduler() {return this->runner;}

private:    
    void buildCoreModules();

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