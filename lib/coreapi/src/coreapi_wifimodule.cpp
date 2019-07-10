#include "coreapi.h"


#ifdef ESP8266
#include <ESP8266WiFi.h>
#elif defined (ESP32)
#include <WiFi.h>
#endif

#include <WiFiManager.h>



_Error _WifiConnectionModule::setup()
{
    this->theApp->getLogger().debug(("_WifiConnectionModule::setup()\n"));

    _Error err = this->wifiManagerOpenConnection();
    if(err==_NoError)
    {
        if(this->theApp->isDebug()) 
        {            
            // setup remote log
            this->theApp->getLogger().setupRemoteLog( _DiscoveryServices::getHostname() );
            
            this->theApp->getNetServices().printDiagWifi( );        
        }

        bool ret = this->theApp->getNetServices().mdnsAnnounceTheDevice(true);        
        if(!ret) {
            err = _Error(-12,"MDNS announce error") ;
        }

    }

    return err;   
}

void _WifiConnectionModule::shutdown()
{
    this->theApp->getLogger().info(("%s: ConnectionModule shutdown..\n"), this->getTitle().c_str());
    
    this->theApp->getNetServices().mdnsStopTheDevice();
    
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
    this->theApp->getLogger().debug(("\twifiManagerOpenConnection()\n"));

     // configuration    
    const JsonObject& root = this->theApp->getConfig().getJsonObject("wifi");
    if(root.isNull()) 
        return _Error(-1, "Error parsing wifi config");
    
    const char* SSID = root["SSID"]; // TODO: parse
    const char* password = root["password"]; // TODO: parse
    
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

    this->theApp->getLogger().info ((">WiFI Connection SETUP: SSID: %s, password: %s, hostname: %s, static_ip: %s, static_gw: %s, static_sn: %s, static_dns1: %s, phy_mode: %s, connectionTimeout: %d, statcaptivePortalTimeout: %d, minimumSignalQuality: %d, outputPower: %s \r\n"), 
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
    this->theApp->getLogger().debug("\t>WiFi.begin(): status %d\n", status); //    WL_CONNECTED        = 3,    WL_CONNECT_FAILED   = 4,
    if(status!= WL_CONNECTED)
    {               
        if (!SSID)  
        {           
            // AsyncWebServer server(80);
            DNSServer dns;            
            // AsyncWiFiManager wifiManager(&server,&dns);             
            WiFiManager wifiManager(DEBUG_OUTPUT);      

            this->theApp->getLogger().info (("Start WiFiManager connection.. \r\n") );            
            
            wifiManager.setDebugOutput(this->theApp->isDebug());            

            if(connectionTimeout) wifiManager.setConnectTimeout(connectionTimeout);                 //cerca di stabilire una connessione in 15 secondi
            if(captivePortalTimeout) wifiManager.setConfigPortalTimeout(captivePortalTimeout);      //il portale dura per 5 minuti poi fa reset
            if(minimumSignalQuality) wifiManager.setMinimumSignalQuality(minimumSignalQuality);
            if(static_ip && static_gw && static_sn && static_dns1) 
            {
                this->theApp->getLogger().info (("Use Custom STA IP/GW/Subnet (%s, %s, %s, %s)\r\n"), static_ip, static_gw, static_sn, static_dns1);
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
                _Error ret = _Error(-10, "wifiManager auto connection error => restart.");
                // Reset
                this->theApp->restart(ret);      
                return ret;           
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
            this->theApp->getLogger().debug (("Start WiFi connection custom SSID: %s ..\r\n"), SSID);
            if(static_ip && static_gw && static_sn && static_dns1) 
            {
                this->theApp->getLogger().info (("Use Custom STA IP/GW/Subnet (%s, %s, %s, %s)\r\n"), static_ip, static_gw, static_sn, static_dns1);
                IPAddress ip1,ip2,ip3, dns1;
                if(ip1.fromString(static_ip) && ip2.fromString(static_gw) && ip3.fromString(static_sn) && dns1.fromString(static_dns1))
                {                    
                    WiFi.config(ip1, ip2, ip3, dns1);                
                }
                else
                {
                   this->theApp->getLogger().warn (("STA Failed to configure: ERROR on reading static ip config params\r\n")); 
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
                _Error ret = _Error(-10, "connection error to SSID: "+String(SSID)+" => restart.");
                // Reset
                this->theApp->restart(ret);     
                return ret;
            }
        }         
    }

    //CONNECTED OK
    // DPRINTLN("CONNECTED OK");

    //set device host name
    _DiscoveryServices::setHostname(hostname);

    String connectedSSID = WiFi.SSID();
    this->theApp->getLogger().info (("WiFi connected to SSID: %s "), connectedSSID.c_str());
    this->theApp->getLogger().info ((" HOSTNAME: %s"),  _DiscoveryServices::getHostname().c_str());

    IPAddress connectedIPAddress = WiFi.localIP();  
    this->theApp->getLogger().info ((" IP address: %s"), connectedIPAddress.toString().c_str());
    this->theApp->getLogger().debug ((" ESP Mac Address: %s"), WiFi.macAddress().c_str());
    this->theApp->getLogger().debug ((" Subnet Mask: %s"), WiFi.subnetMask().toString().c_str());
    this->theApp->getLogger().debug ((" Gateway IP: %s"), WiFi.gatewayIP().toString().c_str());
    this->theApp->getLogger().debug ((" DNS: %s .\n"), WiFi.dnsIP().toString().c_str());
    
    return _NoError; 
}


