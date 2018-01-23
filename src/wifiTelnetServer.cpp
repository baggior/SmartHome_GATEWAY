#include "config.h"

#ifdef ESP8266
#include <ESP8266WiFi.h>
#elif defined ESP32
#include <WiFi.h>
#endif
#include <TextFinder.h>

#include "wifiTelnetServer.h"
#include "rs485.h"

#define MAX_TIME_INACTIVE_DEFAULT 10*60*1000 //10 minuti (in millis)
#define TELNET_LISTEN_PORT_DEFAULT 23
#define TELNET_LISTEN_TASK_INTERVAL_DEFAULT 100 //ms


extern Scheduler runner;
//extern Rs485 rs485;

WifiTelnetServer::WifiTelnetServer()
    : port(TELNET_LISTEN_PORT_DEFAULT), MAX_TIME_INACTIVE(MAX_TIME_INACTIVE_DEFAULT), enable(true),
    server(port), dbgstream(NULL)
{
    
}

void WifiTelnetServer::setup(Stream &serial)
{
    JsonObject & root = config.getJsonRoot();    
    enable = root["telnet"]["enable"];
    int _port = root["telnet"]["port"];
    int _MAX_TIME_INACTIVE = root["telnet"]["inactivetime"];    
    int task_listen_interval = root["telnet"]["task_listen_interval"];
    
    DPRINTF(">Telnet Server SETUP: enable: %d, port: %d, MAX_TIME_INACTIVE: %d, task_listen_interval: %d \n", 
        enable, _port, _MAX_TIME_INACTIVE, task_listen_interval);
    
    if(_port) port=_port;
    if(_MAX_TIME_INACTIVE) MAX_TIME_INACTIVE=_MAX_TIME_INACTIVE * 1000;
    if(!task_listen_interval) task_listen_interval=TELNET_LISTEN_TASK_INTERVAL_DEFAULT;

    dbgstream = &serial;

    server.stop();
    if(enable)
    {
        server = WiFiServer(port);
        server.begin();
        server.setNoDelay(true);
    
        // Print the IP address
        serial.print("Server telnet started on IP:");
        serial.print(WiFi.localIP());
        serial.print(" port: ");
        serial.println(port);
    }

    TaskCallback funct = std::bind(&WifiTelnetServer::process, this);
    taskReceiveCmd.set(task_listen_interval
        , TASK_FOREVER
        , funct
        );
    runner.addTask(taskReceiveCmd);
    taskReceiveCmd.enable();
}

WiFiClient WifiTelnetServer::getClient()
{
    return telnetClient;
}

void WifiTelnetServer::manageNewConnections()
{
    if (server.hasClient())
    {     
        if (telnetClient && telnetClient.connected())
        {
            // Verify if the IP is same than actual conection
            WiFiClient newClient;
            newClient = server.available();
            String ip = newClient.remoteIP().toString();
            String oldip = telnetClient.remoteIP().toString();
            if (ip == oldip)
            {
                // Reconnect
                dbgstream->println("Telnet Client Renew. IP:" + oldip);
                telnetClient.stop();
                telnetClient = newClient;
            }
            else
            {
                // Desconnect (not allow more than one connection)
                dbgstream->println("Another Telnet Client is Already in use. IP: " + ip);
                newClient.stop();
                return;
            }
        }
        else
        {
            // New TCP client
            telnetClient = server.available();
        }

        if (!telnetClient)
        { // No client yet ???
            return;
        }

        // Set client
        String ip = telnetClient.remoteIP().toString();
        dbgstream->println("Telnet Client Open " + ip);
        telnetClient.setNoDelay(true); // More faster
        telnetClient.flush();          // clear input buffer, else you get strange characters

        _lastTimeCommand = millis(); // To mark time for inactivity

        // Show the initial message
        showHelp();

        // Empty buffer in
        while (telnetClient.available())
        {
            telnetClient.read();
        }
    }
}

void WifiTelnetServer::send(const String& s_msg)
{    
    //SEND
    if(enable && s_msg.length()>0)
    {
        if(telnetClient && telnetClient.connected())
        {                    
            //ECHO Telnet client
            telnetClient.flush();  //??
            telnetClient.println(s_msg);

            // print to debugstream            
            dbgstream->println("Telnet OUT -> " + s_msg);
        }
    }
}

void WifiTelnetServer::handleInputCommand(String& command)
{
    //Stream s;
    //TextFinder finder(command);
    if(command.startsWith("/"))
    {
        showHelp();
    }
    else if(command.equalsIgnoreCase("quit"))
    {
#ifdef ESP32        
        telnetClient.stop();        
#else
        telnetClient.stopAll();        
#endif

    }
    else if(command.startsWith(":"))
    {
        // String response = rs485.sendMasterCommand(command);
        // send(response); //toTelnetclient
    }    
}

bool WifiTelnetServer::process()
{   
    bool ret=false;

    if(enable)
    {
        manageNewConnections();
        if(telnetClient && telnetClient.connected())
        {        
            //RECEIVE
            if (telnetClient.available())
            {
                _lastTimeCommand = millis();
                
                String r_cmd = telnetClient.readStringUntil('\r');
                r_cmd.trim();
                if(r_cmd.length()>0)
                {
                    // echo to debugstream            
                    dbgstream->println("Telnet IN <- " + r_cmd);
                    
                    lastCommandReceived = r_cmd;
                    handleInputCommand(r_cmd);
                    ret = true;
                }
                
            }
                
            // Inactivit - close connection if not received commands from user in telnet
            // For reduce overheads
            if ((millis() - _lastTimeCommand) > MAX_TIME_INACTIVE) {
                String ip = telnetClient.remoteIP().toString();
                dbgstream->println("Telnet Client Stop by inactivity ip: "+ ip);
                telnetClient.println("* Closing session by inactivity");
                telnetClient.stop();          
                
            }
        }
    }

    return ret;
}


void WifiTelnetServer::showHelp()
{
    String help ="";
    help.concat("******************************************************\r\n");  
    help.concat("* ESP8266 GATEWAY Telnet Server ");     
    String info = Config::getDeviceInfoString("\r\n");
    help.concat(info);
    help.concat("******************************************************\r\n");  
    telnetClient.println(help);

}
