#ifndef rs485svmodule_h
#define rs485svmodule_h

#include <coreapi.h>

#include <vector.h>

class Rs485ServiceModule : public _BaseModule
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
    inline Stream* getSerialAsStream() { return p_ser; }

    void preTransmit();
    void postTransmit();
    void idle();

    size_t write(uint8_t* buffer, size_t count);
    size_t write(const String& asci_buffer);

protected:
    Rs485ServiceModule(String _title, String _descr);

    virtual _Error setup(const JsonObject &root) ;//override;
    virtual _Error setup() override;
    virtual void shutdown() override;

private:
    inline virtual void loop() override { } //task loop not used for a service module  

    // Stream * p_dbgstream;

    _ApplicationLogger * p_logger=NULL;

    bool appendLRC=false;
    String prefix;
    uint32_t m_bitTime_us=0;
    Stream * p_ser=NULL;
    int defaultCommandTimeout;
};

#endif
