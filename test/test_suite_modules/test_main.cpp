#define UNIT_TEST

#include <Arduino.h>

// #include "../../src/config.h"
// #include "wifiConnection.h"
// #include "modbus.h"
// #include "mqttclient.h"
// #include "strutils.h"

#include <coreapi.h>
#include <TaskScheduler.h>

#include "wifiRestServer.h"


#include <unity.h>

#define RS485_REDE_CONTROL 25

#ifdef UNIT_TEST
#include "../../src/wifiRestServer.cpp"

// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }

_Application app;
WifiRestServer restServerModule;

void test_setup_config(void) {
   
    app.addModule(&restServerModule);

    _Error ret = app.setup();
    TEST_ASSERT_MESSAGE(ret==_NoError, (String("setup error. code: ") + ret.errorCode+ String("("+ret.message+")") ).c_str());
}




void setup() {
    // NOTE!!! Wait for >2 secs
    // if board doesn't support software reset via Serial.DTR/RTS
    delay(2000);
    

    UNITY_BEGIN();    // IMPORTANT LINE!

    RUN_TEST(test_setup_config);

}

uint16_t i = 0;
uint16_t max_loops = 800;
void loop() {

    if (true)
    {
        delay(1000);
        
        

        i++;
    }
    else {
      UNITY_END(); // stop unit testing
    }
}




#endif