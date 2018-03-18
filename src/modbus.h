#ifndef modbus_h
#define modbus_h


// #include "LinkedList.h"
#include <vector.h>
// #include <container.h>

class Modbus 
{

public:
    Modbus();
    void setup(Stream &dbgstream);

    void process();


private:
    struct Item {
        uint16_t modbus_address;
        String name;
        uint16_t value;    
    };

    enum ItemType {
        coil,
        holding,
        input,
        discrete,
        UNKNOWN
    };

    void buildDataMemory();
    void addItem(ItemType type, uint16_t modbus_address, String name);
    void updateDataMemoryValues();
    void publishItemValues();
    void publish(const Item& item);

    etl::vector<Item, 100> coils_buffer;
    uint16_t min_coil_address=0;
    uint16_t coil_count=0;

    etl::vector<Item, 100> register_buffer;
    uint16_t min_reg_address=0;
    uint16_t reg_count=0;
    bool enable;   

};



#endif
