#ifndef modbusservicemodule_h
#define modbusservicemodule_h


#include "coreapi.h"
#include <vector.h>

#include "rs485ServiceModule.h"

#define ModbusDataMemory_MAX_MEMORY_ITEM_COUNT 255

class ModbusDataMemory
{
    friend class ModbusServiceModule;

public:
    struct Item {
        uint16_t modbus_address;
        String name;                //mqtt topic name
        uint16_t value;    
    };

    enum ModbusItemType {
        coil,
        holding_register,     
        // holding_register2,    

        UNKNOWN
    };
    
    ModbusDataMemory();
    virtual inline ~ModbusDataMemory() { this->clean(); }

    inline const etl::ivector<Item>& getCoils() const     { return this->coils_buffer; }
    inline const etl::ivector<Item>& getRegisters() const     { return this->registers_buffer; }
    // inline const etl::ivector<Item>& getRegisters2() const     { return this->registers2_buffer; }

    void clean();
    void addItem(ModbusItemType modbus_type, uint16_t modbus_address, String name);
    
    void printDataMemory(Stream& out) const;

private:

    etl::vector<Item, ModbusDataMemory_MAX_MEMORY_ITEM_COUNT> coils_buffer;
    uint16_t min_coil_address;
    etl::vector<Item, ModbusDataMemory_MAX_MEMORY_ITEM_COUNT> registers_buffer;
    uint16_t min_reg_address;    
    // etl::vector<Item, ModbusDataMemory_MAX_MEMORY_ITEM_COUNT> registers2_buffer;
    // uint16_t min_reg2_address=-1;    

};


//---------------------------------------------------------------


class ModbusServiceModule final : public Rs485ServiceModule
{

public:
    
    ModbusServiceModule();
    virtual ~ModbusServiceModule();

    // void process();

    // inline const ModbusDataMemory& getModbusDataMemory() const 
    // {
    //     return this->modbusDataMemory;
    // }

    void updateDataMemoryValues(ModbusDataMemory & modbusDataMemory) const;
    void buildDataMemory(const JsonArray & modbusMemoryConfig, ModbusDataMemory & modbusDataMemory) const;    
    

protected:
    virtual _Error setup() final override;
    virtual _Error setup(const JsonObject &root) final override;
    virtual void shutdown() final override;

private:   
    
    // ModbusDataMemory modbusDataMemory;

};



#endif
