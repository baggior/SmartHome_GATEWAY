#include "coreapi.h"

#define STARTUP_DELAY_MS                2000
#define STARTUP_LOG_SERIAL              Serial
#define STARTUP_LOG_SERIAL_BAUDRATE     115200

////////////////////////////

_Error _NoError;

////////////////////////////


_Application::_Application()
: startupTimeMillis(millis())
{
#ifdef MY_DEBUG
    this->debug=true;
#endif

    //create core modules
    buildCoreModules();
}


_Application::~_Application()
{
    this->shutdown();
}

void _Application::buildCoreModules() 
{

}

_Error _Application::setup()
{
    delay(STARTUP_DELAY_MS);
    
    STARTUP_LOG_SERIAL.end();
    STARTUP_LOG_SERIAL.begin(STARTUP_LOG_SERIAL_BAUDRATE);
    STARTUP_LOG_SERIAL.println();

    if(this->debug)
        STARTUP_LOG_SERIAL.setDebugOutput(true);    
    
    //setup logger
    this->logger.setup(&STARTUP_LOG_SERIAL);    
    this->logger.printf(F("_Application setup start\n"));

    //setup config
    this->logger.printf(F("_Application config load start\n"));
    _Error ret = this->config.load();
    if(ret!=_NoError) 
    {
        this->logger.printf(F("ERROR in _Application config: %s (%d)\n"), 
            ret.message, ret.errorCode);
        return ret;
    }
    this->logger.printf(F("_Application config load done.\n"));

    //setup all modules in order
    this->logger.printf(F("_Application modules setup start\n"));
    for(_BaseModule* module : this->modules) {
        const _Error& err = module->setup();
        if(ret!=_NoError) 
        {
            this->logger.printf(F("ERROR in _Application modules setup: %s (%d)\n"), 
                ret.message, ret.errorCode);
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

void _Application::loop()
{
    for(_BaseModule* module : this->modules) {
        module->loop();
    }
}





////////////////////////////////////////////

void _ApplicationLogger::setup(HardwareSerial* hwserial)
{
    if(hwserial)
        this->dbgstream = hwserial;        
}
void _ApplicationLogger::setup(Stream* _dbgstream)
{
    if(_dbgstream)
        this->dbgstream = _dbgstream;
}
_ApplicationLogger::~_ApplicationLogger()
{

}

void _ApplicationLogger:: printf(const char *fmt, ...)
{
    if(this->dbgstream)
    {
        va_list args;
        va_start (args, fmt );
        Serial_printf(*this->dbgstream, fmt, args);
        va_end (args);
    }
}
void _ApplicationLogger::printf(const __FlashStringHelper *fmt, ...)
{
    if(this->dbgstream)
    {
        va_list args;
        va_start (args, fmt );
        Serial_printf(*this->dbgstream, fmt, args);
        va_end (args);
    }
}