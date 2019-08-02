
#include <Arduino.h>
// #include <ModbusMaster.h>
#include <functional>


#include "modbusTcpGatewayModule.h"
#include "internal/modbusTcpSlave.h"
#define RTU_BUFFER_SIZE                     264
#define RTU_RESPONSE_END_TIMEOUT_MILLIS     200

#define TEST_REPLY_RTU                      false

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

static size_t guessRtuResponseFrameDataSize(uint8_t* buffer, size_t len);

static String convertRtuToAsci ( uint8_t* rtubuffer, size_t len);
static size_t convertAsciiToRtu ( uint8_t* rtubuffer, const String& inputAsciiBuffer);

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
    if(!root.isNull()) 
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
    this->modbus_ascii = root["to_modbus_ascii"] | false;
    
    // config  
    const unsigned int _tcp_port = root["tcp_port"];
    
    this->theApp->getLogger().info(("\t%s Modbus TCP gateway config: enabled: %d, tcp_port: %d, convert to_modbus_ascii: %d \n"), 
        this->getTitle().c_str(), on, _tcp_port, this->modbus_ascii );
    
    if(_tcp_port) 
        this->tcp_port = _tcp_port;

    if (on)
    {        
        // -----------------------
        // MODBUS TCP SLAVE SETUP:
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
        // mdns announce MODBUS TCP service
        this->theApp->getNetServices().mdnsAnnounceService(this->tcp_port, this->getTitle());
    }
    else 
    {
        return _Disable;
    }
    
    return _NoError;   
}

void ModbusTCPGatewayModule::shutdown()
{
    this->theApp->getLogger().info(("%s: Module shutdown..\n"), this->getTitle().c_str());
    
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
        this->serialTransactionTask(); 
        // yield();
        p_tcpSlave->writeFrameClient();        

        yield();
    }
}

void ModbusTCPGatewayModule::serialTransactionTask() 
{   
    if(this->p_rs485) 
    {
        Stream* pSerial = this->p_rs485->getSerialAsStream();
        if(pSerial) 
        {
            switch(this->status) 
            {
                default:
                case 0: // Sending a packet to serial
                {
                    ModbusTcpSlave::smbFrame* p_rtu_frame = p_tcpSlave->getReadyToSendRtuBuffer();
                    if(p_rtu_frame)
                    {
                        ModbusTcpSlave::smbFrame* pmbFrame = p_rtu_frame;
                        if(this->modbus_ascii)
                        {
                            //ASCII
                            size_t rtu_len = (pmbFrame->len) - TCP_MBAP_SIZE; 
                            size_t rtu_base = TCP_MBAP_SIZE;
                            uint8_t* rtubuffer = (uint8_t*) (pmbFrame->buffer + rtu_base);  

                            // uint8_t asciibuffer[ASCII_BUFFER_SIZE];
                            String ascii_string_pdu = convertRtuToAsci(rtubuffer, rtu_len);
                            if(ascii_string_pdu.length()>0)
                            {
                                // LRC
                                unsigned char lrc = baseutils::calculateLRC( ascii_string_pdu.c_str(), ascii_string_pdu.length() );

                                // ASCII PACKET
                                String ascii_string = ":" + ascii_string_pdu + lrc + "\r\n"; // CR=13 LF=10

                                // write to ASCII len bytes from asciibuffer
                                size_t written = this->p_rs485->write( ascii_string );

                                if(this->theApp->isDebug()) {                                    
                                    this->theApp->getLogger().debug(("%s: Send pack to ASCII serial. Len ASCII pak: [%d]: \n\t%s\n"), 
                                        this->getTitle().c_str(),
                                        written,
                                        ascii_string.c_str());
                                }

                                if(written != ascii_string.length()) {
                                    this->theApp->getLogger().error(("ERROR sending packet to ASCII serial . written=%d, ASCII pak len=%d:\n"), 
                                        written, ascii_string.length());
                                }
                            }
                        }
                        else
                        {
                            //RTU
                            uint16_t crcFrame = baseutils::CRC16(pmbFrame->buffer + TCP_MBAP_SIZE, (pmbFrame->len) - TCP_MBAP_SIZE );

                            *((pmbFrame->buffer) + (pmbFrame->len) +1) = (uint8_t) (crcFrame >> 8);
                            *((pmbFrame->buffer) + (pmbFrame->len) +0) = (uint8_t)  (crcFrame & 0xFF);

                            size_t len = (pmbFrame->len) - TCP_MBAP_SIZE + 2 ; 
                            size_t base = TCP_MBAP_SIZE;
                        
                            if(base < len) 
                            {
                                // write to RTU len bytes from buffer
                                uint8_t* buffer = (uint8_t*) (pmbFrame->buffer + base);                            
                                size_t written = this->p_rs485->write( buffer, len );
                                                            
                                if(this->theApp->isDebug()) {
                                    String hex = baseutils::byteToHexString(buffer, len);
                                    this->theApp->getLogger().debug(("%s: Send pack to RTU. CRC [%X], Len RTU pak: [%d]: \n\t%s\n"), 
                                        this->getTitle().c_str(),
                                        crcFrame, written,
                                        hex.c_str());
                                }
                                
                                if(written != len) {
                                    this->theApp->getLogger().error(("ERROR sending packet to RTU . written=%d, RTU pak len=%d:\n"), 
                                        written, len);
                                }
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
                                this->theApp->getLogger().debug(("\trespose is ready from Serial to TCP client: %d\n"), pmbFrame->nClient);
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

                            this->theApp->getLogger().debug(("\n\tTEST: simulate data received from RTU... sending to client TCP cli: %d len=%d \n\t%s\n"), 
                                pmbFrame->nClient, pmbFrame->len,
                                hex.c_str());
                        }              

                        yield();          
                    }
                    else
                    {
                        status = 0;
                    }
                    
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
                                this->theApp->getLogger().debug(("\treading respose from Serial for TCP client: %d (so far: %d bytes)\n"), 
                                    pmbFrame->nClient, pmbFrame->len - TCP_MBAP_SIZE);
                            }
                            
                            if(this->modbus_ascii)
                            {
                                // Reading the ASCII answer      
                                while(pSerial->available())
                                {
                                    char c = pSerial->read();
                                    pmbFrame->ascii_response_buffer += c;
                                }                                
                            }
                            else
                            {
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
                                            this->theApp->getLogger().error(("\tERROR: response RTU buffer is FULL for TCP client: %d\n"), pmbFrame->nClient);
                                        }
                                }

                                // try to read response data size in bytes
                                if(p_rtu_frame->guessedReponseLen == 0 ) {
                                    pmbFrame->guessedReponseLen = guessRtuResponseFrameDataSize(pmbFrame->buffer, pmbFrame->len);

                                    if( pmbFrame->guessedReponseLen>0 && this->theApp->isDebug()) {
                                        this->theApp->getLogger().debug(("\tguessed respose RTU length for TCP client: %d is: %d bytes\n"), 
                                            pmbFrame->nClient, pmbFrame->guessedReponseLen);
                                    }
                                }
                            }


                            // tick last read time
                            pmbFrame->millis = millis();                            
                        }
                        
                        else if (this->modbus_ascii)
                        {
                            // ASCII wait for cr/LF
                            if(pmbFrame->ascii_response_buffer.endsWith("\r\n"))
                            {
                                // ASCII response received (!!! is supposed completed now !!!)
                                // remove cr lf
                                pmbFrame->ascii_response_buffer.remove(pmbFrame->ascii_response_buffer.length() - 2, 2);

                                if(this->theApp->isDebug()) {
                                    this->theApp->getLogger().debug(("%s: has read from ASCII %d chars: \n\t%s\n"), 
                                        this->getTitle().c_str(),
                                        pmbFrame->ascii_response_buffer.length(),
                                        pmbFrame->ascii_response_buffer.c_str());
                                }

                                unsigned char lrc = baseutils::calculateLRC(pmbFrame->ascii_response_buffer.c_str(), pmbFrame->ascii_response_buffer.length() );
                                if (!lrc)
                                {
                                    // response LRC check OK => convert ASCII to RTU and send TCP response
                                    
                                    // remove LRC
                                    pmbFrame->ascii_response_buffer.remove(pmbFrame->ascii_response_buffer.length() - 1, 1);

                                    uint8_t* buffer = pmbFrame->buffer + TCP_MBAP_SIZE;
                                    size_t rtu_len = convertAsciiToRtu(buffer, pmbFrame->ascii_response_buffer);
                                    
                                    // prepare to send TCP response
                                    pmbFrame->len  = rtu_len + TCP_MBAP_SIZE;
                                    *(pmbFrame->buffer + 5) = rtu_len; // set MBAP response size
                                    pmbFrame->status = ModbusTcpSlave::frameStatus::readyToSendTcp;
                                }
                                else
                                {
                                    // response LRC check error
                                    this->theApp->getLogger().error(("\tLRC check ERROR in packet received from ASCII serial -> Del pack.\n"));
                                    pmbFrame->status = ModbusTcpSlave::frameStatus::empty;
                                    while(pSerial->available()) 
                                        pSerial->read();
                                }                                
                            }

                            else if ( millis() - pmbFrame->millis > RTU_RESPONSE_END_TIMEOUT_MILLIS )
                            {
                                //ASCII RESPONSE TIMEOUT
                                this->theApp->getLogger().error(("\tASCII serial response timeout waiting for <Cr><LF> -> Del pack.\n"));
                                pmbFrame->status = ModbusTcpSlave::frameStatus::empty;
                                while(pSerial->available()) 
                                    pSerial->read();
                            }

                            status = 0;                            
                        }
                        
                        else 
                        {
                            // RTU wait for size bytes or timeout
                            if( ( (p_rtu_frame->guessedReponseLen > 0) && (p_rtu_frame->len - TCP_MBAP_SIZE >= p_rtu_frame->guessedReponseLen) )
                                || ( millis() - pmbFrame->millis > RTU_RESPONSE_END_TIMEOUT_MILLIS ) ) 
                            {

                                // RTU response received (!!! is supposed completed now !!!)

                                if(this->theApp->isDebug()) {
                                    uint8_t* buffer = (uint8_t*) (pmbFrame->buffer + TCP_MBAP_SIZE );
                                    size_t cnt = pmbFrame->len - TCP_MBAP_SIZE;
                                    String hex = baseutils::byteToHexString( buffer, cnt );
                                    this->theApp->getLogger().debug(("%s: has read from RTU %d bytes, (guessed are %d): \n\t%s\n"), 
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
                                    this->theApp->getLogger().error(("\tCRC check ERROR in packet received from RTU -> Del pack.\n"));

                                    pmbFrame->status = ModbusTcpSlave::frameStatus::empty;
                                    while(pSerial->available()) 
                                        pSerial->read();
                                }

                                status = 0;  
                            }
                        }


                        yield();
                    }
                    
                    else
                    {
                        status = 0;
                    }
                    
                    break;
                }
            }
        }
    }

}



size_t guessRtuResponseFrameDataSize(uint8_t* buffer, size_t len) {
    uint8_t fncode_index = TCP_MBAP_SIZE + 1 ;
    uint8_t count_index = fncode_index + 1 ;
    if( len > count_index )
    {
        uint8_t fncode = buffer[fncode_index];
        uint8_t count = buffer[count_index];

        if (fncode == 1 || fncode == 2 || fncode == 3 || fncode == 4) {
            size_t ret = 1 + 1 + 1 + count + 2;   // ID , FN , count ,  data , CRC 
            return ret;
        }
        else if (fncode == 5 || fncode == 6) {
            size_t ret = 1 + 1 + 4 + 2;       // ID , FN , addrval , CRC
            return ret;
        }
        else if (fncode == 7) {
            size_t ret = 1 + 1 + 1 + 2;       // ID , FN , val , CRC
            return ret;
        }
        else if (fncode == 15 || fncode == 16) {
            size_t ret = 1 + 1 + 4 + 2;       // ID , FN , addrquant , CRC
            return ret;
        }

        else if (fncode >= 0x80) {              //exception response
            size_t ret = 1 + 1 + 1 + 2;       // ID , FN , excode , CRC
            return ret;
        }

    }

    return 0;
}



String convertRtuToAsci ( uint8_t* rtubuffer, size_t len)
{
    String ret = ":"; // start ASCI MESSAGE delimiting character
    if (rtubuffer && len>0) 
    {
        for (size_t i=0; i<len; ++i)
        {
            unsigned char high_nibble = ( unsigned char ) rtubuffer[i] >>4;
            unsigned char high_char = baseutils::binNibble2Char(high_nibble);

            unsigned char low_nibble = ( unsigned char ) rtubuffer[i]  & 0x0F;
            unsigned char low_char = baseutils::binNibble2Char(low_nibble);

            ret += high_char + low_char;
        }
    }
    return ret;
}

size_t convertAsciiToRtu ( uint8_t* rtubuffer, const String& inputAsciiBuffer)
{
    int len = inputAsciiBuffer.length();
    if (rtubuffer && len>0)
    {
        for (int i=0; i<len; ++i)
        {
            unsigned char c = inputAsciiBuffer[i];
            unsigned char bin = baseutils::char2Binary( c );
            
            rtubuffer[i] = bin;
        }

        return len;
    }
    return 0;
}

