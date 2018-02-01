#include <Arduino.h>
#include <unity.h>
#include "../../src/config.h"
#include <ModbusMaster.h>
#include <ConfigurableSoftwareSerial.h>

#ifdef UNIT_TEST

// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }

#define RX_PIN RX
#define TX_PIN TX

void test_basic(void) {
    ConfigurableSoftwareSerial csserial(RX_PIN, TX_PIN);
    ModbusMaster node;

    node.begin(16, csserial);

    
    TEST_ASSERT_MESSAGE(true, "info messagge shown");
}




void setup() {
    // NOTE!!! Wait for >2 secs
    // if board doesn't support software reset via Serial.DTR/RTS
    delay(2000);

    UNITY_BEGIN();    // IMPORTANT LINE!

    RUN_TEST(test_basic);

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