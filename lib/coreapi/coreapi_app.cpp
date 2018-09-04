#include "coreapi.h"

#define STARTUP_DELAY_MS                2000
#define STARTUP_LOG_SERIAL_BAUDRATE     115200

////////////////////////////////////////////


_Application::_Application()
:   startupTimeMillis(millis()), 
    netSvc(*this)
{
#ifdef DEBUG_OUTPUT
    this->debug=true;
#endif

    //create core modules
    addCoreModules();
}


_Application::~_Application()
{
    this->shutdown();
}

static _WifiConnectionModule Core_WifiConnectionModule;
static _RestApiModule Core_RestApiModule;
void _Application::addCoreModules() 
{
    //TODO add core modules in order
    this->addModule(&Core_WifiConnectionModule);
    this->addModule(&Core_RestApiModule);
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

    this->logger.printf(F("_Application setup start\n"));

    //setup main configuration
    this->logger.printf(F("_Application config load start\n"));
    _Error ret = this->config.load(this->logger);
    if(ret!=_NoError) 
    {
        this->logger.printf(F("ERROR in _Application config: %s (%d)\n"), 
            ret.message, ret.errorCode);
        return ret;
    }
    this->logger.printf(F("_Application config load done.\n"));

    //setup all modules in order
    this->logger.printf(F("_Application modules setup start\n"));
    for(_BaseModule* module : this->modules) 
    {
        this->logger.printf(F(">[%s] module: setup start\n"), module->getTitle().c_str());        
        const _Error& err = module->setup();
        if(ret==_NoError) 
        {
            module->setEnabled(true);
            this->logger.printf(F(">[%s] module: setup done (ENABLED)\n"), module->getTitle().c_str());
        }
        else if (ret==_Disable) 
        {
            module->setEnabled(false);
            this->logger.printf(F(">[%s] module: setup done (DISABLED)\n"), module->getTitle().c_str());
        }
        else
        {        
            module->setEnabled(false);
            this->logger.printf(F(">ERROR in [%s] module setup: %s (%d)\n"), 
                module->getTitle().c_str(), ret.message, ret.errorCode);                
            return ret;
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

void _Application::addModule(_BaseModule* module) 
{
    if(module) {
        // module->theApp = this;
        module->beforeModuleAdded(this);

        this->modules.push_back(module);
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
_BaseModule* _Application::getModule(String title)
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
    this->logger.printf(F("_Application::loop()begin, ") );
    for(_BaseModule* module : this->modules) 
    {
        if(module->executeInMainLoop)
        {
            if(module->isEnabled())
            {
                //module main loop
                this->logger.printf(F("[%s]loop(), "), module->getTitle().c_str() );
                module->loop();
            }
        }
    }

    // scheduled modules tasks loop
    bool idle = runner.execute();
    if(idle)
    {
        //app idle loop
        this->logger.printf(F("idleLoop(), ") );
        this->idleLoop();
    }
    this->logger.printf(F("_Application::loop()end. ") );
}


void _Application::restart()
{
    delay(3000);      
    this->getLogger().flush();

    baseutils::ESP_restart();      //soft resetart
    delay(5000);
}




////////////////////////////////////////////