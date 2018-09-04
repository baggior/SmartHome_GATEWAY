
#include "config.h"

#include "wifiFtpServer.h"

#define FTP_LISTEN_TASK_INTERVAL_DEFAULT    10 //ms

extern Scheduler runner;

WifiFtpServer::WifiFtpServer()
: _TaskModule("FTPServer", "FTP server to access SPIFFS filesystem content")
{

}

WifiFtpServer::~WifiFtpServer()
{

}

_Error WifiFtpServer::setup()  
{
    bool on = true; 
    const char* _server_auth_username = NULL;
    const char* _server_auth_password = NULL;
    int task_listen_interval = 0;

    // configuration
    const JsonObject& root = this->theApp->getConfig().getJsonObject("ftp");
    if(root.success())
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

    this->theApp->getLogger().printf( F("\t%s config: enable: %d, server_auth_username: %s, server_auth_password: %s, taskLoopTimeMs: %d \n"), 
        this->getTitle().c_str(),
        on, REPLACE_NULL_STR(_server_auth_username), REPLACE_NULL_STR(_server_auth_password), this->taskLoopTimeMs);

    if(on)
    {
        if(!SPIFFS.begin())
        {
            DPRINTF("Error opening SPIFFS filesystem\n");   
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
void WifiFtpServer::setup(Stream &serial)
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