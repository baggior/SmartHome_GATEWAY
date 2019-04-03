#include "coreapi_mqttmodule.h"



//----------------------------------------------------
#include <IPAddress.h>
#include <WiFi.h> 
// #include <AsyncMqttClient.h>
#include <PubSubClient.h>

//----------------------------------------------------
#define LOOP_RECONNECT_ATTEMPT_MS 3000

// AsyncMqttClient mqttClient;
static WiFiClient espClient;
static PubSubClient mqttClient(espClient);


//----------------------------------------------------


// void connectToMqtt() {
//   Serial.println("Connecting to MQTT...");
//   mqttClient.connect();
// }


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

MqttModule::MqttModule()
: _BaseModule(ENUM_TO_STR(_CoreMqttModule), "Core Mqtt service module", true, Order_BeforeNormal, true )
{

}

MqttModule::~MqttModule()
{
    this->shutdown();
}


_Error MqttModule::setup() 
{  
  const JsonObject& root = this->theApp->getConfig().getJsonObject("mqtt");  
  if(!root.isNull()) 
  {
    return this->setup(root);
  }

  return _Disable;
}


_Error MqttModule::setup(const JsonObject &root) 
{
    bool on = root["enable"] | false ;
        
    // config  
    const char * _server_host = root["mqtt_server_host"];
    const int _server_port= root["mqtt_server_port"] | 1883;
    
    this->clientId = String(root["client_id"] | "shgw_*");    

    this->topicPrefix = String(root["topic_prefix"] | "");

    const char* _server_auth_username= root["server_auth"]["username"];
    const char* _server_auth_password= root["server_auth"]["password"];    

    this->theApp->getLogger().printf(F("\t%s Mqtt config: server_host: %s, port: %d, client_id: %s, topic_prefix: %s, server_auth_username: %s, server_auth_password: %s.\n"), 
        this->getTitle().c_str(),
        REPLACE_NULL_STR(_server_host), _server_port,
        this->clientId.c_str(), this->topicPrefix.c_str(),
        REPLACE_NULL_STR(_server_auth_username), REPLACE_NULL_STR(_server_auth_password)
    );
    
    if(! _server_host) {
        on = false;
    }

    if(on)
    {
        // IPAddress ipadd ( 198,41,30,241 ); //iot.eclipse.org
        // mqttClient.setServer(ipadd, _server_port);      
        mqttClient.setServer(_server_host, _server_port);      

        // bool ret = mqttPubsubClient.connect(_client_id, _server_auth_username, _server_auth_password);  
        // bool ret = mqttPubsubClient.connect(_client_id);  

        // DPRINTF("\tMqttServiceModule mqttPubsubClient.connect() ret: %d state: %d \n", ret, mqttPubsubClient.state());
        // if(!ret)
        // {
        //     return _Error(2, "impossibile connettersi al MQTT BROKER");
        // }

        // build client_id
        String chipId = baseutils::getChipId();
        this->clientId.replace("*", chipId);
        this->clientId.toLowerCase();
    }
    else 
    {
        return _Disable;
    }
    
    return _NoError;
}

void MqttModule::loop()
{
    if (!mqttClient.connected()) 
    {
        // Client not connected
        long now = millis();
        if (now - this->lastReconnectAttempt > LOOP_RECONNECT_ATTEMPT_MS) {
            this->lastReconnectAttempt = now;
            // Attempt to reconnect
            _Error ret = this->reconnect();
            if (ret == _NoError) {
                this->lastReconnectAttempt = 0;
                // DPRINTF(F(">\t%s CONNECTED: reconnect() return-> OK\n"), this->getTitle().c_str());
            } else {
                DPRINTF(F(">\t%s ERROR: reconnect(): %s (%d)\n"),
                    this->getTitle().c_str(), ret.message.c_str(), ret.errorCode );
            }
        }
    } 
    else 
    {
        // Client connected
        bool b = mqttClient.loop();
        // DPRINTF(F("MqttModule::loop() return-> %d\n"), b);
    }

}

void MqttModule::publish(String topicEnd, String value, bool retained) const
{
    if(_NoError == this->reconnect())
    {
        String topic = this->topicPrefix + topicEnd;

        bool ok = mqttClient.publish(topic.c_str(), value.c_str(), retained);
        if(!ok) {
            DPRINTF(F("\t> Error MqttModule::publish(%s)\n%s\n "), topic.c_str(), value.c_str());
        }
        
    }
}

void MqttModule::publish(String topicEnd, int value, bool retained) const
{
    String value_str = String(value);
    this->publish(topicEnd, value_str, retained);
}

void MqttModule::shutdown()
{   
    if(mqttClient.connected())
        mqttClient.disconnect();
}


_Error MqttModule::reconnect() const {
    // 
    if (!mqttClient.connected()) 
    {
        DPRINT(F("Attempting MQTT connection... "));
        
        // Attempt to connect
        if (mqttClient.connect(this->clientId.c_str())) {
            DPRINTF(F("MQTT: connected -> client_id: %s\n"), this->clientId.c_str());            
        } else {
            DPRINTF(F("MQTT: connection failed, state: %d \n"), 
                mqttClient.state());            

            return _Error(2, "impossibile connettersi al MQTT BROKER");
        }
    }
    return _NoError;
}