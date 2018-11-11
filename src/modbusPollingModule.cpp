#include "modbusPollingModule.h"

#define DEFAULT_MODBUS_TASK_LOOP_TIME_MS 60000 //each 60 seconds

ModbusPollingModule::ModbusPollingModule() 
:   _TaskModule("ModbusPollingModule", "Scan configured registers values and forwards them to MQTT", DEFAULT_MODBUS_TASK_LOOP_TIME_MS),
    p_modbus(NULL)
{

}


_Error ModbusPollingModule::setup()  
{
    bool on = false;
    int task_listen_interval = 0;

    const JsonObject& root = this->theApp->getConfig().getJsonObject("modbusPolling");  
    if(root.success()) 
    {
        on = root["enable"];   
        task_listen_interval = root["task_listen_interval"];   
        //TODO..
    }

    if(!task_listen_interval) task_listen_interval=DEFAULT_MODBUS_TASK_LOOP_TIME_MS;
    this->taskLoopTimeMs = task_listen_interval;

    this->theApp->getLogger().printf( F("\t%s config: enable: %u,.. , taskLoopTimeMs: %d \n"),
        this->getTitle().c_str(), on
        , this->taskLoopTimeMs);
    
    if(on) 
    {
        //MODBUS
        this->p_modbus = this->theApp->getModule<ModbusServiceModule>("ModbusServiceModule");
        if(!this->p_modbus)
        {
            this->theApp->getLogger().printf(F(">ModbusPollingModule Error servizio ModbusServiceModule non esistente\n"));
            return _Error(2, "ModbusPollingModule Error: servizio ModbusServiceModule non esistente");            
        }

        const JsonArray& memoryConfigArray = root["dataMemoryConfig"];  
        if(memoryConfigArray.success())  
        {            
            this->p_modbus->buildDataMemory(memoryConfigArray);
            size_t coils = this->p_modbus->getModbusDataMemory().getCoils().size();
            size_t regs = this->p_modbus->getModbusDataMemory().getRegisters().size();

            this->theApp->getLogger().printf(F(">ModbusPollingModule: ModbusDataMemory inizializzata: %d coils, %d registers \n")
                , coils, regs );
        }
        else
        {
            this->theApp->getLogger().printf(F(">ModbusPollingModule: Configurazione della ModbusDataMemory non esistente\n"));
        }

        //MQTT
        this->p_mqtt = this->theApp->getModule<MqttServiceModule>("MqttServiceModule");
        if(!this->p_mqtt)
        {
            this->theApp->getLogger().printf(F(">MqttServiceModule Error servizio MqttServiceModule non esistente\n"));
            return _Error(2, "MqttServiceModule Error: servizio MqttServiceModule non esistente");            
        }
        //TODO
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
    if(this->p_mqtt) {

    }
}