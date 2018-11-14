#include "coreapi.h"


#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
//#include <ESP8266LLMNR.h>
#elif defined (ESP32)
#include <WiFi.h>
#include <ESPmDNS.h>
#endif

#include <ESPAsyncWiFiManager.h>



_Error _WifiConnectionModule::setup()
{
    DPRINTLN(F("_WifiConnectionModule::setup()"));

    _Error err = this->wifiManagerOpenConnection();
    if(err==_NoError)
    {
        bool ret = this->theApp->getNetServices().announceTheDevice();        
        if(!ret) {
            err = _Error(-12,"MDNS announce error") ;
        }

        if(this->theApp->isDebug())
        {
            this->theApp->getNetServices().printDiagWifi(this->theApp->getLogger().getStream());        
        }
    }

    return err;   
}

void _WifiConnectionModule::shutdown()
{
    MDNS.end();
    WiFi.disconnect();
}

void _WifiConnectionModule::loop()
{
#ifndef ESP32
  // mDNS happens asynchronously on ESP32
  MDNS.update();
#endif
}

void _WifiConnectionModule::beforeModuleAdded()
{
    //remove core WiFi module if exists   
    this->theApp->removeModule(  this->theApp->getBaseModule( ENUM_TO_STR(_CoreWifiConnectionModule) ) );
}


///////////////////////////////////////////////////////////////////////////////////////////


#ifdef ESP8266
static WiFiPhyMode parsePhyModeParamString(const char * _phy_mode_param)
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
#endif

_Error _WifiConnectionModule::wifiManagerOpenConnection()
{
    DPRINTLN(F("\twifiManagerOpenConnection()"));

     // configuration    
    const JsonObject& root = this->theApp->getConfig().getJsonObject("wifi");
    if(!root.success()) return _Error(-1, "Error parsing wifi config");
    
    const char* SSID = root["SSID"]; //TODO
    const char* password = root["password"]; //TODO
    
    const char* hostname = root["hostname"];
    const char* static_ip = root["static_ip"];
    const char* static_gw = root["static_gw"];
    const char* static_sn = root["static_sn"];
    const char* static_dns1 = root["static_dns1"];

    const char* _phy_mode = root["phy_mode"];
    
    int connectionTimeout = root["connectionTimeout"];
    int captivePortalTimeout = root["captivePortalTimeout"];
    int minimumSignalQuality = root["minimumSignalQuality"];
    float outputPower = root["outputPower"];
    char buff[20];

    this->theApp->getLogger().printf( F(">WiFI Connection SETUP: SSID: %s, password: %s, hostname: %s, static_ip: %s, static_gw: %s, static_sn: %s, static_dns1: %s, phy_mode: %s, connectionTimeout: %d, statcaptivePortalTimeout: %d, minimumSignalQuality: %d, outputPower: %s \r\n"), 
        REPLACE_NULL_STR(SSID), REPLACE_NULL_STR(password),
        REPLACE_NULL_STR(hostname), 
        REPLACE_NULL_STR(static_ip), REPLACE_NULL_STR(static_gw), REPLACE_NULL_STR(static_sn), REPLACE_NULL_STR(static_dns1),
        REPLACE_NULL_STR(_phy_mode),
        connectionTimeout, captivePortalTimeout, minimumSignalQuality, dtostrf(outputPower,3,1, buff) );
       
    // dbgstream->printf(">WiFI Connection SETUP: SSID: %s, password: %s, hostname: %s, static_ip: %s, static_gw: %s, static_sn: %s, phy_mode: %s, connectionTimeout: %d, statcaptivePortalTimeout: %d, minimumSignalQuality: %d, outputPower: %s \r\n", 
    //     REPLACE_NULL_STR(SSID), REPLACE_NULL_STR(password),
    //     REPLACE_NULL_STR(hostname), REPLACE_NULL_STR(static_ip), REPLACE_NULL_STR(static_gw), REPLACE_NULL_STR(static_sn),
    //     REPLACE_NULL_STR(_phy_mode),
    //     connectionTimeout, captivePortalTimeout, minimumSignalQuality, dtostrf(outputPower,3,1, buff) );

    // Connect to WiFi    
    
    uint8_t status = WiFi.begin();
    DPRINTF("\t>WiFi.begin(): status %d\n", status); //    WL_CONNECTED        = 3,    WL_CONNECT_FAILED   = 4,
    if(status!= WL_CONNECTED)
    {               
        if (!SSID)  
        {           
            AsyncWebServer server(80);
            DNSServer dns;            
            AsyncWiFiManager wifiManager(&server,&dns);             

            this->theApp->getLogger().printf( F("Start WiFiManager connection.. \r\n") );            
            
            wifiManager.setDebugOutput(this->theApp->isDebug());

            if(connectionTimeout) wifiManager.setConnectTimeout(connectionTimeout);                 //cerca di stabilire una connessione in 15 secondi
            if(captivePortalTimeout) wifiManager.setConfigPortalTimeout(captivePortalTimeout);      //il portale dura per 5 minuti poi fa reset
            if(minimumSignalQuality) wifiManager.setMinimumSignalQuality(minimumSignalQuality);
            if(static_ip && static_gw && static_sn && static_dns1) 
            {
                this->theApp->getLogger().printf( F("Use Custom STA IP/GW/Subnet (%s, %s, %s, %s)\r\n"), static_ip, static_gw, static_sn, static_dns1);
                IPAddress ip1,ip2,ip3, dns1;                 
                if(ip1.fromString(static_ip))
                {
                    ip2.fromString(static_gw); ip3.fromString(static_sn); 
                    dns1.fromString(static_dns1);
                    wifiManager.setSTAStaticIPConfig(ip1, ip2, ip3, dns1);
                }
            }

#ifdef ESP8266            
            if(outputPower) WiFi.setOutputPower(outputPower);    //max: +20.5dBm  min: 0dBm
            if(_phy_mode) WiFi.setPhyMode(parsePhyModeParamString(_phy_mode));           
#endif
            bool connected= wifiManager.autoConnect();  //use this for auto generated name ESP + ChipID   
            if(!connected)
            {
                // Reset
                this->theApp->restart();      
                return _Error(-10, "wifiManager auto connection error => reset.");           
            }
            else
            {
               WiFi.mode(WIFI_STA); 
               WiFi.setAutoConnect(true);
               WiFi.setAutoReconnect(true);            
            }            
        }
        else //SSID defined
        {
            this->theApp->getLogger().printf( F("Start WiFi connection custom SSID: %s ..\r\n"), SSID);
            if(static_ip && static_gw && static_sn && static_dns1) 
            {
                this->theApp->getLogger().printf( F("Use Custom STA IP/GW/Subnet (%s, %s, %s, %s)\r\n"), static_ip, static_gw, static_sn, static_dns1);
                IPAddress ip1,ip2,ip3, dns1;
                if(ip1.fromString(static_ip) && ip2.fromString(static_gw) && ip3.fromString(static_sn) && dns1.fromString(static_dns1))
                {                    
                    WiFi.config(ip1, ip2, ip3, dns1);                
                }
                else
                {
                   this->theApp->getLogger().printf( F("STA Failed to configure: ERROR on reading static ip config params\r\n")); 
                }
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
                this->theApp->restart();     
                return _Error(-10, "connection error to SSID: "+String(SSID)+" => reset."); 
            }
        }         
    }

    //CONNECTED OK
    // DPRINTLN("CONNECTED OK");

    //set device host name
    if(hostname) 
    {
        String str_hostname(hostname);
        String chipId = baseutils::getChipId();
        str_hostname.replace("*", chipId);
        str_hostname.toLowerCase();

        #ifdef ESP8266
        WiFi.hostname(str_hostname.c_str());
        #elif defined ESP32
        WiFi.setHostname(str_hostname.c_str());
        #endif  
    }

    String connectedSSID = WiFi.SSID();
    this->theApp->getLogger().printf( F("WiFi connected to SSID: %s "), connectedSSID.c_str());
    this->theApp->getLogger().printf( F(" HOSTNAME: %s"),  this->theApp->getNetServices().getHostname().c_str());

    IPAddress connectedIPAddress = WiFi.localIP();  
    this->theApp->getLogger().printf( F(" IP address: %s"), connectedIPAddress.toString().c_str());
    this->theApp->getLogger().printf( F(" ESP Mac Address: %s"), WiFi.macAddress().c_str());
    this->theApp->getLogger().printf( F(" Subnet Mask: %s"), WiFi.subnetMask().toString().c_str());
    this->theApp->getLogger().printf( F(" Gateway IP: %s"), WiFi.gatewayIP().toString().c_str());
    this->theApp->getLogger().printf( F(" DNS: %s .\n"), WiFi.dnsIP().toString().c_str());
    
    return _NoError; 
}


