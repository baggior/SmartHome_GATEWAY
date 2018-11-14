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
    addCoreModules();
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
        this->logger.setup(DEBUG_OUTPUT);    
    }

#endif 

    this->loopcnt = 0;
    this->startupTimeMillis = millis();

    this->logger.printf(F("_Application setup start\n"));
   
    //setup main configuration
    this->logger.printf(F("_Application config load start\n"));
    _Error err = this->config.load(this->logger);
    if(err!=_NoError) 
    {
        this->logger.printf(F("ERROR in _Application config: %s (%d)\n"), 
            err.message, err.errorCode);
        return err;
    }
    this->logger.printf(F("_Application config load done.\n"));

    //setup all modules in order
    this->logger.printf(F("_Application modules setup start\n"));

    if(this->isDebug()) {
        DPRINTF(F("\t (%d) Moduli: ["), this->modules.size());
        for(_BaseModule* module : this->modules) {
            DPRINTF(F("%s, "), module->info().c_str());
        }
        DPRINTF(F("]\n"));
    }

    for(_BaseModule* module : this->modules) 
    {
        this->logger.printf(F(">[%s] module: setup start\n"), module->getTitle().c_str());      

        err = module->setup();
        if(err==_NoError) 
        {
            module->setEnabled(true);
            this->logger.printf(F(">[%s] module: setup done (ENABLED)\n"), module->getTitle().c_str());
        }
        else if (err==_Disable) 
        {
            module->setEnabled(false);
            this->logger.printf(F(">[%s] module: setup done (DISABLED)\n"), module->getTitle().c_str());
        }
        else
        {        
            module->setEnabled(false);
            this->logger.printf(F(">ERROR in [%s] module setup: %s (%d)\n"), 
                module->getTitle().c_str(), err.message.c_str(), err.errorCode);                
            return err;
        }
        
    }
    
    this->logger.printf(F("_Application modules setup done.\n"));
    return _NoError;
}

void _Application::shutdown()
{
    this->logger.printf(F("_Application shutdown start\n"));
    for(_BaseModule* module : this->modules) {
        module->setEnabled(false);
        module->shutdown();        
    }    
    this->logger.printf(F("_Application shutdown completed.\n"));
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
        
        module->beforeModuleAdded();

        this->modules.push_back(module);

        auto comparator = std::bind(&_Application::modules_comparator, this, std::placeholders::_1, std::placeholders::_2);
        this->modules.sort(comparator);        

        this->logger.printf(F("_Application module added : [%s].\n"), module->getTitle().c_str() );
    }
}

void _Application::removeModule(_BaseModule* module) 
{
    if(module) {
        module->setEnabled(false);
        module->shutdown();  
        module->theApp = NULL;
        this->modules.remove(module);
        this->logger.printf(F("_Application module removed : [%s].\n"), module->getTitle().c_str() );

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

void _Application::loop()
{
    static long logmillis = 0 ;
    bool tolog = ( this->isDebug() && ( (millis() - logmillis) > 1000) ); 
    if(tolog) this->logger.printf(F("_Application::loop(%d)BEGIN, "), loopcnt );

    for(_BaseModule* module : this->modules) 
    {
        if(module->executeInMainLoop)
        {
            if(module->isEnabled())
            {
                //module main loop
                if(tolog) this->logger.printf(F("[%s]loop, "), module->getTitle().c_str() );
                module->loop();
            }
        }
    }

    // scheduled modules tasks loop
    bool idle = runner.execute();
    if(idle)
    {
        //app idle loop
        if(tolog) this->logger.printf(F("[IDLE]loop, ") );
        this->idleLoop();
    }

    if(tolog) this->logger.printf(F("_Application::loop END.\n") );

    this->loopcnt++;
    if(tolog) logmillis = millis(); 
}


void _Application::restart()
{
    delay(3000);      
    this->getLogger().flush();

    baseutils::ESP_restart();      //soft resetart
    delay(5000);
}




////////////////////////////////////////////