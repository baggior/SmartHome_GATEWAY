#pragma once



#include <coreapi.h>


class MqttServiceModule final : public _ServiceModule  {

public:
    
    MqttServiceModule();
    virtual ~MqttServiceModule();

    inline void setTopicPrefix(String topicPrefix) { this->topicPrefix = topicPrefix; }

    void publish(String topicEnd, String value) const;
    void publish(String topicEnd, int value) const;

protected:
    virtual _Error setup() final override;
    virtual _Error setup(const JsonObject &root) final override;
    virtual void shutdown() final override;

    String topicPrefix = "";
};