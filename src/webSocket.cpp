#include "config.h"

#include "WebSocket.h"
#include "Rs485.h"


#define WEBSOCKETSERVER_PORT_DEFAULT 81
#define WEBSOCKETSERVER_PROTOCOL_DEFAULT "SHGw"
#define WEBSOCKET_LISTEN_TASK_INTERVAL_DEFAULT 1 //ms


extern Scheduler runner;
extern Rs485 rs485;


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
                this->showHello(num);
				//this->webSocketsServer.sendTXT(num, "Connected");
            }
            break;
        case WStype_TEXT:
            {
                DEBUG_OUTPUT.printf("[%u] get Text: %s\n", num, payload);            
                
                // send message to client
                this->webSocketsServer.sendTXT(num, String("ECHO: [") + ((const char *)payload) + "]");
                
                // send data to all connected clients
                // webSocket.broadcastTXT("message here");
                String r_CMD = (const char *)payload;
                if(r_CMD.length()>0)
                {                    
                    this->lastCommandReceived.inputCommand = r_CMD;
                    this->lastCommandReceived.clientNum = num;
                    this->handleInputCommand(lastCommandReceived);
                }
                
                break;
            }
        case WStype_BIN:
            DEBUG_OUTPUT.printf("[%u] get binary length: %u\n", num, length);
            hexdump(payload, length);

            // send message to client
            this->webSocketsServer.sendBIN(num, payload, length);
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
    
    const char* _server_auth_username = root["websocket"]["server_auth"]["username"];
    const char* _server_auth_password = root["websocket"]["server_auth"]["password"];

    const char* _protocol = root["websocket"]["protocol"];

    int task_listen_interval = root["telnet"]["task_listen_interval"];

    DPRINTF(">WebSocket SETUP: enable: %d, server_port: %d, protocol: %s, server_auth_username: %s, server_auth_password: %s, task_listen_interval: %d \n", 
        enable, _server_port, _protocol, _server_auth_username, _server_auth_password, task_listen_interval);

    if(!_server_port) _server_port=WEBSOCKETSERVER_PORT_DEFAULT;    
    if(!_protocol) _protocol =WEBSOCKETSERVER_PROTOCOL_DEFAULT;
    if(!task_listen_interval) task_listen_interval= WEBSOCKET_LISTEN_TASK_INTERVAL_DEFAULT;
    webSocketsServer.disconnect();

    if(enable)
    {
        // server address, port and URL
        webSocketsServer = WebSocketsServer(_server_port, "", _protocol); 
        webSocketsServer.begin(); 
        
        // event handler
        WebSocketsServer::WebSocketServerEvent serverevent_function = std::bind(&WebSocket::webSocketEvent, this, 
            std::placeholders::_1,std::placeholders::_2,std::placeholders::_3,std::placeholders::_4);
        webSocketsServer.onEvent(serverevent_function);
        
        if(_server_auth_username)
        {
            // use HTTP Basic Authorization this is optional remove if not needed
            webSocketsServer.setAuthorization(_server_auth_username, _server_auth_password);        
        }
        
        //TASK setting
        TaskCallback funct = std::bind(&WebSocket::process, this);
        taskReceiveCmd.set(task_listen_interval
            , TASK_FOREVER
            , funct
            );
        runner.addTask(taskReceiveCmd);
        taskReceiveCmd.enable();
       
    }

}

bool WebSocket::process()
{
    if(enable)
    {
        lastCommandReceived.reset();
        webSocketsServer.loop();
        if (lastCommandReceived)
        {
            return true;
        }
    }
    return false;
}

void WebSocket::handleInputCommand(CommandObject& command)
{
    //TODO
    if(command.inputCommand.startsWith("/"))
    {
        showHello(command.clientNum);
    }
    else
    {
        if(command.inputCommand.startsWith(":"))
        {
            
            if(command.inputCommand.startsWith(":00"))
            {
                rs485.broadcastMasterCommand(command.inputCommand);
            }
            else
            {
                String response = rs485.sendMasterCommand(command.inputCommand);
                sendResponse(response, command.clientNum);
            }
        }
    }
}

void WebSocket::showHello( uint8_t clientNum)
{
    String info = Config::getDeviceInfoString("\r\n");
    this->webSocketsServer.sendTXT(clientNum, "Connected:\n" + info);
}


void WebSocket::sendResponse(String response, uint8_t clientNum)
{
    if(clientNum<0)
    {
        this->webSocketsServer.broadcastTXT(response);
    }
    else
    {
        this->webSocketsServer.sendTXT(clientNum, response);
    }
}