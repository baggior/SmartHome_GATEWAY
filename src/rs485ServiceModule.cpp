// #include "config.h"


#include "rs485ServiceModule.h"
//#include "ExponentialBackoffTimer.h"
#include <ConfigurableSoftwareSerial.h>
#include <HardwareSerial.h>

#ifdef ESP8266
#define RS485_RO_RX D1        //receive out
#define RS485_DI_TX D2        //data in
#define RS485_REDE_CONTROL D6 //receive enable / data enable

#elif defined ESP32 
//                          uart2     uart1   software
#define RS485_RO_RX  27     //16      //9     //27        //receive out
#define RS485_DI_TX  26     //17      //10    //26        //data in

#define RS485_REDE_CONTROL 25 //receive enable / data enable
#endif

#define UART_BAUD 19200
#define UART_STOP_BITS 2
#define UART_PARITY "N"
#define UART_DATA_BITS 7

#define DEFAULT_COMMAND_TIMEOUT 500

// -----------------------------------------------

#define RS485_DEBUG_FN(...)  { if(p_logger) {p_logger->debug(__VA_ARGS__);} }

// -----------------------------------------------

union conf0_t {
  struct {
      uint32_t parity:             1;         /*This register is used to configure the parity check mode.  0:even 1:odd*/
      uint32_t parity_en:          1;         /*Set this bit to enable uart parity check.*/
      uint32_t bit_num:            2;         /*This register is used to set the length of data:  0:5bits 1:6bits 2:7bits 3:8bits*/
      uint32_t stop_bit_num:       2;         /*This register is used to set the length of  stop bit. 1:1bit  2:1.5bits  3:2bits*/
      uint32_t sw_rts:             1;         /*This register is used to configure the software rts signal which is used in software flow control.*/
      uint32_t sw_dtr:             1;         /*This register is used to configure the software dtr signal which is used in software flow control..*/
      uint32_t txd_brk:            1;         /*Set this bit to enable transmitter to  send 0 when the process of sending data is done.*/
      uint32_t irda_dplx:          1;         /*Set this bit to enable irda loop-back mode.*/
      uint32_t irda_tx_en:         1;         /*This is the start enable bit for irda transmitter.*/
      uint32_t irda_wctl:          1;         /*1：the irda transmitter's 11th bit is the same to the 10th bit. 0：set irda transmitter's 11th bit to 0.*/
      uint32_t irda_tx_inv:        1;         /*Set this bit to inverse the level value of  irda transmitter's level.*/
      uint32_t irda_rx_inv:        1;         /*Set this bit to inverse the level value of irda receiver's level.*/
      uint32_t loopback:           1;         /*Set this bit to enable uart loop-back test mode.*/
      uint32_t tx_flow_en:         1;         /*Set this bit to enable transmitter's flow control function.*/
      uint32_t irda_en:            1;         /*Set this bit to enable irda protocol.*/
      uint32_t rxfifo_rst:         1;         /*Set this bit to reset uart receiver's fifo.*/
      uint32_t txfifo_rst:         1;         /*Set this bit to reset uart transmitter's fifo.*/
      uint32_t rxd_inv:            1;         /*Set this bit to inverse the level value of uart rxd signal.*/
      uint32_t cts_inv:            1;         /*Set this bit to inverse the level value of uart cts signal.*/
      uint32_t dsr_inv:            1;         /*Set this bit to inverse the level value of uart dsr signal.*/
      uint32_t txd_inv:            1;         /*Set this bit to inverse the level value of uart txd signal.*/
      uint32_t rts_inv:            1;         /*Set this bit to inverse the level value of uart rts signal.*/
      uint32_t dtr_inv:            1;         /*Set this bit to inverse the level value of uart dtr signal.*/
      uint32_t clk_en:             1;         /*1：force clock on for registers：support clock only when write registers*/
      uint32_t err_wr_mask:        1;         /*1：receiver stops storing data int fifo when data is wrong. 0：receiver stores the data even if the  received data is wrong.*/
      uint32_t tick_ref_always_on: 1;         /*This register is used to select the clock.1：apb clock：ref_tick*/
      uint32_t reserved28:         4;
  };
  uint32_t val;
};

// -----------------------------------------------

static ConfigurableSoftwareSerial* p_sWser=NULL;
static HardwareSerial* p_hWser=NULL;


static Stream* initSerial(int _uart_nr, int _baud, int _databits, int _stopbits, char _parity) {

  if(_uart_nr==3) // _uart_nr=3 -> SOFTWARE SERIAL
  {
    //SOFTWARE SERIAL
    p_sWser = new ConfigurableSoftwareSerial(RS485_RO_RX, RS485_DI_TX);
  
    p_sWser->begin(_baud, _stopbits, _parity, _databits);
    p_sWser->setTransmitEnablePin(RS485_REDE_CONTROL);
    p_sWser->enableRx(true);

    DPRINTF(F("\t Software serial configured. \n"));

    return p_sWser;      
  }
  else
  {
    //HARDWARE SERIAL
    if (_uart_nr<0 || _uart_nr>3) {
      DPRINTF(F("\tERROR: Hardware serial -> uart out of range: %d \n"), _uart_nr);
      return NULL; //uart out of range
    }
    if(_databits<5 && _databits>8) {
      DPRINTF(F("\tERROR: Hardware serial -> databit out of range: %d \n"), _databits);
      return NULL; //databit out of range
    }
    if(_stopbits<1 && _stopbits>2) {
      DPRINTF(F("\tERROR: Hardware serial -> _stopbits out of range: %d \n"), _stopbits);
      return NULL; //_stopbits out of range
    }

    conf0_t c;
    c.val = SERIAL_8N1;
    c.parity = ('O'==_parity)?1:0;
    c.parity_en = ('N'==_parity)?0:1;
    c.bit_num = (_databits==5)?0:(_databits==6)?1:(_databits==7)?2:(_databits==8)?3 : 3;// error
    c.stop_bit_num = (_stopbits==1)?1:(_stopbits==2)?3 : 1; //error 
  
    switch(_uart_nr) {
      case 0:
        p_hWser = &Serial;
        break;
      case 1:
        p_hWser = &Serial1;
        break;

#ifdef ESP32        

      case 2:
        p_hWser = &Serial2;
        break;
        
#endif        
      default:
        return NULL;
    }
    // p_hWser = new HardwareSerial( _uart_nr );

    p_hWser->begin(_baud, c.val); //, RS485_RO_RX, RS485_DI_TX);
    p_hWser->setDebugOutput(false);

    DPRINTF(F("\t Hardware serial configured %X \n"), c.val);

    return p_hWser;
  }

}

static void endSerial() {
  if(p_hWser)
  {
    p_hWser->end();
    p_hWser = NULL;
  }
  if(p_sWser)
  {
    delete p_sWser;
    p_sWser = NULL;
  }
}



static String calculateLRC(String CMD, _ApplicationLogger* p_logger)
{
  unsigned char TOT = 0x00;

  int index, count;
  count = CMD.length();

  for (index = 0; index < count; index++)
  {
    String hex_str(CMD[index]);
    index++;
    if (index < count)
    {
      hex_str += String(CMD[index]);
      unsigned char c = ( ::strtoul(hex_str.c_str(), NULL, 16) & 0xFF);
      TOT += c;
    }
  }

  //unsigned char TOT_c = (0xFF-TOT) +1 ;   // 1's complement  // 2's complements
  unsigned char TOT_c = ((~TOT + 1) & 0xFF);
  if ((TOT + TOT_c) & 0xFF)
  {
    if(p_logger) p_logger->error (("ERROR in LRC: [TOT + TOT_c <> 0] : %0X + %0X "), TOT, TOT_c); 
  }

  String LRC_String(TOT_c, 16);
  LRC_String.toUpperCase();
  return LRC_String;
}
// -----------------------------------------------

Rs485ServiceModule::Rs485ServiceModule(String _title, String _descr)
: _BaseModule(_title,_descr, false, Order_First, true),
  p_logger(NULL), 
  m_bitTime_us(0), p_ser(NULL), 
  defaultCommandTimeout(DEFAULT_COMMAND_TIMEOUT)
{ }

Rs485ServiceModule::Rs485ServiceModule()
: Rs485ServiceModule( "_Rs485Service", "servizio per la comunicazione seriale RS485")
{ }

Rs485ServiceModule::~Rs485ServiceModule() {
  this->shutdown();
  
}
_Error Rs485ServiceModule::setup(const JsonObject &root)
{  
  this->p_logger = &this->theApp->getLogger();

  if (root.isNull())  {
    this->p_logger->error((">Rs485 Error initializing configuration. Json file error\n"));
    return _ConfigLoadError;
  }
  
  int _uart_num = root["uart"];
  this->appendLRC = root["appendLRC"];
  const char * _prefix = root["prefix"];
  int _baud = root["baud"];
  int _databits = root["databits"];
  int _stopbits = root["stopbits"];
  const char * _parity = root["parity"];
  this->defaultCommandTimeout = root["defaultCommandTimeout"];
  
  this->p_logger->info(("\t%s Rs485 config: prefix: %s, appendLRC: %d, defaultCommandTimeout: %d, uart: %d, baud: %d, databits: %d, stopbits: %d, parity: %s \n"),
          this->getTitle().c_str(),
          REPLACE_NULL_STR(_prefix), this->appendLRC, defaultCommandTimeout, _uart_num, _baud, _databits, _stopbits, REPLACE_NULL_STR(_parity) );

  // this->p_dbgstream = &dbgstream;
  if (_prefix)
    this->prefix = prefix;
  
  if(!this->defaultCommandTimeout)
    this->defaultCommandTimeout = DEFAULT_COMMAND_TIMEOUT;

  if (!_baud)
    _baud = UART_BAUD;
  if (!_databits)
    _databits = UART_DATA_BITS;
  if (!_stopbits)
    _stopbits = UART_STOP_BITS;
  if (!_parity)
    _parity = UART_PARITY;

  this->p_ser = initSerial(_uart_num, _baud,_databits,_stopbits, String(_parity).charAt(0));

  if(!p_ser) {
    this->p_logger->error((">Rs485 Error initializing Serial. Config out of range\n"));
    return _Error(2, "Rs485 Error: impossibile inizializzare la seriale");
  }
  
  p_ser->flush();

  pinMode(RS485_REDE_CONTROL, OUTPUT);
  digitalWrite(RS485_REDE_CONTROL, LOW);

  //Wait for 5+  8 data bits, 1 parity and 1 stop bits, just in case
  this->m_bitTime_us = (5+_databits+1+_stopbits)*( (1000000 / _baud) + 2 );

  this->p_logger->info (("\tRs485 setup done: bitTime=%d us\n"), this->m_bitTime_us);

  return _NoError;
}


_Error Rs485ServiceModule::setup() 
{  
  const JsonObject& root = this->theApp->getConfig().getJsonObject("rs485");  
  if(!root.isNull()) 
  {    
    return this->setup(root);
  }

  return _Disable;
}

void Rs485ServiceModule::shutdown()
{
  this->theApp->getLogger().info(("%s: Module shutdown..\n"), this->getTitle().c_str());
  
  if(p_ser!=NULL) {

    p_ser->flush();
    
    endSerial();

    p_ser = NULL;
  }
}

String Rs485ServiceModule::sendMasterCommand(String &CMD)
{
  return sendMasterCommand(CMD, this->defaultCommandTimeout);
}

//  maxReponseWaitTime<=0 ==> broadcast -> no response)
String Rs485ServiceModule::sendMasterCommand(String &CMD, int maxReponseWaitTime)
{ 
  if(!p_ser) return "";
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
  
  
  if (CMD.length() > 0)
  {
    //String cmd="020300CA0001";   //":020300CA000130"
    
    String packet = String(prefix);
    packet += CMD;
    
    if (appendLRC)
    {
      String LRC = calculateLRC(CMD, this->p_logger);
      packet += LRC;
    }
    
    // delay(10);
    this->preTransmit();

    p_ser->println(packet);

    this->postTransmit();

    
    RS485_DEBUG_FN( ("RS485 SENT: [%s]"),packet.c_str());

    // dbgstream->print("RS485 SENT: [");    dbgstream->print(packet);    dbgstream->println("]");
  }
  
  String RESPONSE;
  if(maxReponseWaitTime>0) //attendi la risposta (altrimenti è un broadcast senza risposta)
  {
    //wait for a RESPONSE    
    long delayTime = _min(maxReponseWaitTime,10);
    delay(delayTime);
    while (p_ser->available()==0 && delayTime<maxReponseWaitTime)
    {
      delayTime = _min(maxReponseWaitTime,3*delayTime);
      delay(delayTime);
    }
    
    while (p_ser->available() > 0)
    {
      // legge il contenuto del bus 
      RESPONSE += p_ser->readString();
      yield();      
    }
  }
    
  RESPONSE.trim();
  RS485_DEBUG_FN( ("RS485 READ: [%s]"),RESPONSE.c_str());
  // dbgstream->println("RS485 READ: [" + RESPONSE + "]");       

  return RESPONSE;
  //valore atteso :020302000CED
}

Rs485ServiceModule::BINARY_BUFFER_T Rs485ServiceModule::sendMasterCommand(Rs485ServiceModule::BINARY_BUFFER_T& CMD)
{
  return sendMasterCommand(CMD, this->defaultCommandTimeout);
}

Rs485ServiceModule::BINARY_BUFFER_T Rs485ServiceModule::sendMasterCommand(Rs485ServiceModule::BINARY_BUFFER_T& CMD, int maxReponseWaitTime)
{
  if(!p_ser) return BINARY_BUFFER_T();

  BINARY_BUFFER_T RESPONSE;

  this->preTransmit();

  for (uint8_t data: CMD) 
  {
    size_t ret = p_ser->write(data);   
    RS485_DEBUG_FN( ("serial write data: 0x%0X\n"), data) ;
    // Stream_printf(*dbgstream, F("serial write %d\n"), data);
  }

  this->postTransmit();
  

  if(maxReponseWaitTime>0)
  {
    //wait for a RESPONSE    
    size_t delayTime = _min(maxReponseWaitTime,10);
    RS485_DEBUG_FN( ("serial wait %d\n"), delayTime) ;
    // Stream_printf(*dbgstream, F("serial wait %d\n"), delayTime);
    delay(delayTime);
    while (p_ser->available()==0 && delayTime<maxReponseWaitTime)
    {
      delayTime = _min(maxReponseWaitTime,3*delayTime);
      RS485_DEBUG_FN( ("serial wait %d\n"), delayTime) ;
      delay(delayTime);
    }
    
    while (p_ser->available() > 0)
    {
      // legge il contenuto del bus 
      RS485_DEBUG_FN( ("serial available to read %d bytes.."), p_ser->available()) ;      
      uint8_t data = p_ser->read();
      RS485_DEBUG_FN( (", read data: 0x%0X\n"), data);

      RESPONSE.push_back(data);
      yield();      
    }
  }
  return RESPONSE;
}

size_t Rs485ServiceModule::write(const String& asciibuffer) 
{
  size_t count = 0;
  if(this->p_ser && asciibuffer.length()>0) {

    this->preTransmit();
    
    count = p_ser->write( asciibuffer.c_str(), asciibuffer.length() );                        

    this->postTransmit();

  }
  return count;
}

size_t Rs485ServiceModule::write(uint8_t* buffer, size_t size)
{
  size_t count = 0;
  if(this->p_ser && buffer) {

    this->preTransmit();
    
    count = p_ser->write( buffer, size );                        

    this->postTransmit();

  }
  return count;
}



void Rs485ServiceModule::preTransmit() {

  // flush receive buffer before transmitting request
  while (this->p_ser->read() != -1);
  // this->p_ser->flush();

  digitalWrite(RS485_REDE_CONTROL, HIGH);
}

void Rs485ServiceModule::postTransmit() {

  // flush transmit buffer
  this->p_ser->flush();

  // TODO test if neededed:
  //Workaround for a bug in serial (ESP32 too fast) not actually being finished yet
  //Wait for 8 data bits, 1 parity and 2 stop bits, just in case  
  // delayMicroseconds(m_bitTime_us);

  digitalWrite(RS485_REDE_CONTROL, LOW);
}

void Rs485ServiceModule::idle() {
}


/**
 *  MODBUS RTU SPEC NOTES
 * 
 * Character size:
 * The format ( 11 bits ) for each byte in RTU mode is :
 * Coding System: 8–bit binary
 * Bits per Byte: 1 start bit
 * 8 data bits, least significant bit sent first
 * 1 bit for parity completion
 * 1 stop bit 
 * 
 *  RTU Framing timings
 * In RTU mode, message frames are separated by a silent interval of at least 3.5 character times. In the following sections, this time interval is called t3,5. 
 * If a silent interval of more than 1.5 character times occurs between two characters, the message frame is declared incomplete and should be discarded by the receiver. 
 * 
 * The implementation of RTU reception driver may imply the management of a lot of interruptions due to the t1.5 and t3.5 timers. With 
 * high communication baud rates, this leads to a heavy CPU load. Consequently these two timers must be strictly respected when the
 * baud rate is equal or lower than 19200 Bps. For baud rates greater than 19200 Bps, fixed values for the 2 timers should be used: it is
 * recommended to use a value of 750µs for the inter-character time-out (t1.5) and a value of 1.750ms for inter-frame delay (t3.5).
 * 
 * 
 * 
 * 
 **/

