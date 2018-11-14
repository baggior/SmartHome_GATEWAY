#pragma once



#include <coreapi.h>


class MqttModule final : public _BaseModule  {

public:
    
    MqttModule();
    virtual ~MqttModule();

    inline void setTopicPrefix(String topicPrefix) { this->topicPrefix = topicPrefix; }

    void publish(String topicEnd, String value) const;
    void publish(String topicEnd, int value) const;

protected:
    virtual _Error setup() final override;
    _Error setup(const JsonObject &root);
    virtual void loop() override ; 
    virtual void shutdown() final override;

    _Error reconnect() const;

    String topicPrefix = "";
    String clientId;
};