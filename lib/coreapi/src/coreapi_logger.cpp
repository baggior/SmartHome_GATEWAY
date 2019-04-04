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

    Debug.showTime(true); // Show time in millis
    Debug.showDebugLevel(true); // Show debug level
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

void _ApplicationLogger::loop()
{
    // RemoteDebug handle
    Debug.handle();
}

void _ApplicationLogger::printf(const char *format, ...) const
{    
    char loc_buf[64];
    char * temp = loc_buf;
    va_list arg;
    va_list copy;
    va_start(arg, format);
    va_copy(copy, arg);
    size_t len = vsnprintf(NULL, 0, format, arg);
    va_end(copy);
    if(len >= sizeof(loc_buf)){
        temp = new char[len+1];
        if(temp == NULL) {
            return ;
        }
    }
    len = vsnprintf(temp, len+1, format, arg);
    ((_ApplicationLogger*) this)->Print::write((uint8_t*)temp, len);
    va_end(arg);
    if(len >= sizeof(loc_buf)){
        delete[] temp;
    }
    return;

}

// void _ApplicationLogger::printf(const char *fmt, ...) const
// {
//     if(this->dbgstream || Debug.isActive(Debug.DEBUG))
//     {
//         va_list args;
//         va_start (args, fmt );
        
//         if(this->dbgstream)
//             Stream_printf_args(*this->dbgstream, fmt, args);        

//         // if(Debug.isActive(Debug.ANY))
//             // Debug.printf(fmt, args);    //TODO: test        
//             Debug.printf( (String("(C%d) ") + String(fmt)).c_str(), xPortGetCoreID(), args);

//         va_end (args);
//     }
// }
// void _ApplicationLogger::printf(const __FlashStringHelper *fmt, ...) const
// {
//     if(this->dbgstream || Debug.isActive(Debug.DEBUG))
//     {
//         va_list args;
//         va_start (args, fmt );
        
//         // if(this->dbgstream)
//         //     Stream_printf_args(*this->dbgstream, fmt, args);
        
//         // if(Debug.isActive(Debug.ANY))
//             // Debug.printf(reinterpret_cast<const char *> (fmt), args); //TODO: test
//             // Debug.printf( (String("(C%d) ") + String(reinterpret_cast<const char *> (fmt))).c_str(), xPortGetCoreID(), args);

//         ((_ApplicationLogger*) this)->Print::print(fmt);

//         va_end (args);
//     }
// }

// print implementation
size_t _ApplicationLogger::write(uint8_t data)
{
    size_t ret = 0;
    if(this->dbgstream)
    {
        ret = this->dbgstream->write(data);
    }
    
    size_t ret1 = Debug.write(data);
        
    return ret ? ret : ret1;
}



void _ApplicationLogger::printf(const _Error& error) const
{
    if(error==_NoError)
        this->printf(("[NoError]"));
    else
        this->printf(("[Error (%d): %s]"), error.errorCode, error.message.c_str());
}



