#include "config.h"
#include "wifiConnection.h"

#include <ESPAsyncWiFiManager.h>

#define THING_GATEEWAY_DISCOVERY_SERVICE    "_gateway"
#define THING_GATEEWAY_DISCOVERY_PROTO      "_tcp"

#ifdef ESP8266
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
#endif

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
        REPLACE_NULL_STR(hostname), REPLACE_NULL_STR(static_ip), REPLACE_NULL_STR(static_gw), REPLACE_NULL_STR(static_sn),
        REPLACE_NULL_STR(_phy_mode),
        connectionTimeout, captivePortalTimeout, minimumSignalQuality, dtostrf(outputPower,3,1, buff) );
       
    // Connect to WiFi    
    uint8_t status = WiFi.begin();
    if(status!= WL_CONNECTED)
    {                
#ifdef ESP8266
        if(hostname) WiFi.hostname(hostname);
#elif defined ESP32
        if(hostname) WiFi.setHostname(hostname);
#endif
        if (!SSID)  
        {           
            AsyncWebServer server(80);
            DNSServer dns;
            AsyncWiFiManager wifiManager(&server,&dns);             

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

#ifdef ESP8266            
            if(outputPower) WiFi.setOutputPower(outputPower);    //max: +20.5dBm  min: 0dBm
            if(_phy_mode) WiFi.setPhyMode(parsePhyModeParamString(_phy_mode));           
#endif
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
    dbgstream->print(" HOSTNAME: ");    
#ifdef ESP8266
    dbgstream->println(WiFi.hostname());
#elif defined ESP32
    dbgstream->println(WiFi.getHostname());
#endif
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
#ifdef ESP8266
    String hostname = WiFi.hostname();
    uint32_t chipId = ESP.getChipId();
#elif defined ESP32
    String hostname = WiFi.getHostname();
    uint32_t chipId = (uint32_t)ESP.getEfuseMac();
#endif
    IPAddress ip = WiFi.localIP();

    hostname.toLowerCase();
    if (!MDNS.begin(hostname.c_str())) {
        dbgstream->println("Error setting up MDNS responder! ");
    }
    dbgstream->printf("mDNS responder started. hostname: %s (ip: %s) \n"
        , hostname.c_str(), ip.toString().c_str());
   

    const char* proto = THING_GATEEWAY_DISCOVERY_PROTO;
    const int port = 80; //TODO take from REST server config
     // Announce esp tcp service on port 80
    MDNS.addService(THING_GATEEWAY_DISCOVERY_SERVICE, THING_GATEEWAY_DISCOVERY_PROTO, port);
    MDNS.addServiceTxt(THING_GATEEWAY_DISCOVERY_SERVICE, THING_GATEEWAY_DISCOVERY_PROTO, "service", "theservice");
    MDNS.addServiceTxt(THING_GATEEWAY_DISCOVERY_SERVICE, THING_GATEEWAY_DISCOVERY_PROTO, "type", "thetype");
//    MDNS.addServiceTxt(THING_GATEEWAY_DISCOVERY_SERVICE, THING_GATEEWAY_DISCOVERY_PROTO, "id", "theid");
//TEST
    MDNS.addService("_gateway-telnet", THING_GATEEWAY_DISCOVERY_PROTO, 21);

    dbgstream->printf("mDNS service: %s, proto: %s, port: %d \n"
        , THING_GATEEWAY_DISCOVERY_SERVICE, THING_GATEEWAY_DISCOVERY_PROTO, port);
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

QueryResult WiFiConnection::query()
{
    return this->query(THING_GATEEWAY_DISCOVERY_SERVICE, THING_GATEEWAY_DISCOVERY_PROTO);
}
QueryResult WiFiConnection::query(String service, String proto)
{
    QueryResult ret( {.port=0} );
    
    dbgstream->printf("mDNS query for service _%s._%s.local. ...\n", service.c_str(), proto.c_str());

    int n = MDNS.queryService(service, proto); // Send out query for esp tcp services
    if (n == 0) {
        dbgstream->println("\tno services found");
    } else {
        dbgstream->printf(" \t%d services found\n", n);
        
        ret.host = MDNS.hostname(0);
        ret.port = MDNS.port(0);
        ret.ip = MDNS.IP(0);

        #ifdef MY_DEBUG
        for (int i = 0; i < n; ++i) {
            // Print details for each service found
            dbgstream->printf("\t%d: %s (%s:%d)\n",
                (i+1), MDNS.hostname(i).c_str(), MDNS.IP(i).toString().c_str(), MDNS.port(i));
        }
        #endif
    }

    return ret;
}


void WiFiConnection::restartESP()
{
    delay(3000);        
    dbgstream->flush();    
    baseutils::ESP_restart();      //soft resetart
    delay(5000);
}