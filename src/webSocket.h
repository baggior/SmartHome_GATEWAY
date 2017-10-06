
#ifndef websocket_h
#define websocket_h

#include <Hash.h>
#include <WebSocketsServer.h>


class WebSocket
{

    Stream * dbgstream;
    WebSocketsServer webSocketsServer;
    String lastCommandReceived;
    
    bool enable;
    
    void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) ;

public:
    WebSocket();

    void setup(Stream &dbgstream);
    bool process();

    inline String& getLastCommandReceived() {return lastCommandReceived;}
    void send(const String& s_msg);
};

#endif