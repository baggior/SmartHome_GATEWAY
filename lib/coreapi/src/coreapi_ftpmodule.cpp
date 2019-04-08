#include "coreapi_ftpmodule.h"

#define FTP_LISTEN_TASK_INTERVAL_DEFAULT    10 //ms
#define FORMAT_SPIFFS_ON_FAIL true  //ESP32

// extern Scheduler runner;

WifiFtpServerModule::WifiFtpServerModule()
: _TaskModule( ENUM_TO_STR(_CoreFTPServerModule), "FTP server to access SPIFFS filesystem content" )
{

}

WifiFtpServerModule::~WifiFtpServerModule()
{
    this->shutdown();
}

_Error WifiFtpServerModule::setup()  
{
    bool on = false; 
    const char* _server_auth_username = NULL;
    const char* _server_auth_password = NULL;
    int task_listen_interval = 0;

    // configuration
    const JsonObject& root = this->theApp->getConfig().getJsonObject("ftp");
    if(!root.isNull())
    {
        on = root["enable"]; 
        _server_auth_username = root["server_auth"]["username"];
        _server_auth_password = root["server_auth"]["password"];
        task_listen_interval = root["task_listen_interval"];               
    }

    if(!_server_auth_username) _server_auth_username="";
    if(!_server_auth_password) _server_auth_password="";
    if(!task_listen_interval) task_listen_interval=FTP_LISTEN_TASK_INTERVAL_DEFAULT;

    this->taskLoopTimeMs = task_listen_interval;

    this->theApp->getLogger().info (("\t%s config: enable: %d, server_auth_username: %s, server_auth_password: %s, taskLoopTimeMs: %d \n"), 
        this->getTitle().c_str(), on
        , REPLACE_NULL_STR(_server_auth_username), REPLACE_NULL_STR(_server_auth_password), this->taskLoopTimeMs);

    if(on)
    {
#ifdef ESP32
        bool b = SPIFFS.begin(FORMAT_SPIFFS_ON_FAIL);
#elif defined ESP8266
        bool b = SPIFFS.begin();
#endif
        if(!b)
        {
            this->theApp->getLogger().error("Error opening SPIFFS filesystem\n");   
        }
        
        ftpServer.begin(_server_auth_username, _server_auth_password);

        return _NoError;            
    }
    else
    {
        return _Disable;
    }    
}
/*
void WifiFtpServerModule::setup(Stream &serial)
{
    
    JsonObject & root = config.getJsonRoot();    
    enable = root["ftp"]["enable"];     
    const char* _server_auth_username = root["ftp"]["server_auth"]["username"];
    const char* _server_auth_password = root["ftp"]["server_auth"]["password"];
    int task_listen_interval = root["ftp"]["task_listen_interval"];
    
    DPRINTF(F(">FTP Server SETUP: enable: %d, server_auth_username: %s, server_auth_password: %s, task_listen_interval: %d \n"), 
    enable, REPLACE_NULL_STR(_server_auth_username), REPLACE_NULL_STR(_server_auth_password), task_listen_interval);
    
    if(!_server_auth_username) _server_auth_username="";
    if(!_server_auth_password) _server_auth_password="";
    if(!task_listen_interval) task_listen_interval=FTP_LISTEN_TASK_INTERVAL_DEFAULT;
    
    if(enable)
    {
        if(!SPIFFS.begin())
        {
            DPRINTF("Error opening SPIFFS filesystem\n");   
        }
        
        ftpServer.begin(_server_auth_username, _server_auth_password);

        //TASK setting
        TaskCallback funct = std::bind(&WifiFtpServer::process, this);
        taskReceiveCmd.set(task_listen_interval
            , TASK_FOREVER
            , funct
            );
        runner.addTask(taskReceiveCmd);
        taskReceiveCmd.enable();
    }
}
*/