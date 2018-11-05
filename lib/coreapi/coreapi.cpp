#include "coreapi.h"



////////////////////////////

_Error _NoError;
_Error _Disable(1000, "Disable module request");
_Error _ConfigPersistError(1, "Cannot write json config file");
////////////////////////////



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