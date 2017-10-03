#ifndef wifitelnetserver_h
#define wifitelnetserver_h

#include <WifiServer.h>
#include <WifiClient.h>

class WifiTelnetServer
{
    WiFiServer server;
    Stream * dbgstream;
    WiFiClient telnetClient;
       
    String lastCommandReceived;

    int MAX_TIME_INACTIVE; //in millis
    int port;
    bool enable;

public:
    WifiTelnetServer();

    void setup(Stream &serial);
    bool process(const String& msg="");
    WiFiClient getClient();
    inline String& getLastCommandReceived() {return lastCommandReceived;}

private:    
    unsigned long _lastTimeCommand;
    
    void manageNewConnections();
    void showHelp();
};

#endif