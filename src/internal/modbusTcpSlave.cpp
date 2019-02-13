#include "Arduino.h"

#include "modbusTcpSlave.h"

// WiFiServer mbServer(MODBUSIP_PORT);

#define TCP_TIMEOUT_MS RTU_TIMEOUT

ModbusTcpSlave::ModbusTcpSlave(const _ApplicationLogger& logger, uint16_t port = MODBUSIP_PORT)
: mbServer(port), mLogger(logger)
{
  mbServer.begin();
  mbServer.setNoDelay(true);
  mbServer.setTimeout(TCP_TIMEOUT_MS / 1000);

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
      if (clientOnLine[i].onLine && !clientOnLine[i].client.connected())
      {
        clientOnLine[i].client.stop(); // TEST
        clientOnLine[i].onLine = false;
        this->mLogger.printf (F("\tClient stopped: [%d]\n"), i);

      }
      // else clientOnLine[i].client.flush();
   }

   if (mbServer.hasClient())
   {
     bool clientReg = false;
     for(uint8_t i = 0 ; i < CLIENT_NUM; i++)
     {
       if( !clientOnLine[i].onLine)
       {
          clientOnLine[i].client = mbServer.available();
          clientOnLine[i].onLine = true;
          this->mLogger.printf (F("\tNew Client: [%d] -> %s\n"), i, clientOnLine[i].client.remoteIP().toString().c_str());

          clientReg = true;
          break;
       }
     }

     if (!clientReg) // If there was no place for a new client
     {
        this->mLogger.printf (F("\tToo many Clients reuse first \n") );
        clientOnLine[0].client.stop();
        clientOnLine[0].client = mbServer.available();
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
        readFrameClient(clientOnLine[i].client, i);
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
    this->mLogger.printf (F("\tPaket in : len TCP data [%d] Len mbap pak [%d], UnitId [%d], TI [%d] \n"),
      len, mbap._len, mbap._ui, mbap._ti);

    // checking for glued requests. (wizards are requested for 4 requests)
    while((count < len ) && ((len - count) <= (mbap._len + 6)) && (mbap._pi ==0))
    {
      smbFrame * pmbFrame = this->getFreeBuffer();
      if(pmbFrame == 0) break; // if there is no free buffer then we reduce the parsing
      pmbFrame->nClient = nClient;
      
      if(mbap._ui==0) {
        // UnitId = 0 => broadcast modbus message
        pmbFrame->status = frameStatus::readyToSendRtuNoReply;
      } else {
        pmbFrame->status = frameStatus::readyToSendRtu;

      }
      pmbFrame->len = mbap._len + 6;
      pmbFrame->millis   = millis();

      for (uint16_t j = 0; j < (pmbFrame->len); j++)
        pmbFrame->buffer[j] = buf[j];

      count +=  pmbFrame->len;
      mbapUnpack(&mbap, &buf[count]);
    }
  }
  else
  {
    this->mLogger.printf (F("\tTCP client [%d] data count invalid : %d\n"), nClient, available);

    // uint16_t tmp = client.available();
    while(client.available()) 
      client.read();
    
  }
}

void ModbusTcpSlave::writeFrameClient(void)
{  
  smbFrame * pmbFrame = getReadyToSendTcpBuffer();
  if(pmbFrame)
  {
    uint8_t cli = pmbFrame->nClient;
    size_t len = pmbFrame->len;

    if(! clientOnLine[cli].client.connected() ) {
      this->mLogger.printf (F("\tERROR writeFrameClient: writing to a disconnected client: %d"), cli);
    }

    this->mLogger.printf (F("TEST writeFrameClient: len=%d, cli=%d\n"), len, cli);
    // write to TCP client
    size_t written = clientOnLine[cli].client.write(&pmbFrame->buffer[0], len);

    if(written!= len) {
      this->mLogger.printf (F("\tERROR writeFrameClient: writing buffer [%d] to RTU client len_to_write=%d, written=%d\n"), cli, len, written);
    }
    // delay(1);
    // yield();
    clientOnLine[cli].client.flush();
    pmbFrame->status = frameStatus::empty;
  }
}

void ModbusTcpSlave::task()
{
  waitNewClient();
  yield();
  readDataClient();
  yield();  
  writeFrameClient();
  yield();
  timeoutBufferCleanup();
}

void ModbusTcpSlave::timeoutBufferCleanup() {
  // Cleaning the buffers
  for(uint8_t i = 0; i < FRAME_COUNT; i++)
  {
    if(mbFrame[i].status != frameStatus::empty ) {
      if (millis() - mbFrame[i].millis > RTU_TIMEOUT)
      {
        mbFrame[i].status = frameStatus::empty;
        this->mLogger.printf (F("\tRTU_TIMEOUT -> Del pack.\n"));        
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
      this->mLogger.printf (F("\tNo Free buffer\n"));
      scanBuff = 0;
      return 0;
    }    
  }
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
    return 0;
}

ModbusTcpSlave::smbFrame * ModbusTcpSlave::getWaitFromRtuBuffer ()
{
  for(uint8_t i = 0; i < FRAME_COUNT; i++)
  {
    if(mbFrame[i].status == frameStatus::waitFromRtu )
      return &mbFrame[i];
  }
  return 0;
}
ModbusTcpSlave::smbFrame *ModbusTcpSlave::getReadyToSendTcpBuffer ()
{
  for(uint8_t i = 0; i < FRAME_COUNT; i++)
  {
    if(mbFrame[i].status == frameStatus::readyToSendTcp )
      return &mbFrame[i];
  }
  return 0;
}
void ModbusTcpSlave::mbapUnpack(smbap* pmbap, uint8_t * buff )
{
  pmbap->_ti  = *(buff + 0) << 8 |  *(buff + 1);
  pmbap->_pi  = *(buff + 2) << 8 |  *(buff + 3);
  pmbap->_len = *(buff + 4) << 8 |  *(buff + 5);
  pmbap->_ui  = *(buff + 6);
}


