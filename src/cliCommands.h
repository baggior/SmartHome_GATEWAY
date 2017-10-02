#ifndef clicommands_h
#define clicommands_h

#include <CmdMessenger.h> 


class CliCommands 
{
    static CliCommands * _instance;

    CmdMessenger* cmdMessenger;
    Stream* current;

    void attachCommandCallbacks();

public:
    CliCommands();
    void setupCli(Stream& stream);
    void processCli(Stream& stream);
    
    static CliCommands* getInstance()
    {        
        return _instance;
    }
    static CmdMessenger* getMessengerInstance()
    {        
        if(_instance) return _instance->cmdMessenger;
        return NULL;
    }

};



#endif