#include "mqttServiceModule.h"

MqttServiceModule::MqttServiceModule() 
: _ServiceModule("MqttServiceModule", "")
{

}

MqttServiceModule::~MqttServiceModule()
{
    this->shutdown();
}


_Error MqttServiceModule::setup() 
{  
  const JsonObject& root = this->theApp->getConfig().getJsonObject("mqtt");  
  if(root.success()) 
  {
    return this->setup(root);
  }

  return _Disable;
}


_Error MqttServiceModule::setup(const JsonObject &root) {
        
    //TODO config  
    const char * _protocolo = root["protocol"];
    const int _slave_id= root["slave_id"]; //16
    
    this->theApp->getLogger().printf(F("\t%s Mqtt config: todo.. \n"), 
        this->getTitle().c_str());
    
    return _NoError;
}

void MqttServiceModule::shutdown()
{
  
}