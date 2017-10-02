#include "config.h"

#include <ESP8266WiFi.h>
#include "wifiTelnetServer.h"

#define MAX_TIME_INACTIVE 60000 //1 minuto
#define TELNET_LISTEN_PORT 23

WifiTelnetServer::WifiTelnetServer()
    : server(TELNET_LISTEN_PORT), dbgstream(NULL)
{
}

void WifiTelnetServer::setup(Stream &serial)
{
    dbgstream = &serial;
    server.begin();
    server.setNoDelay(true);

    // Print the IP address
    serial.print("Server telnet started on IP:");
    serial.print(WiFi.localIP());
    serial.print(" port: ");
    serial.println(TELNET_LISTEN_PORT);
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


bool WifiTelnetServer::process(const String& s_msg)
{
    bool ret=false;

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
                lastCommandReceived = r_cmd;
                ret = true;
            }
            
            // echo to debugstream            
            dbgstream->println("Telnet IN <- " + r_cmd);
        }

        //SEND
        if(s_msg.length()>0)
        {
            //telnetClient.flush();  //??
            telnetClient.println(s_msg);

            // echo to debugstream            
            dbgstream->println("Telnet OUT -> " + s_msg);
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

    delay(10);

    return ret;
}


void WifiTelnetServer::showHelp()
{
    String help ="";
    help.concat("******************************************************\r\n");  
    help.concat("**ESP8266 GATEWAY Telnet Server "); 
    #ifdef SW_VERSION
        help.concat("- version: "); help.concat(SW_VERSION); 
    #endif
    #ifdef MY_DEBUG
        help.concat(" DEBUG ON "); 
        #ifdef DEBUG_OUTPUT
            help.concat(" Port: "+ String(VALUE_TO_STRING(DEBUG_OUTPUT))); 
        #endif
    #endif
    help.concat("\r\n");
    help.concat("ESP8266 Chip ID: " + String(ESP.getChipId()) +"\r\n");
    help.concat("* IP:");  help.concat(WiFi.localIP().toString());  help.concat(" Mac address:"); help.concat(WiFi.macAddress()); help.concat("\r\n");
    help.concat("* Free Heap RAM: "); help.concat(ESP.getFreeHeap()); help.concat("\r\n");
    help.concat("******************************************************\r\n");  
    telnetClient.println(help);

}
