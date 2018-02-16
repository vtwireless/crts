#ifndef __Module_h__
#define __Module_h__

#include <crts/MakeModule.hpp>


class CRTSModule
{
    public:

        CRTSModule(void);

        virtual ~CRTSModule(void);
};


#define CRTSMODULE_MAKE_MODULE(derived_class_name) \
    CRTS_MAKE_MODULE(CRTSModule, derived_class_name)

#endif // #ifndef __Module__
