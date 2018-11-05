#ifndef rs485_h
#define rs485_h

#include <coreapi.h>

#include <vector.h>

class Rs485
{
    
public:
    typedef etl::vector<uint8_t, 1024> BINARY_BUFFER_T;

    Rs485();
    virtual ~Rs485();

    //mode text
    _Error setup(const _ApplicationLogger& _theLogger, const JsonObject &root);

    // int setup(Stream &dbgstream);
    // int setup(Stream &dbgstream, const JsonObject &root);
    String sendMasterCommand(String& CMD, int maxReponseWaitTime );    //  maxReponseWaitTime<=0 ==> broadcast  
    String sendMasterCommand(String& CMD);   
    inline void broadcastMasterCommand(String& CMD) {sendMasterCommand(CMD,0);}

    //mode binary
    BINARY_BUFFER_T sendMasterCommand(BINARY_BUFFER_T& CMD, int maxReponseWaitTime);  
    BINARY_BUFFER_T sendMasterCommand(BINARY_BUFFER_T& CMD);  
    inline void broadcastMasterCommand(BINARY_BUFFER_T& CMD) {sendMasterCommand(CMD,0);} 

    //
    inline Stream* getSerialAsStream() {return p_ser;}

    void preTransmit();
    void postTransmit();
    void idle();

private:
    void fixSerialFlush();
    
    const _ApplicationLogger * p_logger;
    // Stream * p_dbgstream;
    Stream * p_ser;

    bool appendLRC=false;
    String prefix;
    int defaultCommandTimeout;

    uint32_t m_bitTime_us=0;
};

#endif
