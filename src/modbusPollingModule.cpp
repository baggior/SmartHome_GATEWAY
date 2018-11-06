#include "modbusPollingModule.h"

ModbusPollingModule::ModbusPollingModule() 
:   _TaskModule("ModbusPollingModule", "Scan configured registers values and forwards them to MQTT"),
    p_modbus(NULL)
{

}


_Error ModbusPollingModule::setup()  
{
    bool on = false;

    const JsonObject& root = this->theApp->getConfig().getJsonObject("modbusPolling");  
    if(root.success()) 
    {
        on = root["enable"];   
    }
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