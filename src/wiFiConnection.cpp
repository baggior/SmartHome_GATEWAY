#include "config.h"
#include "wifiConnection.h"

#include <ESPAsyncWiFiManager.h>

#define THING_GATEEWAY_DISCOVERY_SERVICE    "gateway"
#define THING_GATEEWAY_DISCOVERY_PROTO      "tcp"

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

    Serial_printf(*dbgstream, F(">WiFI Connection SETUP: SSID: %s, password: %s, hostname: %s, static_ip: %s, static_gw: %s, static_sn: %s, phy_mode: %s, connectionTimeout: %d, statcaptivePortalTimeout: %d, minimumSignalQuality: %d, outputPower: %s \r\n"), 
        REPLACE_NULL_STR(SSID), REPLACE_NULL_STR(password),
        REPLACE_NULL_STR(hostname), REPLACE_NULL_STR(static_ip), REPLACE_NULL_STR(static_gw), REPLACE_NULL_STR(static_sn),
        REPLACE_NULL_STR(_phy_mode),
        connectionTimeout, captivePortalTimeout, minimumSignalQuality, dtostrf(outputPower,3,1, buff) );
       
    // dbgstream->printf(">WiFI Connection SETUP: SSID: %s, password: %s, hostname: %s, static_ip: %s, static_gw: %s, static_sn: %s, phy_mode: %s, connectionTimeout: %d, statcaptivePortalTimeout: %d, minimumSignalQuality: %d, outputPower: %s \r\n", 
    //     REPLACE_NULL_STR(SSID), REPLACE_NULL_STR(password),
    //     REPLACE_NULL_STR(hostname), REPLACE_NULL_STR(static_ip), REPLACE_NULL_STR(static_gw), REPLACE_NULL_STR(static_sn),
    //     REPLACE_NULL_STR(_phy_mode),
    //     connectionTimeout, captivePortalTimeout, minimumSignalQuality, dtostrf(outputPower,3,1, buff) );

    // Connect to WiFi    
    uint8_t status = WiFi.begin();
    if(status!= WL_CONNECTED)
    {                

        if (!SSID)  
        {           
            AsyncWebServer server(80);
            DNSServer dns;
            AsyncWiFiManager wifiManager(&server,&dns);             

            dbgstream->println(F("Start WiFiManager connection.. \r\n"));            
            
            #ifdef MY_DEBUG
            wifiManager.setDebugOutput(true);
            #endif
            if(connectionTimeout) wifiManager.setConnectTimeout(connectionTimeout);                 //cerca di stabilire una connessione in 15 secondi
            if(captivePortalTimeout) wifiManager.setConfigPortalTimeout(captivePortalTimeout);      //il portale dura per 5 minuti poi fa reset
            if(minimumSignalQuality) wifiManager.setMinimumSignalQuality(minimumSignalQuality);
            if(static_ip && static_gw && static_sn) 
            {
                Serial_printf(*dbgstream, F("Use Custom STA IP/GW/Subnet (%s, %s, %s)\r\n"), static_ip, static_gw, static_sn);
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
        else //SSID defined
        {
            Serial_printf(*dbgstream, F("Start WiFi connection custom SSID: %s ..\r\n"), SSID);
            if(static_ip && static_gw && static_sn) 
            {
                Serial_printf(*dbgstream, F("Use Custom STA IP/GW/Subnet (%s, %s, %s)\r\n"), static_ip, static_gw, static_sn);
                IPAddress ip1,ip2,ip3;
                if(ip1.fromString(static_ip) && ip2.fromString(static_gw) && ip3.fromString(static_sn))
                {                    
                    WiFi.config(ip1, ip2, ip3);                
                }
                else
                {
                   Serial_printf(*dbgstream, F("STA Failed to configure: ERROR on reading static ip config params\r\n")); 
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
                restartESP();
            }
        }
         
    }

#ifdef ESP8266
    if(hostname) WiFi.hostname(hostname);
#elif defined ESP32
    if(hostname) WiFi.setHostname(hostname);
#endif  

    String connectedSSID = WiFi.SSID();
    dbgstream->print(F("WiFi connected to SSID: "));    dbgstream->print(connectedSSID);
    dbgstream->print(F(" HOSTNAME: "));    
#ifdef ESP8266
    dbgstream->println(WiFi.hostname());
#elif defined ESP32
    dbgstream->println(WiFi.getHostname());
#endif
    IPAddress connectedIPAddress = WiFi.localIP();  
    dbgstream->print(F(" IP address: "));    dbgstream->println(connectedIPAddress.toString());
    dbgstream->print(F("ESP Mac Address: ")); dbgstream->println(WiFi.macAddress());
    dbgstream->print(F("Subnet Mask: ")); dbgstream->println(WiFi.subnetMask());
    dbgstream->print(F("Gateway IP: "));  dbgstream->println(WiFi.gatewayIP());
    dbgstream->print(F("DNS: ")); dbgstream->println(WiFi.dnsIP());
}


void WiFiConnection::DEBUG_printDiagWiFI()
{
    #ifdef MY_DEBUG
    dbgstream->println(F("WiFI printDiag:"));
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
        dbgstream->println(F("Error setting up MDNS responder! "));
    }
    Serial_printf(*dbgstream, F("mDNS responder started. hostname: %s (ip: %s) \n")
        , hostname.c_str(), ip.toString().c_str());
   
    String proto("_"), service("_");    
    // proto.concat("_"); 
    proto.concat(THING_GATEEWAY_DISCOVERY_PROTO);
    // service.concat("_"); 
    service.concat(THING_GATEEWAY_DISCOVERY_SERVICE);

    const int port = 9; //TODO take from REST server config
     // Announce esp tcp service on port 80
    MDNS.addService(service, proto, port);
    MDNS.enableWorkstation();
    MDNS.enableArduino(9);
//TEST    
    // MDNS.addServiceTxt(service, proto, "serviceAttr", "theservice");
    // MDNS.addServiceTxt(service, proto, "typeAttr", "thetype");

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
    
    Serial_printf(*dbgstream, F("mDNS query for service _%s._%s.local. ...\n"), service.c_str(), proto.c_str());

    int n = MDNS.queryService(service, proto); // Send out query for esp tcp services
    if (n == 0) {
        dbgstream->println(F("\tno services found"));
    } else {
        Serial_printf(*dbgstream, F(" \t%d services found\n"), n);
        
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