#ifndef rs485_h
#define rs485_h

#include <ConfigurableSoftwareSerial.h>


class Rs485
{

    Stream * dbgstream;
    ConfigurableSoftwareSerial swSer;
    bool appendLRC=false;
    String prefix="";

public:
    Rs485();

    void setup(Stream &dbgstream);
    String process(String& CMD);    

    static String calculateLRC(String CMD);
};

#endif
