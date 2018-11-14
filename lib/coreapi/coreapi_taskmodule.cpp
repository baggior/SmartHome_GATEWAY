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
        Scheduler & runner = this->theApp->getScheduler();
        
        TaskCallback funct = std::bind(&_TaskModule::taskloop, this);
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

void _TaskModule::taskloop()
{    
    static long logmillis = 0 ;
    bool tolog = this->theApp->isDebug() && ( (millis() - logmillis) > 1000) ;
    if(tolog) {
        this->theApp->getLogger().printf(F("TASK [%s]loop(%d)BEGIN, "), 
            this->getTitle().c_str(), this->loopcnt );
    }

    this->loop();
    this->loopcnt++; 

    if(tolog) {
        this->theApp->getLogger().printf(F("TASK [%s]loop END.\n"), 
            this->getTitle().c_str() );
        logmillis = millis(); 
    }

}

void _TaskModule::shutdown()  
{
    this->setEnabled(false); 
}