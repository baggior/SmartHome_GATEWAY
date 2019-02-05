#ifndef modbusgtwmodule_h
#define modbusgtwmodule_h


#include <coreapi.h>

#include "rs485ServiceModule.h"

/*
* Modbus-TCP
* Modbus-RTU (remote terminal unit)
*/
class ModbusTCPGatewayModule final : public _BaseModule
{    
public:
    
    ModbusTCPGatewayModule();
    virtual ~ModbusTCPGatewayModule();


protected:
    virtual _Error setup() final override;
    virtual _Error setup(const JsonObject &root) final;
    virtual void shutdown() final override;
    virtual void loop() final override;

private:   

    void rtuTransactionTask();
    uint16_t status = 0;
    
    unsigned int tcp_port = 502;
    // unsigned int default_slave_id = 1;
    Rs485ServiceModule* p_rs485 = NULL;

};


#endif
