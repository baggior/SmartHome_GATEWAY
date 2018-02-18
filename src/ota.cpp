#include "config.h"

#include "ota.h"


#define OTA_LISTEN_TASK_INTERVAL_DEFAULT 1000 //ms

extern Scheduler runner;

void Ota::begin(Stream &stream)
{
    this->dbgstream = &stream;

    //TODO: configuration
    int task_listen_interval = 0;
    
    if(!task_listen_interval) task_listen_interval=OTA_LISTEN_TASK_INTERVAL_DEFAULT;

    // Port defaults to 8266
    // ArduinoOTA.setPort(8266);

    // Hostname defaults to esp8266-[ChipID]
    // ArduinoOTA.setHostname("myesp8266");

    // No authentication by default
    // ArduinoOTA.setPassword((const char *)"123");

    ArduinoOTA.onStart([this]() {        
        this->dbgstream->println("Ota Start");
    });
    ArduinoOTA.onEnd([this]() {
        this->dbgstream->println("Ota End");
    });
    ArduinoOTA.onProgress([this](unsigned int progress, unsigned int total) {
        this->dbgstream->printf("Ota Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([this](ota_error_t error) {
        this->dbgstream->printf("Ota Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
            this->dbgstream->println("Ota Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
            this->dbgstream->println("Ota Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
            this->dbgstream->println("Ota Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
            this->dbgstream->println("Ota Receive Failed");
        else if (error == OTA_END_ERROR)
            this->dbgstream->println("Ota End Failed");
    });

    ArduinoOTA.begin();
    this->dbgstream->print("Ota Ready on ");
    this->dbgstream->print("IP: ");
    this->dbgstream->println(WiFi.localIP());


    if(enable)
    {
        //TASK setting
        TaskCallback funct = std::bind(&Ota::process, this);
        taskprocessOta.set(task_listen_interval
            , TASK_FOREVER
            , funct);
        runner.addTask(taskprocessOta);
        taskprocessOta.enable();
    }
}

void Ota::process()
{
    if(enable)
    {        
        ArduinoOTA.handle();
    }
}