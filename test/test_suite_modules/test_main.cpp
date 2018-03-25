#include <Arduino.h>
#include <unity.h>
#include "../../src/config.h"
#include "wifiConnection.h"
#include "modbus.h"
#include "mqttclient.h"
#include "strutils.h"

#define RS485_REDE_CONTROL 25

#ifdef UNIT_TEST

// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }

extern Config config;
extern WiFiConnection connection;
extern Modbus modbus;
extern MqttClient mqtt;

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

void test_setup_mqtt(void) {
    mqtt.setup(Serial);
    
}

void test_modbus_setup(void) {
    JsonObject &root = config.getJsonRoot();

    TEST_ASSERT_MESSAGE(root.success(), "error parsing JSON file. [modbus]");

    int ret = modbus.setup(Serial);
    TEST_ASSERT_MESSAGE(ret==SUCCESS_OK, "error modbus.setup");
}

void test_modbus_process(void) {
    modbus.process();
}

void test_mqtt_process() {
    mqtt.process();
}

void setup() {
    // NOTE!!! Wait for >2 secs
    // if board doesn't support software reset via Serial.DTR/RTS
    delay(2000);
    Serial.setDebugOutput(true);

    UNITY_BEGIN();    // IMPORTANT LINE!

    RUN_TEST(test_setup_config);
    RUN_TEST(test_setup_wifi);
    RUN_TEST(test_setup_mqtt);


    RUN_TEST(test_modbus_setup);
    
    RUN_TEST(test_modbus_process);
}

uint16_t i = 0;
uint16_t max_loops = 800;
void loop() {

    if (true)
    {
        delay(1000);
        
        RUN_TEST(test_mqtt_process);

        i++;
    }
    else {
      UNITY_END(); // stop unit testing
    }
}




#endif