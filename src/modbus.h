#ifndef modbus_h
#define modbus_h


#include <vector.h>

#include "rs485.h"

#define ModbusDataMemory_max_buff_size 255

class ModbusDataMemory
{
    friend class Modbus;
public:
    struct Item {
        uint16_t modbus_address;
        String name;
        uint16_t value;    
    };

    enum ItemType {
        coil,
        holding_register,     
        holding_register2,    

        UNKNOWN
    };
    
    inline const etl::ivector<Item>& getCoils() const     { return this->coils_buffer; }
    inline const etl::ivector<Item>& getRegisters() const     { return this->registers_buffer; }
    inline const etl::ivector<Item>& getRegisters2() const     { return this->registers2_buffer; }
private:
    
    ModbusDataMemory();
    void clean();
    void addItem(ItemType type, uint16_t modbus_address, String name);

    etl::vector<Item, ModbusDataMemory_max_buff_size> coils_buffer;
    uint16_t min_coil_address=-1;
    etl::vector<Item, ModbusDataMemory_max_buff_size> registers_buffer;
    uint16_t min_reg_address=-1;    
    etl::vector<Item, ModbusDataMemory_max_buff_size> registers2_buffer;
    uint16_t min_reg2_address=-1;    

};


//---------------------------------------------------------------


class Modbus 
{

public:
    
    Modbus();
    int setup(Stream &dbgstream);

    void process();

    inline const ModbusDataMemory& getModbusDataMemory() const 
    {
        return this->modbusDataMemory;
    }

private:   

    void buildDataMemory();
    
    void updateDataMemoryValues();
    
    ModbusDataMemory modbusDataMemory;

    bool enable;   
    Stream * p_dbgstream=NULL;
    Rs485 rs485;

};



#endif
