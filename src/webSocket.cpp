#include "config.h"

#include "WebSocket.h"

#define WEBSOCKETSERVER_PORT_DEFAULT 81

void WebSocket::webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
    switch(type) {
        case WStype_DISCONNECTED:
            DEBUG_OUTPUT.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED:
            {
                IPAddress ip = this->webSocketsServer.remoteIP(num);
                DEBUG_OUTPUT.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
				
				// send message to client
				this->webSocketsServer.sendTXT(num, "Connected");
            }
            break;
        case WStype_TEXT:
            DEBUG_OUTPUT.printf("[%u] get Text: %s\n", num, payload);

            // send message to client
            // webSocket.sendTXT(num, "message here");

            // send data to all connected clients
            // webSocket.broadcastTXT("message here");
            break;
        case WStype_BIN:
            DEBUG_OUTPUT.printf("[%u] get binary length: %u\n", num, length);
            hexdump(payload, length);

            // send message to client
            // webSocket.sendBIN(num, payload, length);
            break;
    }


}


WebSocket::WebSocket()
:   webSocketsServer(WEBSOCKETSERVER_PORT_DEFAULT)
{
    
}

void WebSocket::setup(Stream &dbgstream)
{
    JsonObject & root = config.getJsonRoot();   

    this->enable = root["websocket"]["enable"];
    int _server_port=root["websocket"]["server_port"];

    const char* _home_server_host = root["websocket"]["home_server_host"];
    int _home_server_port=root["websocket"]["home_server_port"];
    const char* _home_server_url = root["websocket"]["home_server_url"];
    const char* _home_server_auth_username = root["websocket"]["home_server_auth"]["username"];
    const char* _home_server_auth_password = root["websocket"]["home_server_auth"]["password"];

    DPRINTF(">WebSocket SETUP: enable: %d, server_port: %d, home_server_host: %s, home_server_port: %d, home_server_url: %s, home_server_auth_username: %s, home_server_auth_password: %s  \n", 
        enable, _server_port, _home_server_host, _home_server_port, _home_server_url, _home_server_auth_username, _home_server_auth_password);

    if(!_server_port) _server_port=WEBSOCKETSERVER_PORT_DEFAULT;
    if(!_home_server_port) _home_server_port=WEBSOCKETSERVER_PORT_DEFAULT;
    if(!_home_server_url) _home_server_url="/";

    webSocketsServer.disconnect();
    if(enable)
    {
        // server address, port and URL
        webSocketsServer = WebSocketsServer(_server_port); //BUG
        webSocketsServer.begin(); 
        
        // event handler
        WebSocketsServer::WebSocketServerEvent serverevent_function = std::bind(&WebSocket::webSocketEvent, this, 
            std::placeholders::_1,std::placeholders::_2,std::placeholders::_3,std::placeholders::_4);
        webSocketsServer.onEvent(serverevent_function);
        
        if(_home_server_auth_username)
        {
            // use HTTP Basic Authorization this is optional remove if not needed
            webSocketsServer.setAuthorization(_home_server_auth_username, _home_server_auth_password);        
        }
        
       
    }

}

void WebSocket::process()
{
    if(enable)
    {
        webSocketsServer.loop();
    }
}