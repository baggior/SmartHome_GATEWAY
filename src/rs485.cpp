#include "config.h"

#include "rs485.h"


#define RS485_RO_RX             D1      //receive out
#define RS485_DI_TX             D2      //data in
#define RS485_REDE_CONTROL      D6      //receive enable / data enable
#define RS485_Tx                HIGH    //control send
#define RS485_Rx                LOW     //control receive

#define UART_BAUD               19200
#define UART_STOP_BITS          2
#define UART_PARITY             'N'
#define UART_DATA_BITS          7


Rs485::Rs485()
: swSer(RS485_RO_RX, RS485_DI_TX)
{
  this->dbgstream=&Serial;
}

void Rs485::setup(Stream& dbgstream)
{
    swSer.begin(UART_BAUD, UART_STOP_BITS, UART_PARITY, UART_DATA_BITS);
    swSer.enableRx(true);
    swSer.setTransmitEnablePin(RS485_REDE_CONTROL);
    
    // pinMode(RS485_REDE_CONTROL, OUTPUT);
    // digitalWrite(RS485_REDE_CONTROL, RS485_Rx);

    this->dbgstream=&dbgstream;
}



String Rs485::process(String& CMD)
{
  String RESPONSE;
  //SLAVE DEVICE 
  //seriale : 205513 (0x31AF9)
  //ID: 02

  //esempio di sequenza di polling standard valida
  /*
    :020300320001C8
      :0203020000F9
    :020300C800052E
      :02030A01050000000C0034009714
    :020300CA000130
      :020302000CED
  */  

  //esempio di sequenza di avvio
  /*
    :020300CA000130
      :0203020000F9
    :021000CA00010200041D
      :021000CA000123

    :020300CA000130
      :0203020004F5
    :020300010008F2
      :020310000200032C8F000CADEC00020000000084
    :02030191000168
      :0203020004F5
    :02030192000167
      :0203020002F7
    :020300C800052E
      :02030A00FF000000040035009326

    :021000CA000102000C15
      :021000CA000123

  */
    
  if(CMD.length()>0)
  {
    //String cmd="020300CA0001";   //":020300CA000130"
    String LRC = calculateLRC( CMD );
    
    dbgstream->println("cmd= "+CMD+", LRC="+LRC);
      
    String packet = String(":") + CMD + LRC;
    
    // digitalWrite(RS485_REDE_CONTROL, RS485_Tx);
    // delay(10);
  
    swSer.flush();//?
    swSer.println(packet);
  
    // digitalWrite(RS485_REDE_CONTROL, RS485_Rx);
  
    dbgstream->print("RS485 SENT: [");dbgstream->print(packet);dbgstream->println("]");
    delay(200);
  }
  

  // legge il contenuto del bus e lo stampa sulla Seriale per debug
  
  while (swSer.available() > 0) {
    RESPONSE += swSer.readString();    
  } 
  RESPONSE.trim();
  dbgstream->println("RS485 READ: ["+RESPONSE+"]");    


  return RESPONSE;
  //valore atteso :020302000CED
}


String Rs485::calculateLRC(String CMD)
{
    unsigned char TOT = 0x00;

    int index, count;
    count = CMD.length();
    
    for (index = 0; index<count; index++)
    {
      String hex_str( CMD[index]);
      index++;
      if(index<count)
      {
        hex_str += String(CMD[index]);
        unsigned char c= ( strtoul(hex_str.c_str(), NULL, 16) & 0xFF );
        TOT += c;
      }      
    }

    //unsigned char TOT_c = (0xFF-TOT) +1 ;   // 1's complement  // 2's complements
        
    unsigned char TOT_c = ( (~TOT +1) & 0xFF );

    if((TOT + TOT_c)& 0xFF)
    {
      DPRINTF("ERROR in LRC: [TOT + TOT_c <> 0] : %0X + %0X ",TOT, TOT_c );
    }
    
    String LRC_String(TOT_c, 16);
    LRC_String.toUpperCase();
    return LRC_String;
   }