#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

class MqttClient 
{
public:
    MqttClient();

    void setup(Stream &dbgstream);

    void process();

    void publish(String name, int value);

    
private:
    bool enable;   
    
    Task taskReceiveCmd;
};


#endif