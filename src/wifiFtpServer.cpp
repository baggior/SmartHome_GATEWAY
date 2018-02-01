
#include "config.h"

#include "wifiFtpServer.h"

#define FTP_LISTEN_TASK_INTERVAL_DEFAULT 1 //ms

extern Scheduler runner;

void WifiFtpServer::setup(Stream &serial)
{
    
    JsonObject & root = config.getJsonRoot();    
    enable = root["ftp"]["enable"];     
    const char* _server_auth_username = root["ftp"]["server_auth"]["username"];
    const char* _server_auth_password = root["ftp"]["server_auth"]["password"];
    int task_listen_interval = root["ftp"]["task_listen_interval"];
    
    DPRINTF(">FTP Server SETUP: enable: %d, server_auth_username: %s, server_auth_password: %s, task_listen_interval: %d \n", 
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
