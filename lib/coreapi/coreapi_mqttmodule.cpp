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
: _BaseModule(ENUM_TO_STR(_CoreMqttModule), "Core Mqtt service module", true, Order_BeforeNormal )
{

}

MqttModule::~MqttModule()
{
    this->shutdown();
}


_Error MqttModule::setup() 
{  
  const JsonObject& root = this->theApp->getConfig().getJsonObject("mqtt");  
  if(root.success()) 
  {
    return this->setup(root);
  }

  return _Disable;
}


_Error MqttModule::setup(const JsonObject &root) 
{
    bool on = false;
        
    // config  
    const char * _server_host = root["mqtt_server_host"];
    const int _server_port= root["mqtt_server_port"] | 1883;
    
    String _client_id= root["client_id"] | "";    
    if(_client_id.length()==0)
    {
        this->clientId = String(_client_id);
    }
    else
    {
        // Create a random client ID
        this->clientId = this->getTitle();
        this->clientId += String(random(0xffff), HEX);
    }

    const char* _server_auth_username= root["server_auth"]["username"];
    const char* _server_auth_password= root["server_auth"]["password"];    

    this->theApp->getLogger().printf(F("\t%s Mqtt config: server_host:%s, port: %d, client_id: %s, server_auth_username: %s, server_auth_password: %s.\n"), 
        this->getTitle().c_str(),
        REPLACE_NULL_STR(_server_host), _server_port,
        this->clientId.c_str(),
        REPLACE_NULL_STR(_server_auth_username), REPLACE_NULL_STR(_server_auth_password)
    );
    
    if(_server_host && _client_id) {
        on = true;
    }

    if(on)
    {
        IPAddress ipadd ( 198,41,30,241 ); //iot.eclipse.org
        mqttClient.setServer(ipadd, _server_port);      

        // bool ret = mqttPubsubClient.connect(_client_id, _server_auth_username, _server_auth_password);  
        // bool ret = mqttPubsubClient.connect(_client_id);  

        // DPRINTF("\tMqttServiceModule mqttPubsubClient.connect() ret: %d state: %d \n", ret, mqttPubsubClient.state());
        // if(!ret)
        // {
        //     return _Error(2, "impossibile connettersi al MQTT BROKER");
        // }
    }
    else 
    {
        return _Disable;
    }


    return _NoError;
}

void MqttModule::loop()
{
    static long lastReconnectAttempt = 0;
    if (!mqttClient.connected()) 
    {
        long now = millis();
        if (now - lastReconnectAttempt > LOOP_RECONNECT_ATTEMPT_MS) {
            lastReconnectAttempt = now;
            // Attempt to reconnect
            _Error ret = this->reconnect();
            if (ret == this->reconnect()) {
                lastReconnectAttempt = 0;
            } else {
                DPRINTF(F(">\t%s ERROR: reconnect(): %s (%d)"),
                    this->getTitle(), ret.message.c_str(), ret.errorCode );
            }
        }
    } 
    else 
    {
        // Client connected
        bool b = mqttClient.loop();
    }

}

void MqttModule::publish(String topicEnd, String value) const
{
    if(_NoError == this->reconnect())
    {
        //TODO

    }
}

void MqttModule::publish(String topicEnd, int value) const
{
    String value_str = String(value);
    this->publish(topicEnd, value_str);
}

void MqttModule::shutdown()
{   
    if(mqttClient.connected())
        mqttClient.disconnect();
}


_Error MqttModule::reconnect() const {
    // Loop until we're reconnected
    if (!mqttClient.connected()) 
    {
        DPRINT(F("Attempting MQTT connection... "));
        
        // Attempt to connect
        if (mqttClient.connect(this->clientId.c_str())) {
            DPRINTLN(F("MQTT: connected"));            
        } else {
            DPRINTF(F("failed, state: %d \n"), 
                mqttClient.state());            

            return _Error(2, "impossibile connettersi al MQTT BROKER");
        }
    }
    return _NoError;
}