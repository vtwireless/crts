#include <stdio.h>

#include "crts/crts.hpp"
#include "crts/debug.h"
#include "crts/Controller.hpp"

#include "../Filters/rxControl.hpp"

class TestRx: public CRTSController
{
    public:

        TestRx(int argc, const char **argv);
        ~TestRx(void) {  DSPEW(); };

        void execute(void);

    private:

        RxControl *rx;

};


//
//    crts_radio -f testRx [ --help ]
//
//
static void usage(void)
{
    char nameBuf[64], *name;
    name = CRTS_BASENAME(nameBuf, 64);
    fprintf(stderr,
"\n"
"\n"
"Usage: %s [ OPTIONS ]\n"
"\n"
"  OPTIONS are optional.\n"
"\n"
"\n"
"  ---------------------------------------------------------------------------\n"
"                           OPTIONS\n"
"  ---------------------------------------------------------------------------\n"
"\n"
"\n"
"   --control NAME   connect to RX control named NAME.  The default RX\n"
"                    control name is \"%s\".\n"
"\n"
"\n", name, DEFAULT_RXCONTROL_NAME);
}



TestRx::TestRx(int argc, const char **argv)
{
    CRTSModuleOptions opt(argc, argv, usage);

    rx = getControl<RxControl *>(opt.get("--control", DEFAULT_RXCONTROL_NAME));

    if(rx == 0) throw "could not get RxControl";

    DSPEW();
}


void TestRx::execute(void)
{
    DSPEW("totalBytesIn=%" PRIu64
        " totalBytesOut=%" PRIu64,
        rx->totalBytesIn(),
        rx->totalBytesOut());
}


// Define the module loader stuff to make one of these class objects.
CRTSCONTROLLER_MAKE_MODULE(TestRx)
