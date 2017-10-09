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
    String sendMasterCommand(String& CMD, int maxReponseWaitTime=500 );    //  maxReponseWaitTime<=0 ==> broadcast  
    inline void broadcastMasterCommand(String& CMD) {sendMasterCommand(CMD,0);}

private:
    static String calculateLRC(String CMD);
};

#endif
