#include "config.h"

#include "WebSocket.h"




void WebSocket::webSocketEvent(WStype_t type, uint8_t * payload, size_t length) 
{
    switch(type) {
        case WStype_DISCONNECTED:
            DEBUG_OUTPUT.printf("[WSc] Disconnected!\n");
            break;
        case WStype_CONNECTED: {
            DEBUG_OUTPUT.printf("[WSc] Connected to url: %s\n", payload);

            // send message to server when Connected
            this->webSocketsClient.sendTXT("Connected");
        }
            break;
        case WStype_TEXT:
            DEBUG_OUTPUT.printf("[WSc] get text: %s\n", payload);

            // send message to server
            // webSocket.sendTXT("message here");
            break;
        case WStype_BIN:
            DEBUG_OUTPUT.printf("[WSc] get binary length: %u\n", length);
            hexdump(payload, length);

            // send data to server
            // webSocket.sendBIN(payload, length);
            break;
    }

}


void WebSocket::setup(Stream &dbgstream)
{
    JsonObject & root = config.getJsonRoot();   

    this->enable = root["websocket"]["enable"];
    const char* _home_server_host = root["websocket"]["home_server_host"];
    int _home_server_port=root["websocket"]["home_server_port"];
    const char* _home_server_url = root["websocket"]["home_server_url"];
    const char* _home_server_auth_username = root["websocket"]["home_server_auth"]["username"];
    const char* _home_server_auth_password = root["websocket"]["home_server_auth"]["password"];

    DPRINTF("enable: %d, home_server_host: %s, home_server_port: %d, home_server_url: %s, home_server_auth_username: %s, home_server_auth_password: %s  \n", 
        enable, _home_server_host, _home_server_port, _home_server_url, _home_server_auth_username, _home_server_auth_password);

    if(!_home_server_port) _home_server_port=80;
    if(!_home_server_url) _home_server_url="/";

    if(enable)
    {
        // server address, port and URL
        webSocketsClient.begin(_home_server_host, _home_server_port, _home_server_url); 
        
        // event handler
        WebSocketsClient::WebSocketClientEvent clientevent_function = std::bind(&WebSocket::webSocketEvent, this, 
            std::placeholders::_1,std::placeholders::_2,std::placeholders::_3);
        webSocketsClient.onEvent(clientevent_function);
        
        if(_home_server_auth_username)
        {
            // use HTTP Basic Authorization this is optional remove if not needed
            webSocketsClient.setAuthorization(_home_server_auth_username, _home_server_auth_password);        
        }
        
        // try ever 5000 again if connection has failed
        webSocketsClient.setReconnectInterval(5000);
    }

}

void WebSocket::process()
{
    if(enable)
    {
        webSocketsClient.loop();
    }
}