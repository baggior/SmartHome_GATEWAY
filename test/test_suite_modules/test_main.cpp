#define UNIT_TEST

#include <Arduino.h>

// #include "../../src/config.h"
// #include "wifiConnection.h"
// #include "modbus.h"
// #include "mqttclient.h"
// #include "strutils.h"

#include <coreapi.h>
#include <TaskScheduler.h>

#include "wifiRestServerModule.h"
#include "coreapi_ftpmodule.h"


#include <unity.h>

#define RS485_REDE_CONTROL 25

#ifdef UNIT_TEST
#include "../../src/wifiRestServerModule.cpp"
// #include "../../lib/coreapi/coreapi_ftpmodule.cpp"

// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }

_Application app;
WifiRestServerModule restServerModule;
WifiFtpServerModule wifiFtpServerModule;

void test_setup_config(void) {
   
    app.addModule(&wifiFtpServerModule);
    app.addModule(&restServerModule);

    app.setIdleLoopCallback( []() -> void  {app.getLogger().printf(".");});
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
uint16_t max_loops = 20;
void loop() {

    if (i<max_loops)
    {
        delay(1000);
        
        app.loop();

        i++;
    }
    else {
      UNITY_END(); // stop unit testing
    }
}




#endif