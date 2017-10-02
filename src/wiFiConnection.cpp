#include "config.h"
#include "wifiConnection.h"
#include <WifiManager.h>


void wifiManagerOpenConnection(Stream& serial)
{
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
            wifiManager.setConnectTimeout(15);          //cerca di stabilire una connessione in 15 secondi
            wifiManager.setConfigPortalTimeout(300);    //il portale dura per 5 minuti poi fa reset
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

