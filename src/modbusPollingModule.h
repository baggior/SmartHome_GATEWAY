#ifndef modbuspollingmodule_h
#define modbuspollingmodule_h

#include <coreapi.h>

#include "modbusServiceModule.h"
#include "mqttServiceModule.h"

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
    MqttServiceModule* p_mqtt = NULL;

    ModbusDataMemory modbusDataMemory;
};






#endif