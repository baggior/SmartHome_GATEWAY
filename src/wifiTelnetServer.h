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

    Task taskReceiveCmd;

public:
    WifiTelnetServer();

    void setup(Stream &serial);
    bool process();
    WiFiClient getClient();
    inline String& getLastCommandReceived() {return lastCommandReceived;}
    void send(const String& msg);

    
private:    
    unsigned long _lastTimeCommand;
    
    void manageNewConnections();
    void showHelp();
    void handleInputCommand(String& command);
    
};

#endif