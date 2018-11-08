#include "coreapi.h"


_TaskModule::_TaskModule(String _title, String _descr, unsigned int _taskLoopTimeMs) 
: _BaseModule(_title, _descr, false, Order_Last), taskLoopTimeMs(_taskLoopTimeMs) 
{

}


void _TaskModule::setEnabled(bool _enabled)  
{   
    if(_enabled &&  !isEnabled())
    {        
        Scheduler & runner = this->theApp->getScheduler();
        
        TaskCallback funct = std::bind(&_TaskModule::loop, this);
        loopTask.set(this->taskLoopTimeMs
            , TASK_FOREVER
            , funct);
        runner.addTask(loopTask);
        loopTask.enable();     

        _BaseModule::setEnabled(true);
    }
    else if (!_enabled && isEnabled())
    {
        Scheduler & runner = this->theApp->getScheduler();
        
        loopTask.disable();
        runner.deleteTask(loopTask);

        _BaseModule::setEnabled(false);
    }
}


void _TaskModule::shutdown()  
{
    this->setEnabled(false); 
}