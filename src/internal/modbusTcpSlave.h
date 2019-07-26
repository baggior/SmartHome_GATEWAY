#ifndef MBTCPSLAVE_H
#define MBTCPSLAVE_H


// #include <WiFi.h>
#include <coreapi.h>

#define MODBUSIP_MAXFRAME   200
#define TCP_BUFFER_SIZE     300
#define FRAME_COUNT          35
#define CLIENT_NUM            4
#define MODBUSIP_PORT       502
#define RTU_TIMEOUT        2000
#define TCP_MBAP_SIZE         6       

class ModbusTcpSlave
{
    friend class ModbusTCPGatewayModule;

private:

    typedef enum eFrameStatus
    { 
        empty,
        readyToSendRtu,
        readyToSendRtuNoReply,
        waitFromRtu,
        readyToSendTcp
    } frameStatus;

    struct smbFrame
    {
        eFrameStatus status;
        uint8_t nClient;
        uint8_t buffer[TCP_BUFFER_SIZE];
        uint16_t len;
        uint32_t millis;                    // Time of sending the package to Serial
        uint16_t guessedReponseLen =0;      // when received guessedReponseLen the response is considered completed 
        String ascii_response_buffer;
    };

    struct smbap
    {
        uint16_t  _ti;  // Transaction Identifier
        uint16_t  _pi;  // Protocol Identifier = 0
        uint16_t  _len; // Length
        
        uint8_t   _ui;  // Unit Identifier
    };

    struct clientOnLine_t
    {
        WiFiClient client;
        bool onLine;
    };

    clientOnLine_t clientOnLine[CLIENT_NUM];
    struct smbFrame mbFrame[FRAME_COUNT];

    WiFiServer mbServer;
    _ApplicationLogger& mLogger;
    const bool isDebug = false;

    void waitNewClient (void);  //
    void readDataClient(void);  // customer scan for data availability
    void readFrameClient(WiFiClient  client, uint8_t i);   // parsing data from the client
    void writeFrameClient(void);
    void timeoutBufferCleanup();

    void mbapUnpack (smbap* pmbap, uint8_t * buff );

public:
    ModbusTcpSlave(_ApplicationLogger& logger, uint16_t port, bool _isDebug);
    ~ModbusTcpSlave();
    // void task(void);
    smbFrame * getFreeBuffer ();
    smbFrame * getReadyToSendRtuBuffer ();
    smbFrame * getWaitFromRtuBuffer ();
    smbFrame * getReadyToSendTcpBuffer ();

};


#endif