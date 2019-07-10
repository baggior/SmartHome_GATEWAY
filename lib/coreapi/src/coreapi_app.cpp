#include "coreapi.h"
#include "coreapi_mqttmodule.h"

#include <TaskScheduler.h>

#include <functional>

////////////////////////////////////////////

#define STARTUP_DELAY_MS                2000
#define STARTUP_LOG_SERIAL_BAUDRATE     115200

////////////////////////////////////////////


_Application::_Application()
:   netSvc(*this), config(*this)
{

#ifdef DEBUG_OUTPUT
    this->debug=true;    
#endif 

    this->loopcnt = 0 ;
    this->startupTimeMillis = 0;

    //create core modules
    this->addCoreModules();
}


_Application::~_Application()
{
    this->shutdown();
}

static _WifiConnectionModule Core_WifiConnectionModule;
static _RestApiModule Core_RestApiModule;
static MqttModule Core_MqttModule;

void _Application::addCoreModules() 
{
    //TODO add core modules in order
    this->addModule(&Core_WifiConnectionModule);
    this->addModule(&Core_RestApiModule);
    this->addModule(&Core_MqttModule);
}

_Error _Application::setup()
{
    delay(STARTUP_DELAY_MS);

#ifdef DEBUG_OUTPUT
    DEBUG_OUTPUT.end();
    DEBUG_OUTPUT.begin(STARTUP_LOG_SERIAL_BAUDRATE);
    DEBUG_OUTPUT.println();

    if(this->debug)
    {
        DEBUG_OUTPUT.setDebugOutput(true);    
        //setup logger
        this->logger.setupSerialLog(&DEBUG_OUTPUT);    
    }

#endif 

    this->loopcnt = 0;
    this->startupTimeMillis = millis();

    this->logger.info(("_Application setup start\n"));
   
    //FS MOUNT
    this->logger.info(("_Application FS mount..\n"));
    _Error ret = this->mountFS();
    if(ret!=_NoError) return ret; 

    //setup main configuration
    this->logger.info(("_Application config load start\n"));
    _Error err = this->config.load(this->logger);
    if(err!=_NoError) 
    {
        this->logger.error(("ERROR in _Application config: %s (%d)\n"), 
            err.message.c_str(), err.errorCode);
        return err;
    }
    this->logger.info(("_Application config load done.\n"));

    //setup all modules in order
    this->logger.info(("_Application modules setup start\n"));

    if(this->isDebug()) {
        this->logger.debug(("\t (%d) Moduli: ["), this->modules.size());
        for(_BaseModule* module : this->modules) {
           this->logger.debug(("%s, "), module->info().c_str());
        }
        this->logger.debug(("]\n"));
    }

    for(_BaseModule* module : this->modules) 
    {
        this->logger.info((">[%s] module: setup start\n"), module->getTitle().c_str());      

        err = module->setup();
        if(err==_NoError) 
        {
            module->setEnabled(true);
            this->logger.info((">[%s] module: setup done (ENABLED)\n"), module->getTitle().c_str());
        }
        else if (err==_Disable) 
        {
            module->setEnabled(false);
            this->logger.info((">[%s] module: setup done (DISABLED)\n"), module->getTitle().c_str());
        }
        else
        {        
            module->setEnabled(false);
            this->logger.error((">ERROR in [%s] module setup: %s (%d)\n"), 
                module->getTitle().c_str(), err.message.c_str(), err.errorCode);                
            return err;
        }
        
    }
    
    this->logger.info(("_Application modules setup done.\n"));
    return _NoError;
}

void _Application::shutdown()
{
    this->logger.debug(("_Application modules shutdown (%d).. \n"), this->modules.size());
    
    ModuleList_t::reverse_iterator rit = this->modules.rbegin();
    for(; rit!= this->modules.rend(); ++rit) {
        _BaseModule* module = *rit;
        if(module) {
            module->setEnabled(false);
            module->shutdown();        
        }
    }    
    this->logger.info(("_Application modules shutdown done. \n"));

    //FS Un-MOUNT
    this->dismountFS();
    this->logger.debug(("_Application FS dismounted.\n"));

    this->logger.info(("_Application shutdown completed.\n"));
}

bool _Application::modules_comparator(const _BaseModule* modulea, const _BaseModule* moduleb) const
{
    //ordina prima i service
    return (modulea->order < moduleb->order);
}

void _Application::addModule(_BaseModule* module) 
{
    if(module) {
        module->theApp = this;
        
        if(module->unique)
        {
            auto * found = this->getBaseModule(module->getTitle());
            if(found) 
            {
                this->logger.error(("_Application module is unique and already exists: Cannot add another [%s].\n"), module->getTitle().c_str());
            }    
        }

        //TODO remove
        module->beforeModuleAdded();

        this->modules.push_back(module);

        auto comparator = std::bind(&_Application::modules_comparator, this, std::placeholders::_1, std::placeholders::_2);
        this->modules.sort(comparator);        

        this->logger.info(("_Application module added : [%s].\n"), module->getTitle().c_str() );
    }
}

void _Application::removeModule(_BaseModule* module) 
{
    if(module) {
        module->setEnabled(false);
        module->shutdown();  
        module->theApp = NULL;
        this->modules.remove(module);
        this->logger.info(("_Application module removed : [%s].\n"), module->getTitle().c_str() );

        module->afterModuleRemoved();
    }
}
_BaseModule* _Application::getBaseModule(const String title) const
{
    if(title.length()>0) 
    {
        for(_BaseModule* module : this->modules) 
        {
            if (title.equalsIgnoreCase(module->getTitle()) )
            {
                return module;
            }
        }
    }

    return NULL;
}

void _Application::idleLoop()
{
    if(this->idleLoopCallback_fn)
    {
        this->idleLoopCallback_fn();        
    }
}

// MAIN LOOP
void _Application::loop()
{
    // static long logmillis = 0 ;
    // long now = millis();
    // this->toLog = ( this->isDebug() && ( (now - logmillis) > 1000) ); 

    // if(this->isToLog()) {
    //     logmillis = now; 
    //     this->logger.printf(("_Application::loop(%d) BEGIN, "), loopcnt );
    // }     

    for(_BaseModule* module : this->modules) 
    {
        //run loop of modules that are to be executed in main loop
        if(module->executeInMainLoop)
        {
            if(module->isEnabled())
            {
                //module main loop
                // if(this->isToLog()) this->logger.printf(("[%s]loop, "), module->getTitle().c_str() );
                module->loop();
            }
        }
    }

    // scheduled modules tasks loop
    bool idle = runner.execute();
    if(idle)
    {
        //app idle loop
        // if(this->isToLog()) this->logger.printf(("[IDLE]loop, ") );
        this->idleLoop();
    }
    this->loopcnt++;

    // if(this->isToLog()) {
    //     this->logger.printf(("_Application::loop END.\n") );
    // }

    // handle log
    this->logger.loop();

    yield();
}


void _Application::restart(const _Error& error)
{
    if(error!=_NoError)
    {
        this->getLogger().log(error);
    }
    this->getLogger().info(("RESTART..\n"));
    this->getLogger().flush();

    this->shutdown();

    delay(3000);      

    //Hw 
    baseutils::ESP_restart();          
}

_Error _Application::mountFS()
{
    bool b = false;
#ifdef ESP32
    b = SPIFFS.begin(true);
#elif defined ESP8266
    b = SPIFFS.begin();
#endif

    if(!b)
    {
        logger.error((">Error opening SPIFFS filesystem\r\n"));   
        return _FSError;
    }
    else 
    {
#ifdef ESP32
        logger.info(("\tSPIFFS filesystem mounted (%d used Bytes)\n\n"), SPIFFS.usedBytes() );  
#elif defined ESP8266   
        logger.info(("SPIFFS filesystem mounted\r\n"));
#endif          
    }
    return _NoError;
}

void _Application::dismountFS()
{
    SPIFFS.end();
}



////////////////////////////////////////////