#include <Arduino.h>

#include <HTTPClient.h>
#include <ArduinoJson.h>

extern "C"
{
#include "crypto/base64.h"
}

#include "rs485MonitorModule.h"

#define DEFAULT_TIME_PACKET_END_MS 100 // TODO: config time to separate monitored packets

static _Error postToHttpServer(String url, uint8_t *const frameBuffer, const uint16_t frameBuffer_len, bool ascii=false);

Rs485MonitorModule::Rs485MonitorModule()
    : _BaseModule("Rs485MonitorModule", "Rs485 Monitor Protocol", true, Order_BeforeNormal, true)
{
}
Rs485MonitorModule::~Rs485MonitorModule()
{
    this->shutdown();
}

_Error Rs485MonitorModule::setup()
{
    const JsonObject &root = this->theApp->getConfig().getJsonObject("rs485_monitor");
    if (root.isNull())
    {
        return _Disable;
    }

    this->p_rs485 = this->theApp->getModule<Rs485ServiceModule>("_Rs485Service");
    if (!this->p_rs485)
    {
        return _Error(2, "Rs485MonitorModule Error: servizio _Rs485Service non esistente");
    }
    if (!this->p_rs485->isEnabled())
    {
        return _Error(3, "Rs485MonitorModule Error: servizio _Rs485Service esistente ma disabilitato");
    }

    bool on = root["enable"] | false;
    this->modbus_ascii = root["modbus_ascii"] | false;
    this->monitor_post_rest_url = (const char *)root["post_rest_url"];
    this->packet_separation_time_ms = root["packet_separation_time_ms"] | DEFAULT_TIME_PACKET_END_MS;

    this->theApp->getLogger().info("\t%s config:: enabled: %d, post_rest_url: <%s>, modbus_ascii: %d \n",
                                   this->getTitle().c_str(), on, this->monitor_post_rest_url.c_str(), this->modbus_ascii);

    if (!(this->monitor_post_rest_url.length() > 0))
    {
        return _Error(4, "Rs485MonitorModule Error: post_rest_url non configurato");
    }

    if (!on)
    {
        return _Disable;
    }
    return _NoError;
}

void Rs485MonitorModule::shutdown()
{
    this->theApp->getLogger().info(("%s: Module shutdown..\n"), this->getTitle().c_str());

    if (p_rs485)
    {
        p_rs485 = NULL;
    }
}

void Rs485MonitorModule::loop()
{
    this->serialTransactionTask();
}

void Rs485MonitorModule::serialTransactionTask()
{
    if (this->p_rs485)
    {
        Stream *pSerial = this->p_rs485->getSerialAsStream();
        if (pSerial)
        {
            switch (this->status)
            {
                default:
                case 0: //awaiting for packet
                {
                    if (pSerial->available())
                    {
                        this->status = 1;
                        this->frameBuffer_len = 0;
                        this->status_millis = millis();
                    }
                    break;
                }
                case 1: //reading packet till the end or timeout
                {
                    while (pSerial->available())
                    {
                        //DATA IN
                        this->status_millis = millis();                   
                        uint8_t byte = pSerial->read();

                        if(this->modbus_ascii && byte == ':' ) {
                            //ascii packet start
                            postToHttpServer(this->monitor_post_rest_url, this->frameBuffer, this->frameBuffer_len, this->modbus_ascii);
                             
                            this->frameBuffer[0] = byte;
                            this->frameBuffer_len = 1;
                        }
                        else {
                            this->frameBuffer[this->frameBuffer_len] = byte;
                            this->frameBuffer_len++;
                        }                       

                        if (this->frameBuffer_len >= BUFFER_MAX_SIZE)
                        {
                            // TODO: LOG: end of packet -> too big
                            postToHttpServer(this->monitor_post_rest_url, this->frameBuffer, this->frameBuffer_len, this->modbus_ascii);

                            this->status = 0;       
                            break;
                        }
                    }

                    if(this->frameBuffer_len>0) {
                        this->status = 2; // start pause time
                        this->status_millis = millis();
                    } else {
                        this->status = 0;
                    }
                    break;
                }
                case 2: //pause time
                {
                    if (pSerial->available()) {
                        // packet in
                        this->status = 1;
                    } else {
                        uint32_t time_elapsed_sinceLastDataIn = millis() - this->status_millis;
                        if (time_elapsed_sinceLastDataIn > this->packet_separation_time_ms)
                        {
                            // LOG: end of packet -> pause time elapsed
                            if (this->frameBuffer != NULL && this->frameBuffer_len > 0)
                            {
                                postToHttpServer(this->monitor_post_rest_url, this->frameBuffer, this->frameBuffer_len, this->modbus_ascii);
                            }

                            this->status = 0;
                        }
                    }
                    break;
                }
            }
        }
    }
}

static _Error postToHttpServer(String url, uint8_t *const frameBuffer, const uint16_t frameBuffer_len, bool ascii)
{
    // TODO: remove -> test
    _Error ret = _NoError;

    HTTPClient httpClient;
    bool ok = httpClient.begin(url);
    httpClient.setConnectTimeout(3000); //3 sec
    if (!ok)
    {
        // this->theApp->getLogger().error("HTTPconnection to url: %s, failed!", url.c_str());
        ret = _Error(10, String("Cannot start modbus Monitor connetion to url: ") + url.c_str());
    }
    else
    {

        String post_json_payload_string;
        // convert buffer -> post JSON payload
        {
            const size_t capacity = JSON_OBJECT_SIZE(2);
            DynamicJsonDocument doc(capacity);
            doc["ts"] = millis(); //    1351824120;

            if(ascii)
            {
                frameBuffer[frameBuffer_len] = '\0';
                String payload_encoded( (const char *)frameBuffer );
                doc["packet"] = payload_encoded;     
                doc["mode"] = 'ascii';     
            }
            else
            {
                size_t outputLength;
                unsigned char *payload_encoded = base64_encode((const unsigned char *)frameBuffer, frameBuffer_len, &outputLength);
                doc["packet"] = payload_encoded;                
                doc["mode"] = 'binary_encoded';     
            }

            size_t size = serializeJson(doc, post_json_payload_string);
        }

        // HEADER
        httpClient.addHeader("Content-Type", "application/json"); //Specify content-type header

        // POST
        int httpCode = httpClient.POST(post_json_payload_string);
        if (httpCode < 0)
        {
            ret = _Error(httpCode, String("HTTP post to url: ") + url.c_str() + ", returnCode: <" + httpCode + ">, error: <" + httpClient.errorToString(httpCode).c_str() + ">\n");
        }
        else
        {
            // POST done -> OK
            String response = httpClient.getString(); //Get the response to the request
            DPRINTF(F("HttpClient Response-> [%s]"), response.c_str());
        }
    }
    httpClient.end();

    return ret;
}