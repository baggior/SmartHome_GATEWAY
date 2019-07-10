#include "coreapi.h"


#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
//#include <ESP8266LLMNR.h>
#elif defined (ESP32)
#include <WiFi.h>
#include <ESPmDNS.h>
#endif


#define THING_SERVICE_PREFIX               "Thing_"
#define THING_DISCOVERY_PROTO               "tcp"
#define THING_DEFAULT_HOSTNAME              "Thing_*"


String _DiscoveryServices::getHostname() {
    #ifdef ESP8266
    return WiFi.hostname();
    #elif defined ESP32
    return WiFi.getHostname();
    #endif
}

bool _DiscoveryServices::setHostname(const char * cs_hostname) {     
    if (cs_hostname==NULL) {
        cs_hostname = THING_DEFAULT_HOSTNAME;
    }

    String hostname (cs_hostname);
    String chipId = baseutils::getChipId();
    hostname.replace("*", chipId);
    hostname.toLowerCase();
    
    #ifdef ESP8266
    bool ret = WiFi.hostname(hostname.c_str());
    #elif defined ESP32
    bool ret = WiFi.setHostname(hostname.c_str());
    #endif  
    
    if(!ret) DPRINTF("Error setting Wifi hostname: %s \n", hostname.c_str());    
    return ret;
}

void _DiscoveryServices::mdnsStopTheDevice() 
{
    MDNS.end();
    String hostname = _DiscoveryServices::getHostname();   
    this->theApp.getLogger().info( "\tMDNS responder stopped. hostname: %s \n", hostname.c_str());
}


bool _DiscoveryServices::mdnsAnnounceTheDevice(bool enableArduino, bool enableWorkstation)
{
    String hostname = _DiscoveryServices::getHostname();   

    if ( !MDNS.begin( hostname.c_str() ) ) {
        this->theApp.getLogger().error( "Error setting up MDNS responder! hostname: %s \n", hostname.c_str() );
        return false;
    }

    String instancename = "Thing mac: [" + WiFi.macAddress() + "]";
    MDNS.setInstanceName(instancename);

    IPAddress ip = WiFi.localIP();
    this->theApp.getLogger().info(("\t>MDNS responder started. hostname: %s (ip: %s), instancename: %s \n"),
        hostname.c_str(), ip.toString().c_str(), instancename.c_str());
   
    if (enableArduino)
        MDNS.enableArduino();

    if (enableWorkstation)
        MDNS.enableWorkstation();
    

    return true;
}

bool _DiscoveryServices::mdnsAnnounceService(unsigned int server_port, const String serviceName, const etl::list<MdnsAttribute, MAX_MDNS_ATTRIBUTES>& attributes)
{
    if(server_port)
    { 
        // Announce esp tcp service:
        MDNS.addService(THING_SERVICE_PREFIX + serviceName, THING_DISCOVERY_PROTO, server_port);  
        this->theApp.getLogger().info(("\tMDNS announced service: %s, proto: %s, port: %d \n"), 
            serviceName.c_str(), THING_DISCOVERY_PROTO, server_port);

        // Add service attributes
        for(MdnsAttribute attr: attributes)
        {            
            this->theApp.getLogger().debug(("\tMDNS service %s, added TXT attribute: %s -> %s\n"), 
                serviceName.c_str(), attr.name.c_str(), attr.value.c_str());            
            MDNS.addServiceTxt(THING_SERVICE_PREFIX + serviceName, THING_DISCOVERY_PROTO, attr.name, attr.value);
        }

        return true;
    }
    else {
        this->theApp.getLogger().error(("\t>Error MDNS announce service [%s]: port is not specified\n"), serviceName.c_str());
    }

    return false;
}

// return only the first result
_DiscoveryServices::MdnsQueryResult _DiscoveryServices::mdnsQuery(String service, String proto)
{
    _DiscoveryServices::MdnsQueryResult ret;
    ret.port=0;   
   
    this->theApp.getLogger().debug( ("\tMDNS query for service _%s._%s ...\n"), service.c_str(), proto.c_str());

    int n = MDNS.queryService(service, proto); // Send out query for esp tcp services
    if (n == 0) {
        this->theApp.getLogger().debug( ("\tno services found\n"));
    } else {
        this->theApp.getLogger().debug( (" \t%d services found\n"), n);
        
        ret.host = MDNS.hostname(0);
        ret.port = MDNS.port(0);
        ret.ip = MDNS.IP(0);

        if(this->theApp.isDebug()) {
            for (int i = 0; i < n; ++i) {
                // Print details for each service found
                this->theApp.getLogger().debug( ("\t%d: %s (%s:%d)\n"),
                    (i+1), MDNS.hostname(i).c_str(), MDNS.IP(i).toString().c_str(), MDNS.port(i));
            }
        }
        
    }

    return ret;
}

void _DiscoveryServices::printDiagWifi()
{    
    if( this->theApp.isDebug() )
    {
        _ApplicationLogger::Loglevel_t previous = this->theApp.getLogger().setLogLevel(_ApplicationLogger::Loglevel_t::DebugLevel);

        this->theApp.getLogger().info(("---- WiFI Diag ----\n"));        
        WiFi.printDiag( (Print&) this->theApp.getLogger() );        
        this->theApp.getLogger().info(("-------------------\n"));

        this->theApp.getLogger().setLogLevel(previous);
    }
}