#pragma once



#include <coreapi.h>


class MqttServiceModule final : public _ServiceModule  {

public:
    
    MqttServiceModule();
    virtual ~MqttServiceModule();

protected:
    virtual _Error setup() final override;
    virtual _Error setup(const JsonObject &root) final override;
    virtual void shutdown() final override;

    
};