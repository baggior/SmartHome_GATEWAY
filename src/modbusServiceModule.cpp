#include <Arduino.h>
#include <ModbusMaster.h>
#include <functional>

#include "modbusServiceModule.h"


#define min(a,b) ((a)<(b)?(a):(b))

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
: coils_buffer(), registers_buffer()
{
    this->clean();
}


ModbusServiceModule::ModbusServiceModule()
: Rs485ServiceModule("ModbusServiceModule", "seriale usata per protocollo modbus")
{

}

_Error ModbusServiceModule::setup() {
    const JsonObject &root = this->theApp->getConfig().getJsonObject("modbus");
    if(!root.isNull()) 
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
    
    // config  
    const char * _protocolo = root["protocol"];
    const int _slave_id= root["slave_id"]; //16
    
    this->theApp->getLogger().printf(("\t%s Modbus config: protocol: '%s', slave: %d,\n"), 
        this->getTitle().c_str(),
        REPLACE_NULL_STR(_protocolo), _slave_id );
    
    node.begin(_slave_id, * this->Rs485ServiceModule::getSerialAsStream());

    ::pfn_idle = std::bind(&Rs485ServiceModule::idle, this);    
    node.idle( ::idle );
    ::pfn_pre = std::bind(&Rs485ServiceModule::preTransmit, this);
    node.preTransmission( ::preTransmit );
    ::pfn_post = std::bind(&Rs485ServiceModule::postTransmit, this);
    node.postTransmission( ::postTransmit );

    this->theApp->getLogger().printf(("\t%s Modbus setup done\n"), 
        this->getTitle().c_str());
    
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

void ModbusServiceModule::updateDataMemoryValues(ModbusDataMemory& modbusDataMemory) const
{
    //COILS
    {
        const uint16_t min_coil_address = modbusDataMemory.min_coil_address;
        uint16_t coil_count = modbusDataMemory.coils_buffer.size();
        DPRINTF("updateCoilsDataMemoryValues: min_address: %d, count: %d \n", min_coil_address, coil_count);
        if(coil_count>0 && min_coil_address>=0) 
        {
            //max coil_count=min(ModbusDataMemory_MAX_MEMORY_ITEM_COUNT, 2000)
            if(coil_count > ModbusDataMemory_MAX_MEMORY_ITEM_COUNT)
            {
                coil_count = ModbusDataMemory_MAX_MEMORY_ITEM_COUNT;
                DPRINTF(F(">Warning: Coils count limited to %d elements\n"), ModbusDataMemory_MAX_MEMORY_ITEM_COUNT);
            }

            uint8_t ret = node.readCoils(min_coil_address, coil_count);
            if(ModbusMaster::ku8MBSuccess==ret)
            {            
                uint8_t bitNum=0, buffNum=0, coilNum=0;
                // etl::vector<Item, 100>::iterator it = this->coils_buffer.begin();
                for(ModbusDataMemory::Item& item : modbusDataMemory.coils_buffer) 
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
                DPRINTF("ERROR> updateCoilsDataMemoryValues: ModbusMaster::Error %d \n", ret);
            }
        }
    }

    // REGS
    {
        const uint16_t min_reg_address = modbusDataMemory.min_reg_address;
        uint16_t regs_count = modbusDataMemory.registers_buffer.size();
        DPRINTF("updateRegistersDataMemoryValues: min_address: %d, count: %d \n", min_reg_address, regs_count);
        if(regs_count>0 && min_reg_address>=0) 
        {
            //max regs_count=min(ModbusDataMemory_MAX_MEMORY_ITEM_COUNT, 125)
            if(regs_count > ModbusDataMemory_MAX_MEMORY_ITEM_COUNT)
            {
                regs_count = ModbusDataMemory_MAX_MEMORY_ITEM_COUNT;
                DPRINTF(F(">Warning: Register count limited to %d elements\n"), ModbusDataMemory_MAX_MEMORY_ITEM_COUNT);
            }

            uint16_t _current_min_reg_address = min_reg_address;
            uint16_t _current_regs_count = regs_count;
            while (_current_regs_count > 0)
            {                
                uint8_t ret = node.readHoldingRegisters(_current_min_reg_address, min(125, _current_regs_count) );   
                if(ModbusMaster::ku8MBSuccess==ret)
                {            
                    uint8_t buffNum=0;
                    
                    for(ModbusDataMemory::Item& item : modbusDataMemory.registers_buffer) 
                    {
                        uint16_t buffer = node.getResponseBuffer(buffNum);
                        item.value = buffer ;
                        
                        buffNum++;               
                    }
                }
                else 
                {
                    // TODO FAIL
                    DPRINTF("ERROR> updateRegistersDataMemoryValues: ModbusMaster::Error %d \n", ret);
                    break; //exit while loop
                }
                
                _current_min_reg_address += 125;
                _current_regs_count -= 125;             
            }

            // uint8_t ret = node.readHoldingRegisters(min_reg_address, regs_count);
            // if(ModbusMaster::ku8MBSuccess==ret)
            // {            
            //     uint8_t buffNum=0;
                
            //     for(ModbusDataMemory::Item& item : this->modbusDataMemory.registers_buffer) 
            //     {
            //         uint16_t buffer = node.getResponseBuffer(buffNum);
            //         item.value = buffer ;
                    
            //         buffNum++;               
            //     }
            // }
            // else 
            // {
            //     // TODO FAIL
            //     DPRINTF("ERROR> updateRegistersDataMemoryValues: %d \n", ret);
            // }
        }
    }

    // REGS2
    // {
    //     uint16_t min_reg2_address = this->modbusDataMemory.min_reg2_address;
    //     uint16_t regs2_count = this->modbusDataMemory.registers2_buffer.size();
    //     DPRINTF("updateRegisters2DataMemoryValues: min_address: %d, count: %d \n", min_reg2_address, regs2_count);
    //     if(regs2_count>0 && min_reg2_address>=0) 
    //     {
    //         uint8_t ret = node.readHoldingRegisters(min_reg2_address, regs2_count);
    //         if(ModbusMaster::ku8MBSuccess==ret)
    //         {            
    //             uint8_t buffNum=0;
                
    //             for(ModbusDataMemory::Item& item : this->modbusDataMemory.registers2_buffer) 
    //             {
    //                 uint16_t buffer = node.getResponseBuffer(buffNum);
    //                 item.value = buffer ;
                    
    //                 buffNum++;               
    //             }
    //         }
    //         else 
    //         {
    //             // TODO FAIL
    //             DPRINTF("ERROR> updateRegisters2DataMemoryValues: %d \n", ret);
    //         }
    //     }
    // }

    node.clearResponseBuffer();
}

// build from config file
void ModbusServiceModule::buildDataMemory(const JsonArray & modbusMemoryConfig, ModbusDataMemory & modbusDataMemory) const 
{
    size_t size = modbusMemoryConfig.size();

    // Walk the JsonArray efficiently
    for (const JsonObject& item : modbusMemoryConfig) 
    {
        String type = item["modbus_type"]      | String("register");
        String topic = item["topic"]    | type; //String("");

        int address = item["address"];
        int count = item["count"];

        if(type && count>0 && address>=0)
        {
            ModbusDataMemory::ModbusItemType ModbusItemType = ModbusDataMemory::ModbusItemType::UNKNOWN;
            
            if( type.equalsIgnoreCase("coil") )
            {
                ModbusItemType = ModbusDataMemory::ModbusItemType::coil;
            }
            else if( type.equalsIgnoreCase("register") )
            {
                ModbusItemType = ModbusDataMemory::ModbusItemType::holding_register;
            }
            else
            {
                // log 
                DPRINTF(F("ModbusMemory config Error: unknown type: %s \n"), type.c_str());
                continue;
            }

            int endaddress = address + count;
            for (int i=address; i<endaddress; ++i) 
            {                
                String name = topic;
                if(count>1) {
                    if(name.length() > 0) {
                        name += "_";
                    }
                    name += String(i);
                } 

                modbusDataMemory.addItem(ModbusItemType, i, name);
            }
        }
    }

    // {
    //     for (int i=1; i<79; ++i) 
    //     {
    //         this->modbusDataMemory.addItem(ModbusDataMemory::ModbusItemType::coil, i, "coil-"+String(i));
    //     }
    // }

    // {
    //     for (int i=1; i<90; ++i) 
    //     {
    //         this->modbusDataMemory.addItem(ModbusDataMemory::ModbusItemType::holding_register, i, "register-"+String(i));
    //     }
    // }

    // {
    //     for (int i=209; i<244; ++i) 
    //     {
    //         this->modbusDataMemory.addItem(ModbusDataMemory::ModbusItemType::holding_register2, i, "register2-"+String(i));
    //     }
    // }
}

void ModbusDataMemory::clean() {
    this->min_coil_address= 0 ;
    this->coils_buffer.clear();
    this->min_reg_address = 0 ;
    this->registers_buffer.clear();
    // this->min_reg2_address = -1 ;
    // this->registers2_buffer.clear();
}

void ModbusDataMemory::addItem(ModbusDataMemory::ModbusItemType type, uint16_t modbus_address, String name) {

    Item _item;
    _item.modbus_address = modbus_address;
    _item.name = name;
    _item.value = 0;

    if(type==ModbusItemType::coil)
    {
        coils_buffer.push_back(_item);    
        if (this->min_coil_address > _item.modbus_address)
        {
            this->min_coil_address = _item.modbus_address;
        }
    }
    else if(type==ModbusItemType::holding_register)
    {        
        registers_buffer.push_back(_item);    
        if (min_reg_address > _item.modbus_address)
        {
            this->min_reg_address = _item.modbus_address;
        }    
    }
    // else if(type==ModbusItemType::holding_register2)
    // {        
    //     registers2_buffer.push_back(_item);    
    //     if (min_reg2_address==-1 || min_reg2_address > _item.modbus_address)
    //     {
    //         min_reg2_address = _item.modbus_address;
    //     }    
    // }
}

static void print(const ModbusDataMemory::Item& item, JsonObject& obj, const char * modbus_type) 
{
    obj["name"] = item.name;
    obj["value"] = item.value;
    obj["address"] = item.modbus_address;
    obj["modbus_type"] = modbus_type;
}


void ModbusDataMemory::printDataMemory(Stream& out) const
{
    DynamicJsonDocument jsonBuffer(1024);
    const JsonArray arr = jsonBuffer.to<JsonArray>();

    for(const ModbusDataMemory::Item& item : this->coils_buffer) 
    {
        JsonObject obj = arr.createNestedObject();
        print(item, obj, "coil");
    }

    for(const ModbusDataMemory::Item& item : this->registers_buffer) 
    {
        JsonObject obj = arr.createNestedObject();
        print(item, obj, "register");
    }

    size_t size = serializeJsonPretty(arr, out);
}

