#ifndef __Controller_hpp__
#define __Controller_hpp__

#include <inttypes.h>
#include <pthread.h>
#include <list>
#include <map>
#include <atomic>

#include <crts/MakeModule.hpp>


class Controller;
class CRTSControl;



class CRTSController
{

    public:

        CRTSController(void);
        virtual ~CRTSController(void);

    protected:

        CRTSControl *getControl(const char *name);

    private:

        // TODO: Ya, this is ugly.  It'd be nice to not expose these
        // things to the module writer.
        friend CRTSControl;
        friend int LoadCRTSController(const char *name,
                int argc, const char **argv, uint32_t magic);
        friend void removeCRTSCControllers(uint32_t magic);


        // We hide some details in this single object.
        static Controller controller;

        // Used to destroy this object because CRTS Controllers are loaded
        // as C++ plugins and destroyController must not be C++ name
        // mangled so that we can call the C wrapped delete.
        //
        // TODO: We should be hiding this data from the C++ header user interface.
        // But I'd too lazy to do that at this point.
        //
        void *(*destroyController)(CRTSController *);
};

#define CRTSCONTROLLER_MAKE_MODULE(derived_class_name) \
    CRTS_MAKE_MODULE(CRTSController, derived_class_name)


#endif // #ifndef __Controller_hpp__
