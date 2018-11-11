#pragma once



#include <coreapi.h>


class MqttServiceModule final : public _ServiceModule  {

public:
    
    MqttServiceModule();
    virtual ~MqttServiceModule();

    void publish(String topic, String value) const;
    void publish(String topic, int value) const;

protected:
    virtual _Error setup() final override;
    virtual _Error setup(const JsonObject &root) final override;
    virtual void shutdown() final override;


};