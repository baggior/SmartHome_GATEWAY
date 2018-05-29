#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <functional>

class MqttClient 
{
public:
    typedef std::function<void(const MqttClient&)> process_fn_t;


    MqttClient();

    void setup(Stream &dbgstream);

    void process();

    void publish(String name, int value) const;

private:

    inline void setExtraProcessFn(process_fn_t process_fn) 
    {
        this->process_fn = process_fn;
    }
    
    bool enable;   
    process_fn_t process_fn = NULL;
    
    Task taskProcess;
};


#endif