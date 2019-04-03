#pragma once

#include "coreapi.h"

#include <ESP8266FtpServer.h>


class WifiFtpServerModule final : public _TaskModule
{
public:
    WifiFtpServerModule();
    virtual ~WifiFtpServerModule();
    
    // void setup(Stream &serial); 
    // inline void process() { ftpServer.handleFTP(); }

protected:
    virtual _Error setup() override;

    inline virtual void loop() override { ftpServer.handleFTP(); }

private:
    FtpServer ftpServer;
    // bool enable;    
    
    // Task taskReceiveCmd;

};