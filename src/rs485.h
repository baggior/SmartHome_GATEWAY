#ifndef rs485_h
#define rs485_h

#include <ConfigurableSoftwareSerial.h>


class Rs485
{

    Stream * dbgstream;
    ConfigurableSoftwareSerial swSer;
   

public:
    Rs485();

    void setup(Stream &serial);
    String process(String& CMD);    

    static String calculateLRC(String CMD);
};

#endif
