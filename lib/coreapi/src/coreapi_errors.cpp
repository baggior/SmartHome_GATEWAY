#include "coreapi.h"

////////////////////////////

_Error _NoError;
_Error _Disable(1000, "Disable module request");
_Error _FSError(10, "Cannot open filesystem");
_Error _ConfigPersistError(10, "Cannot write json config file");
_Error _ConfigLoadError(1, "Cannot read json config file");

////////////////////////////

size_t _Error::printTo(Print& p) const
{
    size_t ret;
    if(*this == _NoError)
        ret = p.printf(("[NoError]"));
    else
        ret = p.printf(("[Error (%d): %s]"), this->errorCode, this->message.c_str());
    return ret;
}
