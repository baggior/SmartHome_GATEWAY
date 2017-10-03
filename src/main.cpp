
#include "config.h"

#include "cliCommands.h"
#include "wifiConnection.h"
#include "wifiRestServer.h"
#include "wifiTelnetServer.h"
#include "rs485.h"


PRAGMA_MESSAGE (VAR_NAME_VALUE(ARDUINO))
PRAGMA_MESSAGE (VAR_NAME_VALUE(ESP8266))
PRAGMA_MESSAGE (VAR_NAME_VALUE(__AVR__))
PRAGMA_MESSAGE (VAR_NAME_VALUE(MY_DEBUG))
PRAGMA_MESSAGE (VAR_NAME_VALUE(DEBUG_OUTPUT))
PRAGMA_MESSAGE (VAR_NAME_VALUE(DEBUG_ESP_PORT))
PRAGMA_MESSAGE (VAR_NAME_VALUE(DEBUG_ESP_WIFI))

Config config;
WifiRestServer restServer;
WifiTelnetServer wifiTelnetServer;
CliCommands clicommands;
Rs485 rs485;


void setup() {
    // put your setup code here, to run once:    

    pinMode(BUILTIN_LED, OUTPUT);     
    digitalWrite(BUILTIN_LED, LOW); //LED ON

    config.load();

    Serial.begin(9600);
    DPRINTLN("main setup start");
    
    config.print(Serial);
    
    wifiManagerOpenConnection(Serial);

    digitalWrite(BUILTIN_LED, HIGH); //LED OFF           

    restServer.setup(Serial);
    wifiTelnetServer.setup(Serial);    
    rs485.setup(Serial);
    
    DPRINTLN("main seup done");
}

void loop() {

    String CMD_received;

    digitalWrite(BUILTIN_LED, LOW);   // turn the LED on

    // put your main code here, to run repeatedly:
    DPRINTLN("main loop start..");

    restServer.process();
    DPRINTLN("..processed REST");

    if(wifiTelnetServer.process())
    {
        CMD_received = wifiTelnetServer.getLastCommandReceived();
        DPRINTLN("..processed Telnet: received [" + CMD_received +"]");       
    }    

    // clicommands.processCli(client);
    // DPRINTLN("..processed Commands over Telnet client");

    //String CMD="020300CA0001";  //:020300CA000130
    String CMD_response = rs485.process(CMD_received);
    DPRINTLN("..processed rs485 sent [" + CMD_received +"] received [" + CMD_response +"]");

    if(CMD_response.length()>0)
    {
        wifiTelnetServer.process(CMD_response);
    }

    DPRINTLN("main loop done");

    delay(100);
    digitalWrite(BUILTIN_LED, HIGH);    // turn the LED off 


    delay(5000);
}

