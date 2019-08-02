#ifndef rs485monitormodule_h
#define rs485monitormodule_h


#include <coreapi.h>

#include "rs485ServiceModule.h"

#define BUFFER_MAX_SIZE (10 * 1024) //10 Kbytes

/*
* 
*/
class Rs485MonitorModule final : public _BaseModule
{    
public:
    
    Rs485MonitorModule();
    virtual ~Rs485MonitorModule();


protected:
    virtual _Error setup() final override;
    
    virtual void shutdown() final override;
    virtual void loop() final override;

private:   

    
    void serialTransactionTask();

    Rs485ServiceModule* p_rs485 = NULL;
    uint16_t status = 0;
    uint32_t status_millis = 0;

    String monitor_post_rest_url;    
    bool modbus_ascii = false;

    uint8_t frameBuffer[BUFFER_MAX_SIZE];
    uint16_t frameBuffer_len=0;
};


#endif
