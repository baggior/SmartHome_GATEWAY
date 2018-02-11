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
    // delay(3000);

// //    WiFi.setHostname("espressif");
// 	MDNS.begin("espressif");
//     delay(2000);
// 	MDNS.addService("_osc","_udp",4500);
// 	MDNS.addServiceTxt("_osc","_udp","does","it");
// 	// MDNS.addServiceTxt("_osc","_udp","work","work");
// 	// MDNS.addServiceTxt("_osc","_udp","yes","yes");
}

void test_mdns(void) {


MDNS.addServiceTxt("_osc","_udp","does","it");
    // QueryResult results = connection.query(String("osc"), String("udp"));
    // TEST_ASSERT_EQUAL_UINT_MESSAGE(4500,results.port, "MDNS service not found");

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
        RUN_TEST(test_mdns);
    
        i++;
    }
    else {
      UNITY_END(); // stop unit testing
    }
}




#endif