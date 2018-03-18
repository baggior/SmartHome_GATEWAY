#include <Arduino.h>
#include <unity.h>
#include "../../src/config.h"
#include "wifiConnection.h"
#include "rs485.h"

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
extern Rs485 rs485;


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
void test_rs485_setup(void) {
    JsonObject &root = config.getJsonRoot()["modbus"]["rs485"];

    TEST_ASSERT_MESSAGE(root.success(), "error parsing JSON file. [modbus.rs485]");

    rs485.setup(Serial, root);
    
}

void test_rs485_binary(void) {
    
    //10 01 00 00 00 50 3F 77
    uint8_t data[] = {0x10, 0x01, 0x00, 0x00, 0x00, 0x50, 0x3F, 0x77};

    Rs485::BINARY_BUFFER_T command;
    command.insert(command.begin(), &data[0], &data[sizeof(data)]);

    uint8_t* p_vdata = command.data();    
    String str = baseutils::byteToHexString(p_vdata, command.size(), ".");
    Serial.println("command: "+ str);

    Rs485::BINARY_BUFFER_T ret = rs485.sendMasterCommand(command,100);
    // Response: 10 01 0A 02 24 10 00 00 00 00 00 00 10 EB 6B 

    p_vdata = ret.data();  
    str = baseutils::byteToHexString(p_vdata, ret.size(), ".");
    Serial.println("response: "+ str);

    TEST_ASSERT_MESSAGE(str.length()>0, "empty response");
}

void setup() {
    // NOTE!!! Wait for >2 secs
    // if board doesn't support software reset via Serial.DTR/RTS
    delay(2000);
    Serial.setDebugOutput(true);

    UNITY_BEGIN();    // IMPORTANT LINE!

    RUN_TEST(test_setup_config);

    // RUN_TEST(test_setup_wifi);

    RUN_TEST(test_rs485_setup);
    
}

uint16_t i = 0;
uint16_t max_loops = 3;
void loop() {

    if (i < max_loops)
    {
        delay(2000);
        RUN_TEST(test_rs485_binary);
    
        i++;
    }
    else {
      UNITY_END(); // stop unit testing
    }
}




#endif