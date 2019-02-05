
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
        p_tcpSlave->task();
        yield();
        this->rtuTransactionTask(); 
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

                        size_t len = (pmbFrame->len) + 2;
                        size_t count = 6;
                        
                        this->theApp->getLogger().printf(F("\tSend pack to RTU. CRC [%X], Len RTU pak: [%d]\n"), 
                            crcFrame, (pmbFrame->len) - 4);

                        if(count<len) 
                        {
                            this->p_rs485->write( (uint8_t*) (pmbFrame->buffer + count), len );
                            // this->preTransmit();

                            // while (count < len) {
                            //     pSerial->write((uint8_t)*(pmbFrame->buffer + (count++)));
                            // }

                            // this->postTransmit();
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
                    break;
                }

                case 1: // Awaiting response & reading the RTU answer
                {
                    ModbusTcpSlave::smbFrame* p_rtu_frame = p_tcpSlave->getWaitFromRtuBuffer();
                    if (p_rtu_frame)
                    {
                        ModbusTcpSlave::smbFrame* pmbFrame = p_rtu_frame;
                        // if (pSerial->available())
                        // {
                        //     pmbFrame->millis = millis();
                        //     pmbFrame->len = 6;
                        //     status = 2;
                        // }
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
                            uint16_t crcFrame = baseutils::CRC16(pmbFrame->buffer + 6, (pmbFrame->len) - 6 );
                            if (!crcFrame)
                            {
                                // response CRC check OK => send TCP response
                                pmbFrame->len -= 2;
                                *(pmbFrame->buffer + 5) = (uint8_t) pmbFrame->len-6;
                                pmbFrame->status = ModbusTcpSlave::frameStatus::readyToSendTcp;
                            }
                            else
                            {
                                // response CRC check error
                                status = 0;
                                pmbFrame->status = ModbusTcpSlave::frameStatus::empty;
                                while(pSerial->available()) 
                                    pSerial->read();
                            }
                        }
                    }
                    else
                        status = 0;
                    break;
                }

                // case 2: // reading the answer
                // {
                //     ModbusTcpSlave::smbFrame* p_rtu_frame = p_tcpSlave->getWaitFromRtuBuffer();
                //     if (p_rtu_frame)
                //     {
                //         ModbusTcpSlave::smbFrame* pmbFrame = p_rtu_frame;
                //         if (pSerial->available())
                //         {
                //             pmbFrame->millis = millis();
                //             while(pSerial->available())
                //             {
                //                 if (pmbFrame->len <= RTU_BUFFER_SIZE)
                //                 {
                //                     *(pmbFrame->buffer + pmbFrame->len) =  pSerial->read();
                //                     pmbFrame->len ++;
                //                 }
                //             }
                //         }
                //         else if (millis() - pmbFrame->millis > 2)
                //         {
                //             uint16_t crcFrame = baseutils::CRC16(pmbFrame->buffer + 6, (pmbFrame->len) - 6 );
                //             if (!crcFrame)
                //             {
                //                 pmbFrame->len -= 2;
                //                 *(pmbFrame->buffer + 5) = (uint8_t) pmbFrame->len-6;
                //                 pmbFrame->status = ModbusTcpSlave::frameStatus::readyToSendTcp;
                //             }
                //             else
                //             {
                //                 status = 0;
                //                 pmbFrame->status = ModbusTcpSlave::frameStatus::empty;
                //                 while(pSerial->available()) 
                //                     pSerial->read();
                //             }
                //         }
                //     }
                //     else
                //         status = 0;
                //     break;
                // }
            }
        }
    }

}