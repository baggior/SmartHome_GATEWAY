#include <Arduino.h>
#include <unity.h>
#include "../../src/config.h"
#include "wifiConnection.h"

#ifdef UNIT_TEST

// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }

#define RX_PIN RX
#define TX_PIN TX

extern Config config;
extern WiFiConnection connection;



void test_setup_config(void) {
   
    int ret =config.load(true);
    TEST_ASSERT_MESSAGE(ret>=0, "config loaded");

    TEST_ASSERT_MESSAGE(config.getJsonRoot().success(), "error parsing JSON file");
    /*
    String deviceInfo = config.getDeviceInfoString();
    Serial_printf(Serial, deviceInfo.c_str());
    TEST_ASSERT_MESSAGE(deviceInfo.length()>=0, "config loaded");
    */
}

WiFiServer test_tcpserver(80);

void test_setup_wifi(void) {
    connection.setup(Serial);
    
    test_tcpserver.begin();
    
    delay(1000);
    connection.announceTheDevice();

}

void setup() {
    // NOTE!!! Wait for >2 secs
    // if board doesn't support software reset via Serial.DTR/RTS
    delay(2000);
    Serial.setDebugOutput(true);

    UNITY_BEGIN();    // IMPORTANT LINE!

    RUN_TEST(test_setup_config);

    RUN_TEST(test_setup_wifi);

    
}

uint16_t i = 0;
uint16_t max_loops = 10;
void loop() {

    if (i < max_loops)
    {
        delay(2000);
        
    
        i++;
    }
    else {
      UNITY_END(); // stop unit testing
    }
}




#endif