
#include <Arduino.h>
#include <ModbusMaster.h>
#include <functional>

#include "modbusTcpGatewayModule.h"

/* ---------------------------------------
* Modbus-TCP 
*    \/
*    \/
*    \/
* Modbus-RTU (remote terminal unit)
* --------------------------------------- */

static ModbusMaster node;

static std::function<void()> pfn_idle=NULL;
static std::function<void()> pfn_pre=NULL;
static std::function<void()> pfn_post=NULL;

static void idle() {
    if(pfn_idle)
    {
        pfn_idle();
    }
}

static void preTransmit() {
    if(pfn_pre)
    {
        pfn_pre();
    }
}

static void postTransmit() {
    if(pfn_post)
    {
        pfn_post();
    }
}
// ---------------------------------------


    
ModbusTCPGatewayModule::ModbusTCPGatewayModule()
: Rs485ServiceModule("ModbusServiceModule", "seriale usata per protocollo modbus")
{

}

ModbusTCPGatewayModule::~ModbusTCPGatewayModule()
{
    this->shutdown();
}


_Error ModbusTCPGatewayModule::setup() 
{
    const JsonObject &root = this->theApp->getConfig().getJsonObject("modbus_gtw");
    if(root.success()) 
    {
        return this->setup(root);
    }
    return _Disable;
}

_Error ModbusTCPGatewayModule::setup(const JsonObject &root)
{
    _Error err = this->Rs485ServiceModule::setup(root);
    if ( err.errorCode != _NoError.errorCode) {
        return err;
    }
    
    // config  
    const unsigned int _tcp_port = root["tcp_port"];
    const unsigned int _slave_id= root["default_slave_id"]; //16
    
    this->theApp->getLogger().printf(F("\t%s Modbus config: tcp_port: %d, default_slave_id: %d,\n"), 
        this->getTitle().c_str(), _tcp_port, _slave_id );
    
    if(_tcp_port) this->tcp_port = _tcp_port;
    if(_slave_id) this->default_slave_id = _slave_id;

    // node.begin(_slave_id, * this->Rs485ServiceModule::getSerialAsStream());

    ::pfn_idle = std::bind(&Rs485ServiceModule::idle, this);    
    node.idle( ::idle );
    ::pfn_pre = std::bind(&Rs485ServiceModule::preTransmit, this);
    node.preTransmission( ::preTransmit );
    ::pfn_post = std::bind(&Rs485ServiceModule::postTransmit, this);
    node.postTransmission( ::postTransmit );

    this->theApp->getLogger().printf(F("\t%s TCP Modbus Gateway setup done\n"), 
        this->getTitle().c_str());
    
    return _NoError;
}

void ModbusTCPGatewayModule::shutdown()
{
    node.clearTransmitBuffer();    
    node.clearResponseBuffer();
}


void ModbusTCPGatewayModule::loop() {
    
}
