
#ifndef websocket_h
#define websocket_h

#include <WebSocketsServer.h>


class WebSocket
{

    Stream * dbgstream;
    WebSocketsServer webSocketsServer;
    
    bool enable;
    
    void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) ;

public:
    WebSocket();

    void setup(Stream &dbgstream);
    void process();
};

#endif