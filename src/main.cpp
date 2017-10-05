
#include "config.h"

#include "wifiConnection.h"
#include "wifiRestServer.h"
#include "wifiTelnetServer.h"
#include "rs485.h"
#include "webSocket.h"


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

    config.load();   

    pinMode(BUILTIN_LED, OUTPUT);     
    digitalWrite(BUILTIN_LED, LOW); //LED ON
    wifiManagerOpenConnection(Serial);
    digitalWrite(BUILTIN_LED, HIGH); //LED OFF           

    wifiTelnetServer.setup(Serial);    
    restServer.setup(Serial);
    webSocket.setup(Serial);
    rs485.setup(Serial);
    
    DPRINTLN("main seup done");
}

void loop() {  

    digitalWrite(BUILTIN_LED, LOW);   // turn the LED on

    // put your main code here, to run repeatedly:
    DPRINTLN("main loop start..");

    String CMD_received = wifiTelnetServer.process();
    DPRINTLN("..processed Telnet: received [" + CMD_received +"]");       

    
    restServer.process();
    DPRINTLN("..processed REST");    
    
    webSocket.process();
    DPRINTLN("..processed webSocket");
    
    //String CMD="020300CA0001";  //:020300CA000130
    if(CMD_received.length()>0)
    {
        String CMD_response = rs485.process(CMD_received);
        DPRINTLN("..processed rs485 sent [" + CMD_received +"] received [" + CMD_response +"]");
        
        if(CMD_response.length()>0)
        {
            wifiTelnetServer.send(CMD_response);
            DPRINTLN("..processed Telnet response [" + CMD_response + "]");
        }        
    }    

    DPRINTLN("main loop done");
    
    digitalWrite(BUILTIN_LED, HIGH);    // turn the LED off 

    delay(500);
}

