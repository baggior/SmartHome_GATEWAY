#include "config.h"
#include "wifiConnection.h"
#include <WifiManager.h>


void wifiManagerOpenConnection(Stream& serial)
{
    JsonObject & root = config.getJsonRoot();    
    const char* SSID = root["wifi"]["SSID"]; //TODO
    const char* password = root["wifi"]["password"]; //TODO
    
    const char* hostname = root["wifi"]["hostname"];
    const char* static_ip = root["wifi"]["static_ip"];
    const char* static_gw = root["wifi"]["static_gw"];
    const char* static_sn = root["wifi"]["static_sn"];
    DPRINTF("hostname: %s, static_ip: %s, static_gw: %s, static_sn: %s \n", 
        hostname, static_ip, static_gw,static_sn);

    int connectionTimeout = root["wifi"]["connectionTimeout"];
    int captivePortalTimeout = root["wifi"]["captivePortalTimeout"];
    int minimumSignalQuality = root["wifi"]["minimumSignalQuality"];
    float outputPower = root["wifi"]["outputPower"];
    char buff[20];
    DPRINTF("connectionTimeout: %d, statcaptivePortalTimeout: %d, minimumSignalQuality: %d, outputPower: %s \n", 
        connectionTimeout, captivePortalTimeout, minimumSignalQuality, dtostrf(outputPower,3,1, buff) );
       
    // Connect to WiFi
    uint8_t status =WiFi.status();
    if(status!= WL_CONNECTED)
    {        

        #ifdef MY_DEBUG
        serial.println("WiFI printDiag:");
        WiFi.printDiag(DEBUG_OUTPUT);
        #endif
        
        //WiFi.mode(WIFI_STA);       
        {           
            serial.println("WiFiManager connection.. ");
            
            WiFiManager wifiManager;        
            
            #ifdef MY_DEBUG
            wifiManager.setDebugOutput(true);
            #endif
            if(connectionTimeout) wifiManager.setConnectTimeout(connectionTimeout);                 //cerca di stabilire una connessione in 15 secondi
            if(captivePortalTimeout) wifiManager.setConfigPortalTimeout(captivePortalTimeout);      //il portale dura per 5 minuti poi fa reset
            if(minimumSignalQuality) wifiManager.setMinimumSignalQuality(minimumSignalQuality);
            if(static_ip) 
            {
                IPAddress ip1,ip2,ip3;                 
                if(ip1.fromString(static_ip))
                {
                    wifiManager.setSTAStaticIPConfig(ip1, ip2, ip3);
                }
            }
            if(hostname) WiFi.hostname(hostname);
            if(outputPower) WiFi.setOutputPower(outputPower);    //max: +20.5dBm  min: 0dBm
            
            WiFi.setAutoConnect(true);
            WiFi.setAutoReconnect(true);
            
            bool connected= wifiManager.autoConnect();  //use this for auto generated name ESP + ChipID   
            if(!connected)
            {
                // Reset
                delay(3000);        
                ESP.reset();        
                delay(5000);
            }
            
        }
         
    } 
    
    String connectedSSID = WiFi.SSID();
    IPAddress connectedIPAddress = WiFi.localIP();  
    serial.print("WiFi connected to SSID: ");    serial.print(connectedSSID);
    serial.print(" IP: ");    serial.println(connectedIPAddress.toString());

    #ifdef MY_DEBUG
    serial.println("WiFI printDiag:");
    WiFi.printDiag(DEBUG_OUTPUT);
    #endif

}

