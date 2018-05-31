#include "coreapi.h"

#define STARTUP_DELAY_MS                2000
#define STARTUP_LOG_SERIAL_BAUDRATE     115200

////////////////////////////

_Error _NoError;

////////////////////////////


_Application::_Application()
: startupTimeMillis(millis())
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

void _Application::addCoreModules() 
{
    //TODO
}

_Error _Application::setup()
{
    delay(STARTUP_DELAY_MS);

#ifdef DEBUG_OUTPUT
    DEBUG_OUTPUT.end();
    DEBUG_OUTPUT.begin(STARTUP_LOG_SERIAL_BAUDRATE);
    DEBUG_OUTPUT.println();

    if(this->debug)
        DEBUG_OUTPUT.setDebugOutput(true);    

    //setup logger
    this->logger.setup(DEBUG_OUTPUT);    
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
        if(ret!=_NoError) 
        {
            this->logger.printf(F(">ERROR in %s module setup: %s (%d)\n"), 
                module->getTitle().c_str(), ret.message, ret.errorCode);                
            return ret;
        }
        else
        {
            this->logger.printf(F(">[%s] module: setup done\n"), module->getTitle().c_str());
        }
    }
    
    this->logger.printf(F("_Application modules setup done.\n"));
    return _NoError;
}

void _Application::shutdown()
{
    this->logger.printf(F("_Application shutdown start\n"));
    for(_BaseModule* module : this->modules) {
        module->shutdown();
        this->removeModule(module);
    }
    this->logger.printf(F("_Application shutdown completed.\n"));
}

void _Application::addModule(_BaseModule* module) 
{
    if(module) {
        module->theApp = this;
        this->modules.push_back(module);
    }
}

void _Application::removeModule(_BaseModule* module) 
{
    if(module) {
        module->theApp = NULL;
        this->modules.remove(module);
    }
}
_BaseModule* _Application::getModule(String title)
{
    if(title.length()>0) {
        for(_BaseModule* module : this->modules) {
            if (title.equalsIgnoreCase(module->getTitle()) )
            {
                return module;
            }
        }
    }

    return NULL;
}

void _Application::loop()
{
    for(_BaseModule* module : this->modules) 
    {
        if(module->isEnabled())
        {
            module->loop();
        }
    }
}





////////////////////////////////////////////

void _ApplicationLogger::setup(HardwareSerial& hwserial)
{    
    this->dbgstream = &hwserial;        
}
void _ApplicationLogger::setup(Stream* _dbgstream)
{
    if(_dbgstream)
        this->dbgstream = _dbgstream;
}
_ApplicationLogger::~_ApplicationLogger()
{

}

void _ApplicationLogger:: printf(const char *fmt, ...) const
{
    if(this->dbgstream)
    {
        va_list args;
        va_start (args, fmt );
        Stream_printf_args(*this->dbgstream, fmt, args);
        va_end (args);
    }
}
void _ApplicationLogger::printf(const __FlashStringHelper *fmt, ...) const
{
    if(this->dbgstream)
    {
        va_list args;
        va_start (args, fmt );
        Stream_printf_args(*this->dbgstream, fmt, args);
        va_end (args);
    }
}