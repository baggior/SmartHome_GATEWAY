#include "config.h"

#include <ESP8266WiFi.h>
#include "wifiTelnetServer.h"

#define MAX_TIME_INACTIVE_DEFAULT 10*60*1000 //10 minuti (in millis)
#define TELNET_LISTEN_PORT_DEFAULT 23

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
    DPRINTF(">Telnet Server SETUP: enable: %d, port: %d, MAX_TIME_INACTIVE: %d \n", 
        enable, _port, _MAX_TIME_INACTIVE);
    
    if(_port) port=_port;
    if(_MAX_TIME_INACTIVE) MAX_TIME_INACTIVE=_MAX_TIME_INACTIVE;

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
            //telnetClient.flush();  //??
            telnetClient.println(s_msg);

            // echo to debugstream            
            dbgstream->println("Telnet OUT -> " + s_msg);
        }
    }
}

String WifiTelnetServer::process()
{   
    String ret;

    if(enable)
    {
        manageNewConnections();
        if(telnetClient && telnetClient.connected())
        {        
            //RECEIVE
            while (telnetClient.available())
            {
                _lastTimeCommand = millis();
                
                String r_cmd = telnetClient.readString();
                if(r_cmd.length()>0)
                {
                    
                    ret = lastCommandReceived = r_cmd;
                }
                
                // echo to debugstream            
                dbgstream->println("Telnet IN <- " + r_cmd);
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
