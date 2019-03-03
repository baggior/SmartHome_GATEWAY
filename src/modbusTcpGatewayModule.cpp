
#include <Arduino.h>
// #include <ModbusMaster.h>
#include <functional>

#include "modbusTcpGatewayModule.h"
#include "internal/modbusTcpSlave.h"

#define RTU_BUFFER_SIZE  264
#define RTU_RESPONSE_END_TIMEOUT_MILLIS  200

#define TEST_REPLY_RTU false

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

static uint16_t guessResponseFrameDataSize(uint8_t* buffer, uint16_t len);

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
        p_tcpSlave = new ModbusTcpSlave(this->theApp->getLogger(), this->tcp_port, this->theApp->isDebug() );

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

        yield();
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
                default:
                case 0: // Sending a packet to RTU
                {
                    ModbusTcpSlave::smbFrame* p_rtu_frame = p_tcpSlave->getReadyToSendRtuBuffer();
                    if(p_rtu_frame)
                    {
                        ModbusTcpSlave::smbFrame* pmbFrame = p_rtu_frame;
                        uint16_t crcFrame = baseutils::CRC16(pmbFrame->buffer + TCP_MBAP_SIZE, (pmbFrame->len) - TCP_MBAP_SIZE );

                        *((pmbFrame->buffer) + (pmbFrame->len) +1) = (uint8_t) (crcFrame >> 8);
                        *((pmbFrame->buffer) + (pmbFrame->len) +0) = (uint8_t)  (crcFrame & 0xFF);

                        size_t len = (pmbFrame->len) - TCP_MBAP_SIZE + 2 ; //(pmbFrame->len) + 2;
                        size_t base = TCP_MBAP_SIZE;
                       
                        if(base < len) 
                        {
                            // write to RTU len bytes from buffer
                            uint8_t* buffer = (uint8_t*) (pmbFrame->buffer + base);                            
                            size_t written = this->p_rs485->write( buffer, len );
                                                        
                            if(this->theApp->isDebug()) {
                                String hex = baseutils::byteToHexString(buffer, len);
                                this->theApp->getLogger().printf(F("%s: Send pack to RTU. CRC [%X], Len RTU pak: [%d]: \n\t%s\n"), 
                                    this->getTitle().c_str(),
                                    crcFrame, len,
                                    hex.c_str());
                            }
                            
                            if(written != len) {
                                this->theApp->getLogger().printf(F("ERROR sending packet to RTU . written=%d, RTU pak len=%d:\n"), 
                                    written, len);
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
                    
                    break;
                }

                case 1: // Awaiting response
                {
                    ModbusTcpSlave::smbFrame* p_rtu_frame = p_tcpSlave->getWaitFromRtuBuffer();
                    if (p_rtu_frame)
                    {
                        ModbusTcpSlave::smbFrame* pmbFrame = p_rtu_frame;

                        if (pSerial->available())
                        {
                            if(this->theApp->isDebug()) {
                                this->theApp->getLogger().printf(F("\treceived respose from RTU for TCP client: %d\n"), pmbFrame->nClient);
                            }

                            pmbFrame->millis = millis();
                            pmbFrame->len = TCP_MBAP_SIZE;
                            status = 2;
                        }

                        // TEST
                        else if (TEST_REPLY_RTU)
                        {
                            uint8_t* buffer = (uint8_t*) (pmbFrame->buffer + TCP_MBAP_SIZE );
                            buffer[0] = 01;
                            buffer[1] = 01;
                            buffer[2] = 02;
                            buffer[3] = 00; buffer[4] = 07; 
                            //buffer[5] = 0xF8; buffer[6] = 0x3E; //CRC
                        
                            pmbFrame->millis = millis();
                            pmbFrame->len = TCP_MBAP_SIZE + 5;

                            *(pmbFrame->buffer + 5) = (uint8_t) pmbFrame->len - TCP_MBAP_SIZE; // set MBAP response size                            
                            pmbFrame->status = ModbusTcpSlave::frameStatus::readyToSendTcp;
                            status = 0;

                            String hex = baseutils::byteToHexString(buffer, 5);

                            this->theApp->getLogger().printf(F("\n\tTEST: simulate data received from RTU... sending to client TCP cli: %d len=%d \n\t%s\n"), 
                                pmbFrame->nClient, pmbFrame->len,
                                hex.c_str());
                        }              

                        yield();          
                    }
                    else
                        status = 0;
                    
                    break;
                }                

                case 2: // reading the answer
                {
                    ModbusTcpSlave::smbFrame* p_rtu_frame = p_tcpSlave->getWaitFromRtuBuffer();
                    if (p_rtu_frame)
                    {
                        ModbusTcpSlave::smbFrame* pmbFrame = p_rtu_frame;

                        if (pSerial->available())
                        {
                            if(this->theApp->isDebug()) {
                                this->theApp->getLogger().printf(F("\treading respose from RTU for TCP client: %d (so far: %d bytes)\n"), 
                                    pmbFrame->nClient, pmbFrame->len - TCP_MBAP_SIZE);
                            }
                            
                            // Reading the RTU answer                            
                            while(pSerial->available())
                            {
                                if (pmbFrame->len <= RTU_BUFFER_SIZE)
                                {
                                    // READ one byte
                                    pmbFrame->buffer[pmbFrame->len] = pSerial->read();

                                    pmbFrame->len ++;
                                }
                                else {
                                    this->theApp->getLogger().printf(F("\tERROR: response RTU buffer is FULL for TCP client: %d\n"), pmbFrame->nClient);
                                }
                            }

                            // try to read response data size in bytes
                            if(p_rtu_frame->guessedReponseLen == 0 ) {
                                pmbFrame->guessedReponseLen = guessResponseFrameDataSize(pmbFrame->buffer, pmbFrame->len);

                                if( pmbFrame->guessedReponseLen>0 && this->theApp->isDebug()) {
                                    this->theApp->getLogger().printf(F("\tguessed respose RTU length for TCP client: %d is: %d bytes\n"), 
                                        pmbFrame->nClient, pmbFrame->guessedReponseLen);
                                }
                            }

                            // tick last read time
                            pmbFrame->millis = millis();                            
                        }

                        else if( ( (p_rtu_frame->guessedReponseLen > 0) && (p_rtu_frame->len - TCP_MBAP_SIZE >= p_rtu_frame->guessedReponseLen) )
                            || ( millis() - pmbFrame->millis > RTU_RESPONSE_END_TIMEOUT_MILLIS ) ) 
                        {

                            // RTU response received (!!! is supposed completed now !!!)

                            if(this->theApp->isDebug()) {
                                uint8_t* buffer = (uint8_t*) (pmbFrame->buffer + TCP_MBAP_SIZE );
                                size_t cnt = pmbFrame->len - TCP_MBAP_SIZE;
                                String hex = baseutils::byteToHexString( buffer, cnt );
                                this->theApp->getLogger().printf(F("%s: has read from RTU %d bytes, (guessed are %d): \n\t%s\n"), 
                                    this->getTitle().c_str(),
                                    cnt, pmbFrame->guessedReponseLen,
                                    hex.c_str());
                            }
                            
                            uint16_t crcFrame = baseutils::CRC16(pmbFrame->buffer + TCP_MBAP_SIZE, (pmbFrame->len) - TCP_MBAP_SIZE );
                            if (!crcFrame)
                            {
                                // response CRC check OK => send TCP response
                                pmbFrame->len -= 2; //remove crc from response
                                *(pmbFrame->buffer + 5) = (uint8_t) pmbFrame->len - TCP_MBAP_SIZE; // set MBAP response size
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

                        yield();
                    }
                    else
                        status = 0;
                    
                    break;
                }
            }
        }
    }

}



uint16_t guessResponseFrameDataSize(uint8_t* buffer, uint16_t len) {
    uint8_t fncode_index = TCP_MBAP_SIZE + 1 ;
    uint8_t count_index = fncode_index + 1 ;
    if( len > count_index )
    {
        uint8_t fncode = buffer[fncode_index];
        uint8_t count = buffer[count_index];

        if (fncode == 1 || fncode == 2 || fncode == 3 || fncode == 4) {
            uint16_t ret = 1 + 1 + 1 + count + 2;   // ID , FN , count ,  data , CRC 
            return ret;
        }
        else if (fncode == 5 || fncode == 6) {
            uint16_t ret = 1 + 1 + 4 + 2;       // ID , FN , addrval , CRC
            return ret;
        }
        else if (fncode == 7) {
            uint16_t ret = 1 + 1 + 1 + 2;       // ID , FN , val , CRC
            return ret;
        }
        else if (fncode == 15 || fncode == 16) {
            uint16_t ret = 1 + 1 + 4 + 2;       // ID , FN , addrquant , CRC
            return ret;
        }

        else if (fncode >= 0x80) {              //exception response
            uint16_t ret = 1 + 1 + 1 + 2;       // ID , FN , excode , CRC
            return ret;
        }

    }

    return 0;

}