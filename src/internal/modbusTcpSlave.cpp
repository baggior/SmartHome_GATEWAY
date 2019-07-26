#include "Arduino.h"

#include "modbusTcpSlave.h"

// WiFiServer mbServer(MODBUSIP_PORT);

#define TCP_TIMEOUT_MS RTU_TIMEOUT * 2

ModbusTcpSlave::ModbusTcpSlave(_ApplicationLogger& logger, uint16_t port = MODBUSIP_PORT, bool _isDebug = false)
: mbServer(port), mLogger(logger), isDebug(_isDebug)
{
  mbServer.begin();
  mbServer.setNoDelay(true);

#ifdef ESP32
  mbServer.setTimeout(TCP_TIMEOUT_MS / 1000);
#endif

  for (uint8_t i = 0 ; i < FRAME_COUNT; i++)
    mbFrame[i].status = frameStatus::empty;

  for (uint8_t i = 0 ; i < CLIENT_NUM; i++) 
    clientOnLine[i].onLine = false;
}

ModbusTcpSlave::~ModbusTcpSlave()
{    
}

void ModbusTcpSlave::waitNewClient(void)
{
   // see if the old customers are alive if not alive then release them
   for (uint8_t i = 0 ; i < CLIENT_NUM; i++)
   {
      //find free/disconnected spot
      if (clientOnLine[i].onLine && !clientOnLine[i].client.connected())
      {
        clientOnLine[i].client.stop(); // 
        clientOnLine[i].onLine = false;
        this->mLogger.debug (("\tClient stopped: [%d]\n"), i);
      }
      // else clientOnLine[i].client.flush();
   }

   if (mbServer.hasClient())
   {
     bool clientAdded = false;
     for(uint8_t i = 0 ; i < CLIENT_NUM; i++)
     {
       if( !clientOnLine[i].onLine)
       {
          clientOnLine[i].client = mbServer.available();
          clientOnLine[i].client.setNoDelay(true); //disable delay feature algorithm
          clientOnLine[i].onLine = true;
          clientAdded = true;

          if(this->isDebug) {
            this->mLogger.debug (("\tNew Client: [%d] remote Ip: %s\n"), i, clientOnLine[i].client.remoteIP().toString().c_str());
          }
          break;
       }
     }

     if (!clientAdded) // If there was no place for a new client
     {
        //no free/disconnected spot so reject        
        this->mLogger.error (("\tToo many Clients reject the new connection \n") );
        mbServer.available().stop();
        // clientOnLine[0].client.stop();
        // clientOnLine[0].client = mbServer.available();
     }
   }
}

void ModbusTcpSlave::readDataClient(void)
{
  for(uint8_t i = 0; i < CLIENT_NUM; i++)
  {
    if(clientOnLine[i].onLine)
    {
      if(clientOnLine[i].client.available())
      {
        this->readFrameClient(clientOnLine[i].client, i);
      }
    }
  }
}

void ModbusTcpSlave::readFrameClient(WiFiClient client, uint8_t nClient)
{
  size_t available = client.available();
  if ((available < TCP_BUFFER_SIZE) && (available > 11))
  {
    size_t len = available;
    uint8_t buf[len];
    size_t count = 0;
    while(client.available()) {
      buf[count] = client.read(); count++; 
    }

    count =0;
    smbap  mbap;
    mbapUnpack(&mbap, &buf[0]);
    if(this->isDebug) {
      this->mLogger.debug (("\tPaket in : len TCP data [%d] Len mbap pak [%d], UnitId [%d], TI [%d] \n"),
        len, mbap._len, mbap._ui, mbap._ti);
    }

    // checking for glued requests. (wizards are requested for 4 requests)
    while((count < len ) && ((len - count) <= (size_t) (mbap._len + TCP_MBAP_SIZE)) && (mbap._pi ==0))
    {
      smbFrame * pmbFrame = this->getFreeBuffer();
      if(pmbFrame == 0) 
        break; // if there is no free buffer then we reduce the parsing

      pmbFrame->nClient = nClient;
      
      if(mbap._ui==0) {
        // UnitId = 0 => broadcast modbus message
        pmbFrame->status = frameStatus::readyToSendRtuNoReply;
      } else {
        pmbFrame->status = frameStatus::readyToSendRtu;

      }
      pmbFrame->len = mbap._len + TCP_MBAP_SIZE;
      pmbFrame->millis   = millis();

      for (uint16_t j = 0; j < (pmbFrame->len); j++)
        pmbFrame->buffer[j] = buf[j];

      count +=  pmbFrame->len;
      mbapUnpack(&mbap, &buf[count]);
    }
  }
  else
  {
    this->mLogger.error (("\tTCP client [%d] data count invalid : %d\n"), nClient, available);

    // uint16_t tmp = client.available();
    while(client.available()) 
      client.read();
    
  }
}

void ModbusTcpSlave::writeFrameClient(void)
{  
  smbFrame * pmbFrame = this->getReadyToSendTcpBuffer();
  if(pmbFrame)
  {
    uint8_t cli = pmbFrame->nClient;
    size_t len = pmbFrame->len;

    if(! clientOnLine[cli].client.connected() ) {
      this->mLogger.warn (("\tERROR writeFrameClient: writing to a disconnected client: %d"), cli);
      return;
    }

    size_t written = clientOnLine[cli].client.write(&pmbFrame->buffer[0], len);

    // write to TCP client
    if(this->isDebug) {
      this->mLogger.debug (("\twritten data buffer to TCP client: %d, len=%d\n"), cli, len );
    }

    if(written!= len) {
      this->mLogger.error (("\tERROR writeFrameClient: writing buffer [%d] to RTU client len_to_write=%d, written=%d\n"), cli, len, written);
    }
    // delay(1);
    // yield();
    clientOnLine[cli].client.flush();
    pmbFrame->status = frameStatus::empty;
  }
}

// void ModbusTcpSlave::task()
// {
//   waitNewClient();
//   yield();
//   readDataClient();
//   yield();  
//   writeFrameClient();
//   yield();
//   timeoutBufferCleanup();
// }

void ModbusTcpSlave::timeoutBufferCleanup() {
  // Cleaning the buffers
  for(uint8_t i = 0; i < FRAME_COUNT; i++)
  {
    if(mbFrame[i].status != frameStatus::empty ) {
      if (millis() - mbFrame[i].millis > RTU_TIMEOUT)
      {
        mbFrame[i].status = frameStatus::empty;        
        // this->mLogger.printf (("\tRTU_TIMEOUT -> Del pack.\n"));        
        this->mLogger.error (("\tRTU_TIMEOUT -> Del pack.\n"));        
      }
    }
  }
}

ModbusTcpSlave::smbFrame * ModbusTcpSlave::getFreeBuffer ()
{
  static uint8_t  scanBuff = 0;
  while (mbFrame[scanBuff].status != frameStatus::empty)
  {
    scanBuff++;
    if(scanBuff >=  FRAME_COUNT) 
    {
      this->mLogger.error (("\tNo Free buffer\n"));
      scanBuff = 0;
      return 0;
    }    
  }
  //init frame  
  mbFrame[scanBuff].nClient=0;
  mbFrame[scanBuff].len=0;
  mbFrame[scanBuff].millis=0;
  mbFrame[scanBuff].guessedReponseLen=0;
  mbFrame[scanBuff].ascii_response_buffer="";

  return &mbFrame[scanBuff];
}

ModbusTcpSlave::smbFrame * ModbusTcpSlave::getReadyToSendRtuBuffer ()
{
  uint8_t pointer = 255;
  uint8_t pointerMillis = 0;
  for(uint8_t i = 0; i < FRAME_COUNT; i++)
  {
    if(mbFrame[i].status == frameStatus::readyToSendRtu || mbFrame[i].status == frameStatus::readyToSendRtuNoReply)
    {
      // check if current buffer is older
      if ( pointerMillis  < (millis() - mbFrame[i].millis))
      {
        pointerMillis = millis() - mbFrame[i].millis;
        pointer = i;
      }
    }
  }
  if (pointer != 255)
    return &mbFrame[pointer]; //returns the LEAST RECENTLY modified frame buffer
  else
    return NULL;
}

ModbusTcpSlave::smbFrame * ModbusTcpSlave::getWaitFromRtuBuffer ()
{
  for(uint8_t i = 0; i < FRAME_COUNT; i++)
  {
    if(mbFrame[i].status == frameStatus::waitFromRtu )
      return &mbFrame[i];
  }
  return NULL;
}
ModbusTcpSlave::smbFrame *ModbusTcpSlave::getReadyToSendTcpBuffer ()
{
  for(uint8_t i = 0; i < FRAME_COUNT; i++)
  {
    if(mbFrame[i].status == frameStatus::readyToSendTcp )
      return &mbFrame[i];
  }
  return NULL;
}
void ModbusTcpSlave::mbapUnpack(smbap* pmbap, uint8_t * buff )
{
  pmbap->_ti  = *(buff + 0) << 8 |  *(buff + 1);
  pmbap->_pi  = *(buff + 2) << 8 |  *(buff + 3);
  pmbap->_len = *(buff + 4) << 8 |  *(buff + 5);
  pmbap->_ui  = *(buff + 6);
}


