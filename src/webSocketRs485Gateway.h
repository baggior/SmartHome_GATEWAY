
#ifndef websocket_h
#define websocket_h

#include "rs485.h"

#include "objectModel.h"

#include <WebSocketsServer.h>

// TODO migrate
class WebSocketRs485Gateway
{
    Rs485 rs485;
    Stream * dbgstream;
    WebSocketsServer webSocketsServer;
    CommandObject lastCommandReceived;
    
    bool enable;
    
    void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) ;

public:
    WebSocketRs485Gateway();

    int setup(Stream &dbgstream);
    bool process();

    inline String& getLastCommandReceived() {return lastCommandReceived.inputCommand;}
    void sendResponse(String response, uint8_t clientNum=-1); // <0 ==> broadcast

private:

    void handleInputCommand(CommandObject& command);
    void showHello(uint8_t clientNum);

    Task taskReceiveCmd;
};

#endif