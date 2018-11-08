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
        this->p_modbus = this->theApp->getModule<ModbusServiceModule>("ModbusServiceModule");
        if(!this->p_modbus)
        {
            this->theApp->getLogger().printf(F(">ModbusPollingModule Error servizio ModbusServiceModule non esistente\n"));
            return _Error(2, "ModbusPollingModule Error: servizio ModbusServiceModule non esistente");            
        }

        const JsonObject& memoryConfig = root["memoryConfig"];  
        if(memoryConfig.success())  
        {            
            this->p_modbus->buildDataMemory(memoryConfig);
            size_t coils = this->p_modbus->getModbusDataMemory().getCoils().size();
            size_t regs = this->p_modbus->getModbusDataMemory().getRegisters().size();

            this->theApp->getLogger().printf(F(">ModbusPollingModule: ModbusDataMemory inizializzata: %d coils, %d registers \n")
                , coils, regs );
        }
        else
        {
            this->theApp->getLogger().printf(F(">ModbusPollingModule: Configurazione della ModbusDataMemory non esistente\n"));
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
    if(this->p_modbus) {
        this->p_modbus->updateDataMemoryValues();
    }

    // TODO publish
}