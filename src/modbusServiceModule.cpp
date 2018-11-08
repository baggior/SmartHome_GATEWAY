#include <Arduino.h>
#include <ModbusMaster.h>
#include <functional>

#include "modbusServiceModule.h"

// ---------------------------------------

static ModbusMaster node;

static std::function<void()> pfn_idle=NULL;
static std::function<void()> pfn_pre=NULL;
static std::function<void()> pfn_post=NULL;

static void idle() {
    if(pfn_idle)
    {
        pfn_idle();
    }
}

static void preTransmit() {
    if(pfn_pre)
    {
        pfn_pre();
    }
}

static void postTransmit() {
    if(pfn_post)
    {
        pfn_post();
    }
}
// ---------------------------------------

#define COIL_ADDR(n)   (0x0000 + n) ///< returns  discrete coil address

ModbusDataMemory::ModbusDataMemory()
: coils_buffer(), registers_buffer(), registers2_buffer()    
{
    this->clean();
}


ModbusServiceModule::ModbusServiceModule()
: Rs485ServiceModule("ModbusServiceModule", "seriale usata per protocollo modbus")
{

}

_Error ModbusServiceModule::setup() {
    const JsonObject &root = this->theApp->getConfig().getJsonObject("modbus");
    if(root.success()) 
    {
        return this->setup(root);
    }
    return _Disable;
}

_Error ModbusServiceModule::setup(const JsonObject &root) {

    _Error err = this->Rs485ServiceModule::setup(root);
    if ( err.errorCode != _NoError.errorCode) {
        return err;
    }
    
    //TODO config  
    const char * _protocolo = root["protocol"];
    const int _slave_id= root["slave_id"]; //16
    
    this->theApp->getLogger().printf(F("\t%s Modbus config: protocol: '%s', slave: %d,\n"), 
        this->getTitle().c_str(),
        REPLACE_NULL_STR(_protocolo), _slave_id );
    
    node.begin(_slave_id, * this->Rs485ServiceModule::getSerialAsStream());

    ::pfn_idle = std::bind(&Rs485ServiceModule::idle, this);    
    node.idle( ::idle );
    ::pfn_pre = std::bind(&Rs485ServiceModule::preTransmit, this);
    node.preTransmission( ::preTransmit );
    ::pfn_post = std::bind(&Rs485ServiceModule::postTransmit, this);
    node.postTransmission( ::postTransmit );

    this->theApp->getLogger().printf(F("\t%s Modbus setup done\n"), 
        this->getTitle().c_str());
    this->modbusDataMemory.clean();
    // this->buildDataMemory();

    return _NoError;
}

ModbusServiceModule::~ModbusServiceModule()
{
    this->shutdown();
}

void ModbusServiceModule::shutdown()
{
    node.clearTransmitBuffer();    
    node.clearResponseBuffer();
}

// void ModbusServiceModule::process() {

// //    if(timeelapsed) 
//    this->updateDataMemoryValues();

// //    this->publishItemValues();

// }

// void Modbus::publishItemValues() 
// {
//     // etl::vector<Item, 100>::iterator it = this->coils_buffer.begin();
//     for(const Item& item : coils_buffer) {

//         publish(item);
//     }
// }

void ModbusServiceModule::updateDataMemoryValues() 
{
    //COILS
    {
        uint16_t min_coil_address = this->modbusDataMemory.min_coil_address;
        uint16_t coil_count = this->modbusDataMemory.coils_buffer.size();
        DPRINTF("updateCoilsDataMemoryValues: min_address: %d, count: %d \n", min_coil_address, coil_count);
        if(coil_count>0 && min_coil_address>=0) 
        {
            uint8_t ret = node.readCoils(min_coil_address, coil_count);
            if(ModbusMaster::ku8MBSuccess==ret)
            {            
                uint8_t bitNum=0, buffNum=0, coilNum=0;
                // etl::vector<Item, 100>::iterator it = this->coils_buffer.begin();
                for(ModbusDataMemory::Item& item : this->modbusDataMemory.coils_buffer) 
                {
                    uint16_t buffer = node.getResponseBuffer(buffNum);
                    item.value = (buffer >> bitNum) & 0x0001 ;

                    coilNum++;
                    buffNum = coilNum / 16;
                    bitNum = coilNum % 16;
                }
            }
            else 
            {
                // TODO FAIL
                DPRINTF("ERROR> updateCoilsDataMemoryValues: %d", ret);
            }
        }
    }

    // REGS
    {
        uint16_t min_reg_address = this->modbusDataMemory.min_reg_address;
        uint16_t regs_count = this->modbusDataMemory.registers_buffer.size();
        DPRINTF("updateRegistersDataMemoryValues: min_address: %d, count: %d \n", min_reg_address, regs_count);
        if(regs_count>0 && min_reg_address>=0) 
        {
            uint8_t ret = node.readHoldingRegisters(min_reg_address, regs_count);
            if(ModbusMaster::ku8MBSuccess==ret)
            {            
                uint8_t buffNum=0;
                
                for(ModbusDataMemory::Item& item : this->modbusDataMemory.registers_buffer) 
                {
                    uint16_t buffer = node.getResponseBuffer(buffNum);
                    item.value = buffer ;
                    
                    buffNum++;               
                }
            }
            else 
            {
                // TODO FAIL
                DPRINTF("ERROR> updateRegistersDataMemoryValues: %d", ret);
            }
        }
    }

    // REGS2
    {
        uint16_t min_reg2_address = this->modbusDataMemory.min_reg2_address;
        uint16_t regs2_count = this->modbusDataMemory.registers2_buffer.size();
        DPRINTF("updateRegisters2DataMemoryValues: min_address: %d, count: %d \n", min_reg2_address, regs2_count);
        if(regs2_count>0 && min_reg2_address>=0) 
        {
            uint8_t ret = node.readHoldingRegisters(min_reg2_address, regs2_count);
            if(ModbusMaster::ku8MBSuccess==ret)
            {            
                uint8_t buffNum=0;
                
                for(ModbusDataMemory::Item& item : this->modbusDataMemory.registers2_buffer) 
                {
                    uint16_t buffer = node.getResponseBuffer(buffNum);
                    item.value = buffer ;
                    
                    buffNum++;               
                }
            }
            else 
            {
                // TODO FAIL
                DPRINTF("ERROR> updateRegisters2DataMemoryValues: %d", ret);
            }
        }
    }

    node.clearResponseBuffer();
}

// build from config file
void ModbusServiceModule::buildDataMemory(const JsonArray &modbusMemoryConfig) {
    this->modbusDataMemory.clean();
    size_t size = modbusMemoryConfig.size();

    // Walk the JsonArray efficiently
    for (JsonObject& item : modbusMemoryConfig) {
        const char* type = item["type"];
        int address = item["address"];
        int count = item["count"];
        if(type && count>0)
        {
            ModbusDataMemory::ItemType itemType = ModbusDataMemory::ItemType::UNKNOWN;
            
            if( String(type).equalsIgnoreCase("coil") )
            {
                itemType = ModbusDataMemory::ItemType::coil;
            }
            else if( String(type).equalsIgnoreCase("register") )
            {
                itemType = ModbusDataMemory::ItemType::coil;
            }
            else
            {
                //TODO log 
                continue;
            }

            int endaddress=address+count;
            for (int i=address; i<endaddress; ++i) 
            {
                this->modbusDataMemory.addItem(itemType, i, String(type) + "-" + String(i));
            }
        }
    }

    // {
    //     for (int i=1; i<79; ++i) 
    //     {
    //         this->modbusDataMemory.addItem(ModbusDataMemory::ItemType::coil, i, "coil-"+String(i));
    //     }
    // }

    // {
    //     for (int i=1; i<90; ++i) 
    //     {
    //         this->modbusDataMemory.addItem(ModbusDataMemory::ItemType::holding_register, i, "register-"+String(i));
    //     }
    // }

    // {
    //     for (int i=209; i<244; ++i) 
    //     {
    //         this->modbusDataMemory.addItem(ModbusDataMemory::ItemType::holding_register2, i, "register2-"+String(i));
    //     }
    // }
}

void ModbusDataMemory::clean() {
    this->min_coil_address= -1 ;
    this->coils_buffer.clear();
    this->min_reg_address = -1 ;
    this->registers_buffer.clear();
    this->min_reg2_address = -1 ;
    this->registers2_buffer.clear();
}

void ModbusDataMemory::addItem(ModbusDataMemory::ItemType type, uint16_t modbus_address, String name) {

    Item _item;
    _item.modbus_address = modbus_address;
    _item.name = name;
    _item.value = 0;

    if(type==ItemType::coil)
    {
        coils_buffer.push_back(_item);    
        if (min_coil_address==-1 || min_coil_address > _item.modbus_address)
        {
            min_coil_address = _item.modbus_address;
        }
    }
    else if(type==ItemType::holding_register)
    {        
        registers_buffer.push_back(_item);    
        if (min_reg_address==-1 || min_reg_address > _item.modbus_address)
        {
            min_reg_address = _item.modbus_address;
        }    
    }
    else if(type==ItemType::holding_register2)
    {        
        registers2_buffer.push_back(_item);    
        if (min_reg2_address==-1 || min_reg2_address > _item.modbus_address)
        {
            min_reg2_address = _item.modbus_address;
        }    
    }
}

