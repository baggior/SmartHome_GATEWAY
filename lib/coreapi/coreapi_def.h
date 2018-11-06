#ifndef _coreapiconfig_h
#define _coreapiconfig_h


#define MAX_MODULES         20
#define MAX_MDNS_ATTRIBUTES 10 

enum CoreModuleTypeEnum {
    ServiceTypeEnum ,
    TaskTypeEnum ,
    ApiTypeEnum ,
    OtherTypeEnum ,

    AnyModuleType
};


enum CoreModuleNamesEnum {
    _CoreWifiConnectionModuleEnum,
    _CoreRestApiModuleEnum
};

#endif

