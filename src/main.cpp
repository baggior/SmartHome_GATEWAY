
#include "config.h"

#include "wifiConnection.h"
#include "wifiRestServer.h"
#include "wifiTelnetServer.h"
#include "rs485.h"
#include "webSocket.h"
#include <Blinker.h>
#include <TaskScheduler.h>


PRAGMA_MESSAGE (VAR_NAME_VALUE(ARDUINO))
PRAGMA_MESSAGE (VAR_NAME_VALUE(ESP8266))
PRAGMA_MESSAGE (VAR_NAME_VALUE(__AVR__))
PRAGMA_MESSAGE (VAR_NAME_VALUE(MY_DEBUG))
PRAGMA_MESSAGE (VAR_NAME_VALUE(DEBUG_OUTPUT))
PRAGMA_MESSAGE (VAR_NAME_VALUE(DEBUG_ESP_PORT))
PRAGMA_MESSAGE (VAR_NAME_VALUE(DEBUG_ESP_WIFI))

Config config;

WifiTelnetServer wifiTelnetServer;
WifiRestServer restServer;
WebSocket webSocket;

Rs485 rs485;

Blinker b;    


void leOsTask1()
{
    DPRINTF("Task1 millis: %u", millis());
    yield();
}


void setup() {
    // put your setup code here, to run once:    
    Serial.flush();
    Serial.begin(9600);
    Serial.println();
    DPRINTLN("main setup start");
/* 
#ifdef MY_DEBUG
    if(SaveCrash.count()>10)
    {
        DPRINTLN("OVERFLOW CLEAR PREVIOUS CRASH INFO (>10)");
        SaveCrash.clear();
    }
    if(SaveCrash.count()>0)
    {
        DPRINTLN("PREVIOUS CRASH INFO:");
        SaveCrash.print(Serial);
        DPRINTLN("-------------------");
    
    }
#endif */

pinMode(BUILTIN_LED, OUTPUT);     

b.start(0.5);//500 msec.


    config.load();   

    //digitalWrite(BUILTIN_LED, LOW); //LED ON
    wifiManagerOpenConnection(Serial);
    //digitalWrite(BUILTIN_LED, HIGH); //LED OFF           


    wifiTelnetServer.setup(Serial);    
    restServer.setup(Serial);
    webSocket.setup(Serial);
    rs485.setup(Serial);
    
    DPRINTLN("main seup done");

    

b.start(5);//5sec.
}


void loop() 
{  
    

    // put your main code here, to run repeatedly:
    //DPRINTLN("main loop start..");

    uint8_t response_to=0;

    String CMD_received = wifiTelnetServer.process();
    if(CMD_received.length()>0)
    {
        DPRINTLN("..processed Telnet: received [" + CMD_received +"]");       
        response_to=1;
    }
    else if (webSocket.process())
    {
        CMD_received = webSocket.getLastCommandReceived();
        DPRINTLN("..processed webSocket: received [" + CMD_received +"]");  
        response_to=2;
    }
    else if(restServer.process())
    {
        ;
        DPRINTLN("..processed REST..TODO");    
        response_to=3;
    }    
    
    //String CMD="020300CA0001";  //:020300CA000130
    if(CMD_received.length()>0)
    {
        String CMD_response = rs485.process(CMD_received);
        DPRINTLN("..processed rs485 sent [" + CMD_received +"] received [" + CMD_response +"]");
        
        //response
        if(CMD_response.length()>0)
        {
            switch (response_to)
            {
                case 1:
                {
                    wifiTelnetServer.send(CMD_response);
                    DPRINTLN("..sent Telnet response [" + CMD_response + "]");
                    break;
                }    
                case 2:
                {
                    webSocket.send(CMD_response);
                    DPRINTLN("..sent webSocket response [" + CMD_response + "]");
                    break;
                }
                case 3:    
                {
                    restServer.send(CMD_response);
                    DPRINTLN("..sent restServer response [" + CMD_response + "]");
                    break;
                }
                default:
                {
                    DPRINTLN("..ERROR: can't sent response [" + CMD_response + "] WHERE??");
                }
            }
        }

    }    

    //DPRINTLN("main loop done");    
   
    yield();
}

