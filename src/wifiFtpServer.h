#pragma once

#include <coreapi.h>

#include <ESP8266FtpServer.h>


class WifiFtpServer: public _TaskModule
{
public:
    WifiFtpServer();
    virtual ~WifiFtpServer();
    
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