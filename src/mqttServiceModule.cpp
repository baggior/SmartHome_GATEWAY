#include "mqttServiceModule.h"



//----------------------------------------------------
#include <IPAddress.h>
#include <WiFi.h> 
// #include <AsyncMqttClient.h>
#include <PubSubClient.h>

// AsyncMqttClient mqttClient;
static WiFiClient espClient;
static PubSubClient mqttPubsubClient(espClient);


//----------------------------------------------------


void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
//   mqttClient.connect();
}


// void onMqttConnect(bool sessionPresent) {
//   Serial.println("Connected to MQTT.");
//   Serial.print("Session present: ");
//   Serial.println(sessionPresent);
// }


// void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
//   Serial.println("Disconnected from MQTT.");

//   if (WiFi.isConnected()) {
//     connectToMqtt();
//   }
// }

//----------------------------------------------------

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


_Error MqttServiceModule::setup(const JsonObject &root) 
{
    bool on = false;
        
    // config  
    const char * _server_host = root["mqtt_server_host"];
    const int _server_port= root["mqtt_server_port"] || 1883;
    
    const char* _client_id= root["client_id"];    

    const char* _server_auth_username= root["server_auth"]["username"];
    const char* _server_auth_password= root["server_auth"]["password"];    
    if(!_server_auth_username) _server_auth_username="";
    if(!_server_auth_password) _server_auth_password="";

    this->theApp->getLogger().printf(F("\t%s Mqtt config: server_host:%s, port: %d, client_id: %s, server_auth_username: %s, server_auth_password: %s.\n"), 
        this->getTitle().c_str(),
        REPLACE_NULL_STR(_server_host), _server_port,
        REPLACE_NULL_STR(_client_id),
        REPLACE_NULL_STR(_server_auth_username), REPLACE_NULL_STR(_server_auth_password)
    );
    
    if(_server_host && _client_id) on = true;

    if(on)
    {
        mqttPubsubClient.setServer(_server_host, _server_port);      
        bool ret = mqttPubsubClient.connect(_client_id, _server_auth_username, _server_auth_password);  

        DPRINTF("\tMqttServiceModule mqttPubsubClient.connect() ret: %d state: %d \n", ret, mqttPubsubClient.state());
        if(!ret)
        {
            return _Error(2, "impossibile connettersi al MQTT BROKER");
        }
    }
    else 
    {
        return _Disable;
    }


    return _NoError;
}

void MqttServiceModule::publish(String topic, String value) const
{
    //TODO
}

void MqttServiceModule::publish(String topic, int value) const
{
    String value_str = String(value);
    this->publish(topic, value_str);
}

void MqttServiceModule::shutdown()
{   
    if(mqttPubsubClient.connected())
        mqttPubsubClient.disconnect();
}