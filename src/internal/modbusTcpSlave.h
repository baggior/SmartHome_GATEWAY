#ifndef MBTCPSLAVE_H
#define MBTCPSLAVE_H


// #include <WiFi.h>
#include "coreapi.h"

#define MODBUSIP_MAXFRAME   200
#define TCP_BUFFER_SIZE     300
#define FRAME_COUNT          10
#define CLIENT_NUM            4
#define MODBUSIP_PORT       502
#define RTU_TIMEOUT        5000
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
        uint32_t millis;                // Time of sending the package to Serial
    };

    struct smbap
    {
        uint16_t  _ti;  // Transaction Identifier
        uint16_t  _pi;  // Protocol Identifier = 0
        uint16_t  _len; // Length
        uint8_t   _ui;  // Unit Identifier
    };

    struct
    {
        WiFiClient client;
        bool onLine;
    } clientOnLine[CLIENT_NUM];

    struct smbFrame mbFrame[FRAME_COUNT];

    WiFiServer mbServer;
    const _ApplicationLogger& mLogger;

    void waitNewClient (void);  //
    void readDataClient(void);  // customer scan for data availability
    void readFrameClient(WiFiClient  client, uint8_t i);   // parsing data from the client
    void writeFrameClient(void);
    void timeoutBufferCleanup();

    void mbapUnpack (smbap* pmbap, uint8_t * buff );

public:
    ModbusTcpSlave(const _ApplicationLogger& logger, uint16_t port);
    ~ModbusTcpSlave();
    void task(void);
    smbFrame * getFreeBuffer (void);
    smbFrame * getReadyToSendRtuBuffer (void);
    smbFrame * getWaitFromRtuBuffer (void);
    smbFrame * getReadyToSendTcpBuffer (void);

};


#endif