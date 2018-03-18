#include <Arduino.h>
#include <unity.h>
// #include "../../src/config.h"




#ifdef UNIT_TEST

// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }


void test_connection(void) {

    String username = "36cc5bb0-266e-11e7-a4a6-237007b7399c";
    String password = "cd82e8f61ca283c10209db705d6c10582d619693";
    String clientID = "e6fd0b70-b56f-11e7-bd7e-3193fab997a8";

    CAYENNE_LOG("before Cayenne.begin()..");
    Cayenne.begin(username.c_str(), password.c_str(), clientID.c_str() 
        ,"cent15c", "elettronica");
    CAYENNE_LOG("Cayenne.begin called.");

}



void setup() {
    // NOTE!!! Wait for >2 secs
    // if board doesn't support software reset via Serial.DTR/RTS
    delay(2000);

    UNITY_BEGIN();    // IMPORTANT LINE!

    RUN_TEST(test_connection);
    
    // UNITY_END(); // stop unit testing
}

uint8_t i = 0;
uint8_t max = 105;

void loop() {
    if (i < max)
    {      
        CAYENNE_LOG("LOOP calling...");
        Cayenne.loop();
        CAYENNE_LOG("LOOP called.");

        delay(5000);
        i++;
    }
    else if (i == max) {
      UNITY_END(); // stop unit testing
    }
}



CAYENNE_CONNECTED()
{
    CAYENNE_LOG("CAYENNE CONNECTED");
}
CAYENNE_OUT_DEFAULT() 
{
    // time elapsed since boot
    CAYENNE_LOG("OUT 0 called.");
    Cayenne.virtualWrite(0, millis());
}


#endif