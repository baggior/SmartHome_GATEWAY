#include "config.h"
#include "wifiConnection.h"
#include <WifiManager.h>

#define THING_GATEEWAY_DISCOVERY_ID "gateway"

WiFiPhyMode parsePhyModeParamString(const char * _phy_mode_param)
{
    WiFiPhyMode ret=WIFI_PHY_MODE_11N;
    if(_phy_mode_param)
    {
        String phy_mode = _phy_mode_param;
        if(phy_mode.length()>0)
        {
            if(phy_mode.equals("B"))
            {
                return WiFiPhyMode::WIFI_PHY_MODE_11B;
            }
            else if(phy_mode.equals("G"))
            {
                return WiFiPhyMode::WIFI_PHY_MODE_11G;
            }
            else if(phy_mode.equals("N"))
            {
                return WiFiPhyMode::WIFI_PHY_MODE_11N;
            }
        }
    }
    return ret;
}


void WiFiConnection::wifiManagerOpenConnection()
{
    JsonObject & root = config.getJsonRoot();    
    const char* SSID = root["wifi"]["SSID"]; //TODO
    const char* password = root["wifi"]["password"]; //TODO
    
    const char* hostname = root["wifi"]["hostname"];
    const char* static_ip = root["wifi"]["static_ip"];
    const char* static_gw = root["wifi"]["static_gw"];
    const char* static_sn = root["wifi"]["static_sn"];

    const char* _phy_mode = root["wifi"]["phy_mode"];
    
    int connectionTimeout = root["wifi"]["connectionTimeout"];
    int captivePortalTimeout = root["wifi"]["captivePortalTimeout"];
    int minimumSignalQuality = root["wifi"]["minimumSignalQuality"];
    float outputPower = root["wifi"]["outputPower"];
    char buff[20];

    dbgstream->printf(">WiFI Connection SETUP: hostname: %s, static_ip: %s, static_gw: %s, static_sn: %s, "
        "phy_mode: %s, connectionTimeout: %d, statcaptivePortalTimeout: %d, minimumSignalQuality: %d, outputPower: %s \n", 
        hostname, static_ip, static_gw,static_sn,
        _phy_mode,
        connectionTimeout, captivePortalTimeout, minimumSignalQuality, dtostrf(outputPower,3,1, buff) );
       
    // Connect to WiFi
    uint8_t status =WiFi.status();
    if(status!= WL_CONNECTED)
    {                     
        if(hostname) WiFi.hostname(hostname);
        if (!SSID)  
        {           
            WiFiManager wifiManager;             

            dbgstream->println("Start WiFiManager connection.. ");            
            
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
                    ip2.fromString(static_gw); ip3.fromString(static_sn);
                    wifiManager.setSTAStaticIPConfig(ip1, ip2, ip3);
                }
            }
            
            if(outputPower) WiFi.setOutputPower(outputPower);    //max: +20.5dBm  min: 0dBm
            if(_phy_mode) WiFi.setPhyMode(parsePhyModeParamString(_phy_mode));           
            
            bool connected= wifiManager.autoConnect();  //use this for auto generated name ESP + ChipID   
            if(!connected)
            {
                // Reset
                restartESP();
            }
            else
            {
               WiFi.mode(WIFI_STA); 
               WiFi.setAutoConnect(true);
               WiFi.setAutoReconnect(true);            
            }            
        }
        else
        {
            dbgstream->println("Custom STA IP/GW/Subnet");
            IPAddress ip1,ip2,ip3;
            if(ip1.fromString(static_ip))
            {
                ip2.fromString(static_gw); ip3.fromString(static_sn);
                WiFi.config(ip1, ip2, ip3);                
            }

            WiFi.begin(SSID, password);
            uint8_t status= WiFi.waitForConnectResult();

            if(status == WL_CONNECTED)
            {
                WiFi.mode(WIFI_STA); 
                WiFi.setAutoConnect(true);
                WiFi.setAutoReconnect(true);
            }
            else
            {
                // Reset
                restartESP();
            }
        }
         
    }     

    String connectedSSID = WiFi.SSID();
    IPAddress connectedIPAddress = WiFi.localIP();  
    dbgstream->print("WiFi connected to SSID: ");    dbgstream->print(connectedSSID);
    dbgstream->print(" HOSTNAME: ");    dbgstream->println(WiFi.hostname());
    dbgstream->print(" IP: ");    dbgstream->println(connectedIPAddress.toString());

}


void WiFiConnection::DEBUG_printDiagWiFI()
{
    #ifdef MY_DEBUG
    dbgstream->println("WiFI printDiag:");
    WiFi.printDiag(DEBUG_OUTPUT);
    #endif
}

void WiFiConnection::announceTheDevice()
{
    String hostname = WiFi.hostname();
    IPAddress ip = WiFi.localIP();
    uint32_t chipId = ESP.getChipId();

    if (!MDNS.begin(hostname.c_str())) {
        dbgstream->println("Error setting up MDNS responder! ");
    }
    dbgstream->printf("mDNS responder started: %s.local (ip: %s) \n"
        , hostname.c_str(), ip.toString().c_str());
   
    //TODO rest port
    const char* proto="tcp";
    const int port =80;
    MDNS.addService(THING_GATEEWAY_DISCOVERY_ID, proto, port); // Announce esp tcp service on port 80
    dbgstream->printf("mDNS service: %s, proto:%s, port: %d \n"
        , THING_GATEEWAY_DISCOVERY_ID, proto, port);
}


void WiFiConnection::setup(Stream &dbgstream)
{
    this->dbgstream = &dbgstream;

    wifiManagerOpenConnection();
    #ifdef MY_DEBUG
    DEBUG_printDiagWiFI();
    #endif

}

void WiFiConnection::process()
{
    
}


void WiFiConnection::query(String service)
{
    dbgstream->println("Sending mDNS query");
    //"thing:server"
    int n = MDNS.queryService(service, "tcp"); // Send out query for esp tcp services
    dbgstream->println("mDNS query done");
    if (n == 0) {
        dbgstream->println("no services found");
    }
}


void WiFiConnection::restartESP()
{
    delay(3000);        
    dbgstream->flush();    
    baseutils::ESP_restart();      //soft resetart
    delay(5000);
}