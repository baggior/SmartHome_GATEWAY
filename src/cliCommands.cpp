#include "config.h"


#include "cliCommands.h"
#include <WifiClient.h>

CliCommands * CliCommands::_instance = NULL;

// Commands
enum
{
  kHelp,  // Command to request print help
  ksMain, // response Main
};

// Callback function
void OnHelp()
{
    CmdMessenger * cmdMessenger = CliCommands::getMessengerInstance();
    if(cmdMessenger)
    {
    }
}
// Called when a received command has no attached function
void OnUnknownCommand()
{
    CmdMessenger * cmdMessenger = CliCommands::getMessengerInstance();
    if(cmdMessenger)
    {
        cmdMessenger->sendCmd(ksMain,"received a Command without attached callback.\n Try:\t kHelp");
        cmdMessenger->sendCmd(ksMain);    
    }
}
    

// Callbacks define on which received commands we take action 
void CliCommands::attachCommandCallbacks()
{
  cmdMessenger->attach(kHelp, OnHelp);
  cmdMessenger->attach(OnUnknownCommand);
}


CliCommands::CliCommands()
{
    // Attach a new CmdMessenger object to the default Serial port
    CmdMessenger* cmdMessenger=NULL;
    Stream* current=NULL;

    _instance = this;
}

void CliCommands::setupCli(Stream& stream)
{    
    if(cmdMessenger!=NULL)
        delete cmdMessenger;

    if(current!=NULL)
        current->flush();
    
    cmdMessenger = new CmdMessenger(stream);
    current=&stream;
        
    // Adds newline to every command
    cmdMessenger->printLfCr();   
  
    // Attach my application's user-defined callback methods
    attachCommandCallbacks();
}

void CliCommands::processCli(Stream& stream)
{    
    if(&stream!=current)
    {
        setupCli(stream);
    }

    // Process incoming serial data, and perform callbacks
    if(cmdMessenger)    cmdMessenger->feedinSerialData();
}


