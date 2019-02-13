
#include <Arduino.h>
// #include <ModbusMaster.h>
#include <functional>

#include "modbusTcpGatewayModule.h"
#include "internal/modbusTcpSlave.h"

#define RTU_BUFFER_SIZE  264
#define RTU_RESPONSE_END_MILLIS 2

/* ---------------------------------------
* Modbus-TCP 
*    \/
*    \/
*    \/
* Modbus-RTU (remote terminal unit)
* --------------------------------------- */

// static ModbusMaster node;

// static std::function<void()> pfn_idle=NULL;
// static std::function<void()> pfn_pre=NULL;
// static std::function<void()> pfn_post=NULL;

// static void idle() {
//     if(pfn_idle)
//     {
//         pfn_idle();
//     }
// }

// static void preTransmit() {
//     if(pfn_pre)
//     {
//         pfn_pre();
//     }
// }

// static void postTransmit() {
//     if(pfn_post)
//     {
//         pfn_post();
//     }
// }
// ---------------------------------------

static ModbusTcpSlave * p_tcpSlave = NULL;

// ---------------------------------------

    
ModbusTCPGatewayModule::ModbusTCPGatewayModule()
: _BaseModule("ModbusTCPGatewayModule", "gateway Modbus TCP <-> RTU", true)
{ 
    
}

ModbusTCPGatewayModule::~ModbusTCPGatewayModule()
{
    this->shutdown();
}


_Error ModbusTCPGatewayModule::setup() 
{
    const JsonObject &root = this->theApp->getConfig().getJsonObject("modbus_tcp_gtw");
    if(root.success()) 
    {
        return this->setup(root);
    }
    return _Disable;
}

_Error ModbusTCPGatewayModule::setup(const JsonObject &root)
{
    this->p_rs485 = this->theApp->getModule<Rs485ServiceModule>("_Rs485Service"); 
    if(!this->p_rs485)
    {
        return _Error(2, "ModbusTCPGatewayModule Error: servizio _Rs485Service non esistente");            
    }
    if(!this->p_rs485->isEnabled())
    {
        return _Error(3, "ModbusTCPGatewayModule Error: servizio _Rs485Service esistente ma disabilitato");    
    }

    bool on = root["enable"] | false;
    
    // config  
    const unsigned int _tcp_port = root["tcp_port"];
    
    this->theApp->getLogger().printf(F("\t%s Modbus TCP gateway config: tcp_port: %d \n"), 
        this->getTitle().c_str(), _tcp_port);
    
    if(_tcp_port) 
        this->tcp_port = _tcp_port;

    if (on)
    {
        // -----------------------
        //TCP SLAVE SETUP:
        p_tcpSlave = new ModbusTcpSlave(this->theApp->getLogger(), this->tcp_port );

        // -----------------------
        //MODBUS MASTER SETUP:

        // ::pfn_idle = std::bind(&Rs485ServiceModule::idle, this);    
        // node.idle( ::idle );
        // ::pfn_pre = std::bind(&Rs485ServiceModule::preTransmit, this);
        // node.preTransmission( ::preTransmit );
        // ::pfn_post = std::bind(&Rs485ServiceModule::postTransmit, this);
        // node.postTransmission( ::postTransmit );
        
        // node.begin(_slave_id);

        // -----------------------        
    }
    else 
    {
        return _Disable;
    }
    
    return _NoError;
   
}

void ModbusTCPGatewayModule::shutdown()
{
    if(p_tcpSlave) {
        delete p_tcpSlave;
        p_tcpSlave = NULL;
    }

    // node.clearTransmitBuffer();    
    // node.clearResponseBuffer();
    if(p_rs485) {
        p_rs485 = NULL;
    }
}


void ModbusTCPGatewayModule::loop() {
    if(p_tcpSlave) 
    {
        p_tcpSlave->timeoutBufferCleanup();
        // yield();
        p_tcpSlave->waitNewClient();
        // yield();  
        p_tcpSlave->readDataClient();
        // yield();
        this->rtuTransactionTask(); 
        // yield();
        p_tcpSlave->writeFrameClient();        
    }
}


void ModbusTCPGatewayModule::rtuTransactionTask() 
{   
    if(this->p_rs485) 
    {
        Stream* pSerial = this->p_rs485->getSerialAsStream();
        if(pSerial) 
        {
            switch(status) 
            {
                case 0: // Sending a packet to RTU
                {
                    ModbusTcpSlave::smbFrame* p_rtu_frame = p_tcpSlave->getReadyToSendRtuBuffer();
                    if(p_rtu_frame)
                    {
                        ModbusTcpSlave::smbFrame* pmbFrame = p_rtu_frame;
                        uint16_t crcFrame = baseutils::CRC16(pmbFrame->buffer + 6, (pmbFrame->len) - 6 );

                        *((pmbFrame->buffer) + (pmbFrame->len) +1) = (uint8_t) (crcFrame >> 8);
                        *((pmbFrame->buffer) + (pmbFrame->len) +0) = (uint8_t)  (crcFrame & 0xFF);

                        size_t len = (pmbFrame->len) - 4; //(pmbFrame->len) + 2;
                        size_t count = 6;
                        
                        this->theApp->getLogger().printf(F("\tSend pack to RTU. CRC [%X], Len RTU pak: [%d]\n"), 
                            crcFrame, len);

                        if(count<len) 
                        {
                            uint8_t* buffer = (uint8_t*) (pmbFrame->buffer + count);                            
                            this->p_rs485->write( buffer, len );
                            
                            if(this->theApp->isDebug()) {
                                String hex = baseutils::byteToHexString(buffer, len);
                                this->theApp->getLogger().printf(F("%s: sent to RTU: \n\t%s\n"), 
                                    this->getTitle().c_str(),
                                    hex.c_str());
                            }
                        }

                        if(pmbFrame->status == ModbusTcpSlave::frameStatus::readyToSendRtuNoReply)
                        {
                            pmbFrame->status = ModbusTcpSlave::frameStatus::empty;
                            status = 0;
                        } 
                        else 
                        {
                            pmbFrame->status = ModbusTcpSlave::frameStatus::waitFromRtu;
                            pmbFrame->millis = millis();
                            status = 1;
                        }
                    }
                    // break;
                }

                case 1: // Awaiting response
                {
                    ModbusTcpSlave::smbFrame* p_rtu_frame = p_tcpSlave->getWaitFromRtuBuffer();
                    if (p_rtu_frame)
                    {
                        ModbusTcpSlave::smbFrame* pmbFrame = p_rtu_frame;

                        if(this->theApp->isDebug()) {
                            this->theApp->getLogger().printf(F("\tawaiting respose from RTU for TCP client: %d\n"), pmbFrame->nClient);
                        }

                        if (pSerial->available())
                        {
                            pmbFrame->millis = millis();
                            pmbFrame->len = 6;
                            status = 2;
                        }
                        // TEST
                        else
                        {
                            uint8_t* buffer = (uint8_t*) (pmbFrame->buffer + 6 );
                            buffer[0] = 01;
                            buffer[1] = 01;
                            buffer[2] = 04;
                            buffer[3] = 00; buffer[4] = 00; buffer[5] = 00; buffer[6] = 03;
                            buffer[7] = 0xBB; buffer[8] = 0xD0;
                        
                            pmbFrame->millis = millis();
                            pmbFrame->len = 6 - 2 + 9;
                            *(pmbFrame->buffer + 5) = (uint8_t) pmbFrame->len-6; 
                            pmbFrame->status = ModbusTcpSlave::frameStatus::readyToSendTcp;
                            status = 0;

                            this->theApp->getLogger().printf(F("\tTEST data received from RTU to client TCP: %d len:%d \n"), pmbFrame->nClient, pmbFrame->len);
                        }                        
                    }
                    else
                        status = 0;
                    // break;
                }                

                case 2: // reading the answer
                {
                    ModbusTcpSlave::smbFrame* p_rtu_frame = p_tcpSlave->getWaitFromRtuBuffer();
                    if (p_rtu_frame)
                    {
                        ModbusTcpSlave::smbFrame* pmbFrame = p_rtu_frame;

                        if(this->theApp->isDebug()) {
                            this->theApp->getLogger().printf(F("\treading respose from RTU for TCP client: %d\n"), pmbFrame->nClient);
                        }
                        
                        if (pSerial->available())
                        {
                            // Reading the RTU answer
                            pmbFrame->millis = millis();
                            while(pSerial->available())
                            {
                                if (pmbFrame->len <= RTU_BUFFER_SIZE)
                                {
                                    *(pmbFrame->buffer + pmbFrame->len) =  pSerial->read();
                                    pmbFrame->len ++;
                                }
                            }
                        }

                        else if (millis() - pmbFrame->millis > RTU_RESPONSE_END_MILLIS) 
                        {
                            // RTU response received (completed)

                            if(this->theApp->isDebug()) {
                                uint8_t* buffer = (uint8_t*) (pmbFrame->buffer + 6 );
                                String hex = baseutils::byteToHexString(buffer, (pmbFrame->len - 6));
                                this->theApp->getLogger().printf(F("%s: read from RTU: \n\t%s\n"), 
                                    this->getTitle().c_str(),
                                    hex.c_str());
                            }
                            
                            uint16_t crcFrame = baseutils::CRC16(pmbFrame->buffer + 6, (pmbFrame->len) - 6 );
                            if (!crcFrame)
                            {
                                // response CRC check OK => send TCP response
                                pmbFrame->len -= 2; //remove CRC
                                *(pmbFrame->buffer + 5) = (uint8_t) pmbFrame->len-6;  //set response length
                                pmbFrame->status = ModbusTcpSlave::frameStatus::readyToSendTcp;
                            }
                            else
                            {
                                // response CRC check error
                                this->theApp->getLogger().printf(F("\tCRC check ERROR in packet received from RTU -> Del pack.\n"));

                                pmbFrame->status = ModbusTcpSlave::frameStatus::empty;
                                while(pSerial->available()) 
                                    pSerial->read();
                            }
                            status = 0;  
                        }
                    }
                    else
                        status = 0;
                    // break;
                }
            }
        }
    }

}