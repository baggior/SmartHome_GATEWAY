
#pragma once



struct CommandObject
{
    String inputCommand;
    int clientNum;

    inline CommandObject() {reset();}    
    inline void reset() {inputCommand="";clientNum=-1;}
    inline operator bool() const {return inputCommand.length()>0;}
};
