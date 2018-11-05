
#include "config.h"

// #include "wifiConnection.h"


#include "wifiTelnetServer.h"
// #include "rs485.h"
// #include "webSocketRs485Gateway.h"
#include "ota.h"
#include "mqttclient.h"
#include "modbus.h"

#include "coreapi.h"
#include "coreapi_ftpmodule.h"

#include "wifiRestServerModule.h"

PRAGMA_MESSAGE (VAR_NAME_VALUE(ARDUINO))
PRAGMA_MESSAGE (VAR_NAME_VALUE(ARDUINO_VARIANT))
PRAGMA_MESSAGE (VAR_NAME_VALUE(ESP8266))
PRAGMA_MESSAGE (VAR_NAME_VALUE(ESP32))
PRAGMA_MESSAGE (VAR_NAME_VALUE(__AVR__))
PRAGMA_MESSAGE (VAR_NAME_VALUE(MY_DEBUG))
PRAGMA_MESSAGE (VAR_NAME_VALUE(DEBUG_OUTPUT))
PRAGMA_MESSAGE (VAR_NAME_VALUE(DEBUG_ESP_PORT))
PRAGMA_MESSAGE (VAR_NAME_VALUE(DEBUG_ESP_WIFI))

//TODO delete
Config config;
// WiFiConnection connection;

WifiFtpServerModule wifiFtpServer;
WifiTelnetServer wifiTelnetServer;
WifiRestServerModule restServer;
// WebSocketRs485Gateway webSocketRs485Gateway;

// Rs485 rs485;

Ota ota;

Modbus modbus;
//MqttClient mqtt;


// Scheduler runner;

///NEW///
_Application app;

#ifndef UNIT_TEST  

void setup() {
    delay(2000);

    // put your setup code here, to run once:    
    Serial.end();
    Serial.begin(115200);

    #ifdef MY_DEBUG
    Serial.setDebugOutput(true);
    #endif
    
    Serial.println();
    DPRINTLN("\n-----OLD Main setup start-----\n");
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

config.getBlinker().start(0.1);//100 msec.
    //
    config.load();   

    //digitalWrite(BUILTIN_LED, LOW); //LED ON
    // connection.setup(Serial); //open wifi connection    
    //digitalWrite(BUILTIN_LED, HIGH); //LED OFF           

config.getBlinker().start(1);//1sec.

    // wifiFtpServer.setup(Serial);
    wifiTelnetServer.setup(Serial);    
    // restServer.setup(Serial);
    //webSocketRs485Gateway.setup(Serial);
    // rs485.setup(Serial);
    //mqtt.setup(Serial);
    modbus.setup(Serial);
    
      
    // connection.announceTheDevice();
    
config.getBlinker().start(3);//3sec.
    // DPRINTLN("Initialized the Scheduler");

    DPRINTLN("\n-----OLD Main setup done-----\n");

    //////////////////

    DPRINTLN("\n-----NEW MAIN setup start-----\n");
    app.addModule(&wifiFtpServer);
    app.addModule(&restServer);
    app.setup();

    DPRINTLN("\n-----NEW MAIN setup done-----\n");
}


void loop() 
{   
    // connection.process();

    app.loop();     

    // bool idle = runner.execute();
    // if(idle)
    // {
    //     // put your main code here, to run repeatedly:
    //     //DPRINTLN("main loop start..");
    
        
    //     //DPRINTLN("main loop done");    
    //     yield();
    // }       
}



#endif