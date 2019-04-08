#include "modbusPollingModule.h"

#define DEFAULT_MODBUS_TASK_LOOP_TIME_MS 60000 //each 60 seconds

#define DATAMEMORY_LOG_FILE "/LOG/modbuspollingdata.json"


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
    if(!root.isNull()) 
    {
        on = root["enable"];   
        task_listen_interval = root["task_listen_interval"];   
        //TODO..
    }

    if(!task_listen_interval) task_listen_interval=DEFAULT_MODBUS_TASK_LOOP_TIME_MS;
    this->taskLoopTimeMs = task_listen_interval;

    this->theApp->getLogger().info (("\t%s config: enable: %u,.. , taskLoopTimeMs: %d \n"),
        this->getTitle().c_str(), on
        , this->taskLoopTimeMs);
    
    if(on) 
    {
        //MODBUS
        this->p_modbus = this->theApp->getModule<ModbusServiceModule>("ModbusServiceModule");
        if(!this->p_modbus)
        {
            this->theApp->getLogger().error((">ModbusPollingModule Error servizio ModbusServiceModule non esistente\n"));
            return _Error(2, "ModbusPollingModule Error: servizio ModbusServiceModule non esistente");            
        }

        const JsonArray& memoryConfigArray = root["dataMemoryConfig"];  
        if(!memoryConfigArray.isNull()) 
        {            
            this->p_modbus->buildDataMemory(memoryConfigArray, this->modbusDataMemory);

            size_t coils = this->modbusDataMemory.getCoils().size();
            size_t regs = this->modbusDataMemory.getRegisters().size();

            this->theApp->getLogger().debug((">ModbusPollingModule: ModbusDataMemory inizializzata: %d coils, %d registers \n")
                , coils, regs );
        }
        else
        {
            this->theApp->getLogger().warn((">ModbusPollingModule: Configurazione della ModbusDataMemory non esistente\n"));
            return _Disable;
        }

        //MQTT
        this->p_mqtt = this->theApp->getModule<MqttModule>(ENUM_TO_STR(_CoreMqttModule));
        if(!this->p_mqtt)
        {
            this->theApp->getLogger().error((">MqttModule Error servizio MqttModule non esistente\n"));
            return _Error(2, "MqttModule Error: servizio MqttModule non esistente");            
        }
        //TODO
        return _NoError;    
    }
    else
    {
        return _Disable;
    }   
}

void ModbusPollingModule::publish(const ModbusDataMemory& modbusDataMemory)
{
    if(!this->p_mqtt) return;
    const etl::ivector<ModbusDataMemory::Item> & coils = modbusDataMemory.getCoils();
    for (const ModbusDataMemory::Item & item : coils) 
    {
        this->p_mqtt->publish(item.name, item.value);
    }
    
    const etl::ivector<ModbusDataMemory::Item> & registers = modbusDataMemory.getRegisters();
    for (const ModbusDataMemory::Item & item : registers) 
    {
        this->p_mqtt->publish(item.name, item.value);
    }

}

void ModbusPollingModule::loop()
{
    if(this->p_modbus) 
    {
        this->p_modbus->updateDataMemoryValues(this->modbusDataMemory);

        if(this->p_mqtt) {                       
            this->publish(this->modbusDataMemory);
        }
        
        if(this->theApp->isDebug())
        {
            File f = baseutils::openFile(DATAMEMORY_LOG_FILE, "w");
            this->modbusDataMemory.printDataMemory(f);
            f.close();
        }
    }
}