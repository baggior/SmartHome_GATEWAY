#include <Arduino.h>
#include <unity.h>
#ifdef ESP32
#include <SPIFFS.h>
#endif
#include "../../src/config.h"

#ifdef UNIT_TEST

// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }


void test_system_info(void) {
    baseutils::printBoardInfo(DEBUG_OUTPUT);
    
    TEST_ASSERT_MESSAGE(true, "info messagge shown");
}

//this test format spiffs
void test_spiffs() {
    TEST_IGNORE_MESSAGE(" ignore this test: format spiffs");
    
    bool b = SPIFFS.begin();
    TEST_ASSERT_MESSAGE(b, "SPIFFS begin");
    b =SPIFFS.format();
    TEST_ASSERT_MESSAGE(b, "SPIFFS format");
    SPIFFS.end();
    
}


void setup() {
    // NOTE!!! Wait for >2 secs
    // if board doesn't support software reset via Serial.DTR/RTS
    delay(2000);

    UNITY_BEGIN();    // IMPORTANT LINE!

    RUN_TEST(test_system_info);
    RUN_TEST(test_spiffs);

    UNITY_END(); // stop unit testing
}

uint8_t i = 0;
uint8_t max_blinks = 5;

void loop() {
    // if (i < max_blinks)
    // {
    //     RUN_TEST(test_led_state_high);
    //     delay(500);
    //     RUN_TEST(test_led_state_low);
    //     delay(500);
    //     i++;
    // }
    // else if (i == max_blinks) {
    //   UNITY_END(); // stop unit testing
    // }
}

#endif