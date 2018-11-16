#pragma once



#include <coreapi.h>


class MqttModule final : public _BaseModule  {

public:
    
    MqttModule();
    virtual ~MqttModule();

    // inline void setTopicPrefix(String topicPrefix) { this->topicPrefix = topicPrefix; }

    void publish(String topicEnd, String value, bool retained=false) const;
    void publish(String topicEnd, int value, bool retained=false) const;

protected:
    virtual _Error setup() final override;
    _Error setup(const JsonObject &root);
    virtual void loop() override ; 
    virtual void shutdown() final override;

//TODO
    // subscribe..
    // void callbackFn(const char* topic, uint8_t* payload, unsigned int length) const;

    _Error reconnect() const;

    String topicPrefix = "";
    String clientId;

private:
    long lastReconnectAttempt=0;
};