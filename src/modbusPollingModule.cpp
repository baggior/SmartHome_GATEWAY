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
        //TODO
    }

    this->theApp->getLogger().printf( F("\t%s config: enable: %u,...\n"),
        this->getTitle().c_str(), on);
    
    if(on) 
    {
        p_modbus = this->theApp->getServiceModule<ModbusServiceModule>("ModbusServiceModule");
        if(!p_modbus)
        {
            this->theApp->getLogger().printf(F(">ModbusPollingModule Error servizio ModbusServiceModule non esistente\n"));
            return _Error(2, "ModbusPollingModule Error: servizio ModbusServiceModule non esistente");            
        }

        return _NoError;    
    }
    else
    {
        return _Disable;
    }   
}


void ModbusPollingModule::loop()
{
    // TODO 
    if(this->p_modbus)
        this->p_modbus->updateDataMemoryValues();
}