#ifndef rs485svmodule_h
#define rs485svmodule_h

#include <coreapi.h>

#include <vector.h>

class Rs485ServiceModule : public _ServiceModule
{
    
public:
    typedef etl::vector<uint8_t, 1024> BINARY_BUFFER_T;

    Rs485ServiceModule();
    virtual ~Rs485ServiceModule();

    
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

protected:
    _Error setup(const JsonObject &root);
    virtual _Error setup() override;
    virtual void shutdown() override;

private:
    void fixSerialFlush();
    
    // Stream * p_dbgstream;

    const _ApplicationLogger * p_logger=NULL;

    bool appendLRC=false;
    String prefix;
    uint32_t m_bitTime_us=0;
    Stream * p_ser=NULL;
    int defaultCommandTimeout;
};

#endif
