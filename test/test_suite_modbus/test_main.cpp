#include <Arduino.h>
#include <unity.h>
// #include "../../src/config.h"
#include <dbgutils.h>
#include <ModbusMaster.h>
#include <ConfigurableSoftwareSerial.h>

#ifdef UNIT_TEST

// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }

#define RX_PIN 16
#define TX_PIN 17
#define RS485_REDE_CONTROL 25

#define MODBUS_NODE_SLAVE_ID 16

// MODBUS RTU, 9600bps - 8 bit data - no parity - 1 stop bit
#define MODBUS_BAUD 9600
#define MODBUS_CONFIG SERIAL_8N1

//ConfigurableSoftwareSerial _serial(RX_PIN, TX_PIN);
HardwareSerial _serial(1);
ModbusMaster node;

#define COIL_ADDR(n)   (0x0000 + n) ///< returns  discrete coil address

static void preTransmission() {
    _serial.flush();
    digitalWrite(RS485_REDE_CONTROL, HIGH);
}

static void postTransmission() {
    _serial.flush();    
    digitalWrite(RS485_REDE_CONTROL, LOW);
}


void test_connection(void) {

    _serial.begin(MODBUS_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
//_serial.begin(MODBUS_BAUD);
//_serial.enableRx(true);
    // _serial.setDebugOutput(true);

    pinMode(RS485_REDE_CONTROL, OUTPUT);
    digitalWrite(RS485_REDE_CONTROL, LOW);

    node.begin(MODBUS_NODE_SLAVE_ID, _serial);
    node.clearResponseBuffer();
    node.clearTransmitBuffer();
    node.preTransmission( preTransmission );    
    node.postTransmission( postTransmission );
    
    // //compressor working COIL 18
    // uint8_t err = node.readCoils(COIL_ADDR(18), 1);
    // TEST_ASSERT_EQUAL_UINT8_MESSAGE(ModbusMaster::ku8MBSuccess, err, String(String("readCoil error: ")+err).c_str());
    
    // // do something with data if read is successful
    // uint16_t data[1];
    // if (err == node.ku8MBSuccess)
    // {
    //     for (uint8_t j = 0; j < 1; j++)
    //     {
    //         data[j] = node.getResponseBuffer(j);
    //     }
    // }
    
}


void test_read_coil(void) {
   
    //compressor working COIL 18
    //  TX: 10h 01h 00h 12h 00h 01h 5eh 8eh
    //  RX: 10h 01h 01h 00h 54h b4h

    uint8_t err = node.readCoils(COIL_ADDR(18), 1);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(ModbusMaster::ku8MBSuccess, err, String(String("readCoil error: ")+err).c_str());
    
    // do something with data if read is successful
    uint16_t data[1];
    if (err == node.ku8MBSuccess)
    {
        for (uint8_t j = 0; j < 1; j++)
        {
            data[j] = node.getResponseBuffer(j);
            DPRINTF(F("read data: %d\n"), data[j]);
        }
    }
    
}


void setup() {
    // NOTE!!! Wait for >2 secs
    // if board doesn't support software reset via Serial.DTR/RTS
    delay(2000);

    UNITY_BEGIN();    // IMPORTANT LINE!

    RUN_TEST(test_connection);
    
    // RUN_TEST(test_read_coil);
    // UNITY_END(); // stop unit testing
}

uint8_t i = 0;
uint8_t max = 105;

void loop() {
    if (i < max)
    {
      
        RUN_TEST(test_read_coil);
        delay(5000);
        i++;
    }
    else if (i == max) {
      UNITY_END(); // stop unit testing
    }
}

#endif