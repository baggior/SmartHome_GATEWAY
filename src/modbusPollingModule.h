#ifndef modbuspollingmodule_h
#define modbuspollingmodule_h

#include <coreapi.h>
#include <coreapi_mqttmodule.h>

#include "modbusServiceModule.h"

class ModbusPollingModule :  public _TaskModule
{

public:
    ModbusPollingModule();
    // virtual ~ModbusPollingModule();

protected:
    virtual _Error setup() override;
    virtual void loop() override;

    void publish(const ModbusDataMemory& modbusDataMemory);

    ModbusServiceModule* p_modbus = NULL;
    MqttModule* p_mqtt = NULL;

    ModbusDataMemory modbusDataMemory;
};






#endif