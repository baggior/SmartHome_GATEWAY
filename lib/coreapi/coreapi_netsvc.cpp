#include "coreapi.h"


#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
//#include <ESP8266LLMNR.h>
#elif defined (ESP32)
#include <WiFi.h>
#include <ESPmDNS.h>
#endif


//TODO add to config
#define THING_GATEEWAY_DISCOVERY_SERVICE    "sht_gtw"
#define THING_GATEEWAY_DISCOVERY_PROTO      "tcp"


String _NetServices::getHostname() {
    #ifdef ESP8266
    return WiFi.hostname();
    #elif defined ESP32
    return WiFi.getHostname();
    #endif
}


bool _NetServices::announceTheDevice(unsigned int server_port)
{    
    MdnsAttributeList attributes;    
    return this->announceTheDevice(server_port, attributes);
} 

bool _NetServices::announceTheDevice(unsigned int server_port, const etl::list<MdnsAttribute, MAX_MDNS_ATTRIBUTES>& attributes)
{
    if(server_port)
    { 
        String hostname = _NetServices::getHostname();   
        hostname.toLowerCase();

        IPAddress ip = WiFi.localIP();

        if (!MDNS.begin(hostname.c_str())) {
            this->theApp.getLogger().printf(F("Error setting up MDNS responder! \n"));
            return false;
        }
        this->theApp.getLogger().printf(F("\tMDNS responder started. hostname: %s (ip: %s) \n"),
            hostname.c_str(), ip.toString().c_str());
    
        String proto("_"), service("_");    
        proto.concat(THING_GATEEWAY_DISCOVERY_PROTO);
        service.concat(THING_GATEEWAY_DISCOVERY_SERVICE);

        // Announce esp tcp service on port 80
        MDNS.addService(service, proto, server_port);      

        // Add service attributes
        for(MdnsAttribute attr: attributes)
        {            
            this->theApp.getLogger().printf(F("\tMDNS attribute: %s -> %s\n"), attr.name, attr.value);
            MDNS.addServiceTxt(service, proto, attr.name, attr.value);
        }
  
        this->theApp.getLogger().printf(F(">MDNS announced service: %s, proto: %s, port: %d \n"), 
            THING_GATEEWAY_DISCOVERY_SERVICE, THING_GATEEWAY_DISCOVERY_PROTO, server_port);

        return true;
    }
    else {
        DPRINTF(F("MDNS announce: server_port undefined: %d\n"), server_port);
    }

    return false;
}

_NetServices::MdnsQueryResult _NetServices::mdnsQuery(String service, String proto)
{
    _NetServices::MdnsQueryResult ret;
    ret.port=0;
    
    this->theApp.getLogger().printf( F("MDNS query for service _%s._%s.local. ...\n"), service.c_str(), proto.c_str());

    int n = MDNS.queryService(service, proto); // Send out query for esp tcp services
    if (n == 0) {
        this->theApp.getLogger().printf( F("\tno services found"));
    } else {
        this->theApp.getLogger().printf( F(" \t%d services found\n"), n);
        
        ret.host = MDNS.hostname(0);
        ret.port = MDNS.port(0);
        ret.ip = MDNS.IP(0);

        if(this->theApp.isDebug()) {
            for (int i = 0; i < n; ++i) {
                // Print details for each service found
                this->theApp.getLogger().printf( F("\t%d: %s (%s:%d)\n"),
                    (i+1), MDNS.hostname(i).c_str(), MDNS.IP(i).toString().c_str(), MDNS.port(i));
            }
        }
        
    }

    return ret;
}

void _NetServices::printDiagWifi(Stream* dbgstream)
{    
    if(dbgstream)
    {
        dbgstream->println(F("---- WiFI Diag ----"));
        WiFi.printDiag(*dbgstream);        
        dbgstream->println(F("-------------------"));
    }
}