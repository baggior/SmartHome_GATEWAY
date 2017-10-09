
#ifndef websocket_h
#define websocket_h

#include <Hash.h>
#include <WebSocketsServer.h>


class WebSocket
{

    Stream * dbgstream;
    WebSocketsServer webSocketsServer;
    CommandObject lastCommandReceived;
    
    bool enable;
    
    void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) ;

public:
    WebSocket();

    void setup(Stream &dbgstream);
    bool process();

    inline String& getLastCommandReceived() {return lastCommandReceived.inputCommand;}
    void sendResponse(String response, uint8_t clientNum=-1); // <0 ==> broadcast

private:

    void handleInputCommand(CommandObject& command);
    void showHello(uint8_t clientNum);

    Task taskReceiveCmd;
};

#endif