#ifndef modbuspollingmodule_h
#define modbuspollingmodule_h

#include <coreapi.h>

#include "modbusService.h"

class ModbusPollingModule :  public _TaskModule
{

public:
    ModbusPollingModule();
    // virtual ~ModbusPollingModule();

protected:
    virtual _Error setup() override;
    // inline virtual void loop() override { }

    ModbusService modbus;
};






#endif