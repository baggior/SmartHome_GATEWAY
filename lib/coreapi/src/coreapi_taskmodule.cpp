#include "coreapi.h"


_TaskModule::_TaskModule(String _title, String _descr, unsigned int _taskLoopTimeMs) 
:   _BaseModule(_title, _descr, false, Order_Last),
    taskLoopTimeMs(_taskLoopTimeMs),
    loopcnt(0)
{

}


void _TaskModule::setEnabled(bool _enabled)  
{   
    if(_enabled &&  !isEnabled())
    {        
        this->theApp->getLogger().info(("%s: TaskModule enable..\n"), this->getTitle().c_str());

        Scheduler & runner = this->theApp->getScheduler();
        
        TaskCallback funct = std::bind(&_TaskModule::taskloop, this);
        this->loopTask.set(this->taskLoopTimeMs
            , TASK_FOREVER
            , funct);
        runner.addTask(loopTask);
        loopTask.enable();     

        _BaseModule::setEnabled(true);
    }
    else if (!_enabled && isEnabled())
    {
        this->theApp->getLogger().info(("%s: TaskModule disable..\n"), this->getTitle().c_str());

        Scheduler & runner = this->theApp->getScheduler();
        
        this->loopTask.disable();
        runner.deleteTask(loopTask);

        _BaseModule::setEnabled(false);
    }
}

void _TaskModule::taskloop()
{    
    // if(this->theApp->isToLog()) {
    //     this->theApp->getLogger().printf(("TASK [%s]loop(%d)BEGIN, "), 
    //         this->getTitle().c_str(), this->loopcnt );
    // }

    this->loop();
    this->loopcnt++; 

    // if(this->theApp->isToLog()) {
    //     this->theApp->getLogger().printf(("TASK [%s]loop END.\n"), 
    //         this->getTitle().c_str() );         
    // }
}

void _TaskModule::shutdown()  
{
    this->theApp->getLogger().info(("%s: TaskModule shutdown..\n"), this->getTitle().c_str());
    this->setEnabled(false); 
}