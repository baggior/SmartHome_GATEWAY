
#ifndef websocket_h
#define websocket_h

#include <WebSocketsClient.h>


class WebSocket
{

    Stream * dbgstream;
    WebSocketsClient webSocketsClient;
    
    bool enable;

    void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) ;

public:
    WebSocket() {}

    void setup(Stream &dbgstream);
    void process();
};

#endif