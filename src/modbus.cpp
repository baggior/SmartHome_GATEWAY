#include <Arduino.h>
#include <ModbusMaster.h>

#include "config.h"
#include "modbus.h"


HardwareSerial _serial(1);
ModbusMaster node;

#define COIL_ADDR(n)   (0x0000 + n) ///< returns  discrete coil address


Modbus::Modbus() : 
coils_buffer(), register_buffer()  
{

}

void Modbus::setup(Stream &dbgstream) {

    const int MODBUS_BAUD= 9600;
    const int MODBUS_NODE_SLAVE_ID= 16;
    bool debug=true; //TODO

    _serial.begin(MODBUS_BAUD, SERIAL_8N1, MODBUS_RX_PIN, MODBUS_TX_PIN);
    _serial.setDebugOutput(debug);
    node.begin(MODBUS_NODE_SLAVE_ID, _serial);
    DPRINTLN(">Modbus setup done");
    buildDataMemory();
}


void Modbus::process() {

//    if(timeelapsed) 
   this->updateDataMemoryValues();

   this->publishItemValues();

}

void Modbus::publishItemValues() 
{
    // etl::vector<Item, 100>::iterator it = this->coils_buffer.begin();
    for(const Item& item : coils_buffer) {

        publish(item);
    }
}

void Modbus::updateDataMemoryValues() 
{
    uint8_t ret = node.readCoils(min_coil_address, coil_count);
    if(ModbusMaster::ku8MBSuccess==ret)
    {
        // OK
        // uint16_t data[coil_count];
        
        // for (uint8_t j = 0; j < 1; j++) 
        // {
        //     data[j] = node.getResponseBuffer(j);
        // }

        uint8_t j=0;
        // etl::vector<Item, 100>::iterator it = this->coils_buffer.begin();
        for(Item& item : coils_buffer) {
        // while(it!=this->coils_buffer.end()) {
            // Item item = *it;            
            item.value = node.getResponseBuffer(j);
            j++;
        }
    }
    else 
    {
        // TODO FAIL

    }

    node.clearResponseBuffer();
}


void Modbus::buildDataMemory() {
    
    this->addItem(ItemType::coil, 18, "18name");

}

void Modbus::addItem(ItemType type, uint16_t modbus_address, String name) {

    Item coilItem ({
        .modbus_address = modbus_address,
        .name = name,
        .value = 0
    });

    coils_buffer.push_back(coilItem);
    coil_count++;
    if(min_coil_address > coilItem.modbus_address)
    {
        min_coil_address = coilItem.modbus_address;
    }

}


void Modbus::publish(const Item& item)
{

}