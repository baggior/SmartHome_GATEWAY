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

static uint8_t convertToRemoteDebugLevels(_ApplicationLogger::Loglevel_t level)
{
    switch (level)
    {
        case _ApplicationLogger::Loglevel_t::DebugLevel:
            return RemoteDebug::DEBUG;
            break;
        case _ApplicationLogger::Loglevel_t::InfoLevel:
            return RemoteDebug::INFO;
            break;
        case _ApplicationLogger::Loglevel_t::WarningLevel:
            return RemoteDebug::WARNING;
            break;
        case _ApplicationLogger::Loglevel_t::ErrorLevel:
            return RemoteDebug::ERROR;
            break;
    
        default:
            return RemoteDebug::DEBUG;        
            break;
    }

}

// void _ApplicationLogger::setup(HardwareSerial& hwserial)
// {    
//     this->dbgstream = &hwserial;        
// }
void _ApplicationLogger::setupSerialLog(Stream* _dbgstream, Loglevel_t level)
{
    if(_dbgstream) {
        this->dbgstream = _dbgstream;
        this->setLogLevel(level);
    }
}

void _ApplicationLogger::setupRemoteLog(const String hostname, Loglevel_t level)
{
    if(hostname.length()>0) {
        initRemoteDebug(hostname);
        this->setLogLevel(level);
    }
}

_ApplicationLogger::~_ApplicationLogger()
{

}

void _ApplicationLogger::loop()
{
    // RemoteDebug handle
    ::Debug.handle();
}

void _ApplicationLogger::info(const char * fmt, ...)
{
    va_list arg;
    va_start(arg, fmt);
    this->log(Loglevel_t::InfoLevel, fmt, arg);
    va_end(arg);
}
void _ApplicationLogger::warn(const char * fmt, ...)
{
    va_list arg;
    va_start(arg, fmt);
    this->log(Loglevel_t::WarningLevel, fmt, arg);
    va_end(arg);
}
void _ApplicationLogger::error(const char * fmt, ...)
{
    va_list arg;
    va_start(arg, fmt);
    this->log(Loglevel_t::ErrorLevel, fmt, arg);
    va_end(arg);
}
void _ApplicationLogger::debug(const char * fmt, ...)
{
    va_list arg;
    va_start(arg, fmt);
    this->log(Loglevel_t::DebugLevel, fmt, arg);
    va_end(arg);
}
void _ApplicationLogger::log(const Loglevel_t level, const char * fmt, ...)
{
    va_list arg;
    va_start(arg, fmt);
    this->log(level, fmt, arg);
    va_end(arg);
}

void _ApplicationLogger::log(const _Error& error) 
{
    Loglevel_t previousLevel =  this->getLogLevel();
    this->setLogLevel(Loglevel_t::ErrorLevel);
    size_t written = this->print(error);
    written += this->println();
    this->setLogLevel(previousLevel);
}

void _ApplicationLogger::log(const Loglevel_t level, const char * format, va_list& args)
{
    char loc_buf[64];
    char * temp = loc_buf;
    
    va_list copy;
    
    va_copy(copy, args);
    size_t len = vsnprintf(NULL, 0, format, args);
    va_end(copy);
    if(len >= sizeof(loc_buf)){
        temp = new char[len+1];
        if(temp == NULL) {
            return ;
        }
    }
    len = vsnprintf(temp, len+1, format, args);
    
    // actual log write
    Loglevel_t previousLevel =  this->getLogLevel();
    this->setLogLevel(level);
    size_t written = this->Print::write((uint8_t*)temp, len);
    // this->setLogLevel(previousLevel);
    //

    if(len >= sizeof(loc_buf)){
        delete[] temp;
    }
    return;
}
// cloned from Print.h
// void _ApplicationLogger::printf(const char *format, ...) const
// {    
//     char loc_buf[64];
//     char * temp = loc_buf;
//     va_list arg;
//     va_list copy;
//     va_start(arg, format);
//     va_copy(copy, arg);
//     size_t len = vsnprintf(NULL, 0, format, arg);
//     va_end(copy);
//     if(len >= sizeof(loc_buf)){
//         temp = new char[len+1];
//         if(temp == NULL) {
//             return ;
//         }
//     }
//     len = vsnprintf(temp, len+1, format, arg);
//     ((_ApplicationLogger*) this)->Print::write((uint8_t*)temp, len);
//     va_end(arg);
//     if(len >= sizeof(loc_buf)){
//         delete[] temp;
//     }
//     return;

// }

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
    
    uint8_t levelConverted = ::convertToRemoteDebugLevels(this->logLevel);
    if(Debug.isActive(levelConverted)) {
        ret = ::Debug.write(data);
    }
        
    return ret;
}



// void _ApplicationLogger::printf(const _Error& error) const
// {
//     if(error==_NoError)
//         this->printf(("[NoError]"));
//     else
//         this->printf(("[Error (%d): %s]"), error.errorCode, error.message.c_str());
// }



