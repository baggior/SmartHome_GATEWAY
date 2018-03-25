#include <Arduino.h>

#include "config.h"
#include "mqttclient.h"

#include <IPAddress.h>
#include <WiFi.h> 
// #include <AsyncMqttClient.h>
#include <PubSubClient.h>

#include "modbus.h"

//----------------------------------------------------

#define MQTT_LISTEN_TASK_INTERVAL_DEFAULT 1 //ms

extern Scheduler runner;

// AsyncMqttClient mqttClient;
static WiFiClient espClient;
static PubSubClient mqttPubsubClient(espClient);

static String cayenneTopicPrefix;

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

//-----------------------------------------------------
extern Modbus modbus;

void processModbusMemoryToCayenne(const MqttClient& mqttClient) 
{
    static int type=0;
    static int nextProcessedIndex = 0;
    static long lastProcessedTime = millis();

    long deltaTime = millis()-lastProcessedTime;
    if( deltaTime < 1000)
    {
        delay(1000-deltaTime);
    }

    const ModbusDataMemory& memory = modbus.getModbusDataMemory();

    const etl::ivector<ModbusDataMemory::Item>& buffer = (type%3)==0? memory.getRegisters2() : ( (type%3)==1? memory.getRegisters() : memory.getCoils() );        

    if (buffer.size() > nextProcessedIndex)
    {
        const ModbusDataMemory::Item& item = buffer.at(nextProcessedIndex);

        mqttClient.publish(item.name, item.value);

        nextProcessedIndex++;
    }
    else
    {
        nextProcessedIndex=0;
        type++;
    }
    
    lastProcessedTime = millis();
    
}

//-----------------------------------------------------

MqttClient::MqttClient() {

}

void MqttClient::setup(Stream &dbgstream) 
{
    JsonObject &root = config.getJsonRoot();
    int task_listen_interval = root["mqtt"]["task_listen_interval"];

    this->enable = root["mqtt"]["cayenne"]["enable"];

    String mqtt_server_host = root["mqtt"]["cayenne"]["mqtt_server_host"];
    uint16_t mqtt_server_port = root["mqtt"]["cayenne"]["mqtt_server_port"];
    String username = root["mqtt"]["cayenne"]["username_token"];
    String password = root["mqtt"]["cayenne"]["mqttpassword_token"];
    String clientID = root["mqtt"]["cayenne"]["client_id"];


    DPRINTF(F(">MQTT Cayenne Server SETUP: enable: %d, server: %s : %d, username: %s, password: %s, clientID: %s, task_listen_interval: %d \n"), 
        this->enable, mqtt_server_host.c_str(), mqtt_server_port,
        username.c_str(), password.c_str(), clientID.c_str(), task_listen_interval);

    if(!task_listen_interval) task_listen_interval=MQTT_LISTEN_TASK_INTERVAL_DEFAULT;

    if(this->enable)
    {
        /*
        mqttClient.onConnect(onMqttConnect);
        mqttClient.onDisconnect(onMqttDisconnect);
        //mqttClient.setCleanSession(true);
        mqttClient.setCredentials(username.c_str(), password.c_str());
        mqttClient.setClientId(clientID.c_str());
        // mqttClient.setServer(mqtt_server_host.c_str(), mqtt_server_port);

        mqttClient.setServer(IPAddress({50,16,133,238}), mqtt_server_port);
        connectToMqtt();
        */
        // mqttClient.setServer(mqtt_server_host.c_str(), mqtt_server_port);
        mqttPubsubClient.setServer({34,225,11,151}, mqtt_server_port);        
        
        bool ret = mqttPubsubClient.connect(clientID.c_str(), username.c_str(), password.c_str());

        DPRINTF("mqttPubsubClient.connect -> %d state: %d \n", ret, mqttPubsubClient.state());

        cayenneTopicPrefix = String("v1/") 
            + username + String("/things/")
            + clientID + String("/data/");

        this->setExtraProcessFn(std::bind(processModbusMemoryToCayenne, std::placeholders::_1));

        //TASK setting
        TaskCallback funct = std::bind(&MqttClient::process, this);
        taskProcess.set(1000 // task_listen_interval
            , TASK_FOREVER
            , funct
            );
        runner.addTask(taskProcess);
        taskProcess.enable();

    }

}


void MqttClient::process() {
    bool connected = mqttPubsubClient.loop();
    
    //uptime
    // this->publish("0", millis());

    //publish modbus memory data
    if(this->process_fn!=NULL)
    {
        this->process_fn(*this);
    }
}


void MqttClient::publish(String name, int value) const
{
    String topic = cayenneTopicPrefix + name;
    String value_str = String(value);

    bool connected = mqttPubsubClient.publish( topic.c_str(), value_str.c_str());

    DPRINTF("published topic: %s -> [%s]\n", topic.c_str(), value_str.c_str());    
}

