#include "modbusPollingModule.h"

ModbusPollingModule::ModbusPollingModule() 
:  _TaskModule("ModbusPollingModule", "Scan configured registers values and forwards them to MQTT")
{

}


_Error ModbusPollingModule::setup()  
{
    bool on = true;
    //TODO

    if(on)
    {
        return _NoError;   
    }
    else
    {
        return _Disable;
    }   
}