#include "coreapi.h"


// #include "RemoteDebug.h"

static RemoteDebug Debug;

static void initRemoteDebug(String hostname)
{
    // Initialize the server (telnet or web socket) of RemoteDebug
    Debug.begin(hostname);

    Debug.setResetCmdEnabled(true); // Enable the reset command
    Debug.showProfiler(true); // To show profiler - time between messages of Debug
	Debug.showColors(true); // Colors

}

// void _ApplicationLogger::setup(HardwareSerial& hwserial)
// {    
//     this->dbgstream = &hwserial;        
// }
void _ApplicationLogger::setup(Stream* _dbgstream)
{
    if(_dbgstream)
        this->dbgstream = _dbgstream;
}

void _ApplicationLogger::setupRemoteLog(const String hostname)
{
    if(hostname.length()>0)
        initRemoteDebug(hostname);
}

_ApplicationLogger::~_ApplicationLogger()
{

}

void _ApplicationLogger::printf(const char *fmt, ...) const
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

void _ApplicationLogger::printf(const _Error& error) const
{
    if(error==_NoError)
        this->printf(F("[NoError]"));
    else
        this->printf(F("[Error (%d): %s]"), error.errorCode, error.message.c_str());
}


size_t _ApplicationLogger::write(uint8_t data)
{
    if(this->dbgstream)
    {
        return this->dbgstream->write(data);
    }
    return 0;
}