#include <stdio.h>
#include <math.h>

#include <crts/crts.hpp>
#include <crts/debug.h>
#include <crts/Controller.hpp>


class Monitor: public CRTSController
{
    public:

        Monitor(int argc, const char **argv);

        ~Monitor(void) {  DSPEW(); };

        void execute(CRTSControl *c, void * &buffer, size_t &len, uint32_t channelNum);

        void shutdown(CRTSControl *c)
        {
            DSPEW("last use of CRTS Control named \"%s\"", c->getName());
        };

    private:

        CRTSControl *control;
};


#define DEFAULT_CONTROL_NAME  "control"


static void usage(void)
{
    char nameBuf[64], *name;
    name = CRTS_BASENAME(nameBuf, 64);
    fprintf(stderr,
"\n"
"\n"
"Usage: %s [ OPTIONS ]\n"
"\n"
"\n"
"\n"
"\n"
"  ---------------------------------------------------------------------------\n"
"                           OPTIONS\n"
"  ---------------------------------------------------------------------------\n"
"\n"
"\n"
"   --control NAME   connect to control named NAME.  The default control name\n"
"                    is \"%s\".\n"
"\n"
"\n"
"\n", name, DEFAULT_CONTROL_NAME);
}



Monitor::Monitor(int argc, const char **argv)
{
    CRTSModuleOptions opt(argc, argv, usage);

    const char *controlName = opt.get("--control", DEFAULT_CONTROL_NAME);
    control = getControl<CRTSControl *>(controlName);

    if(control == 0) throw "could not get Control";

    DSPEW("controlName=\"%s\"", controlName);
}


void Monitor::execute(CRTSControl *c, void * &buffer, size_t &len, uint32_t channelNum)
{
    INFO("totalBytesIn=%" PRIu64
        " totalBytesOut=%" PRIu64,
        control->totalBytesIn(),
        control->totalBytesOut());
}


// Define the module loader stuff to make one of these class objects.
CRTSCONTROLLER_MAKE_MODULE(Monitor)
