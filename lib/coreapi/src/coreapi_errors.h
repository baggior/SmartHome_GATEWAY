#ifndef _coreapierrors_h
#define _coreapierrors_h

#include <printable.h>

class _Error : public Printable {
public:
    inline _Error() {}
    inline _Error(int _errorCode, String _message): errorCode(_errorCode), message(_message) {}
    int errorCode=0;
    String message="";

    inline bool operator==(const _Error& that)const {return that.errorCode==this->errorCode;}
    inline bool operator!=(const _Error& that)const {return !(that==*this);}

    virtual size_t printTo(Print& p) const;
};

extern _Error _NoError;
extern _Error _Disable;
extern _Error _FSError;
extern _Error _ConfigLoadError;
extern _Error _ConfigPersistError;

#endif