#pragma once

#include <ESP8266FtpServer.h>


class WifiFtpServer
{
public:
    WifiFtpServer() {}
    
    void setup(Stream &serial); 
    inline void process() { ftpServer.handleFTP(); }

private:
    FtpServer ftpServer;
    bool enable;    
    
    Task taskReceiveCmd;

};