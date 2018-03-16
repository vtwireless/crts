#include <stdio.h>

#include "crts/crts.hpp"
#include "crts/debug.h"
#include "crts/Controller.hpp"

#include "../Filters/rxControl.hpp"

class TestRx: public CRTSController
{
    public:

        TestRx(int argc, const char **argv);

        // When this destructor is called there are no more CRTSFilter
        // objects, so you can't access any CRTSControl objects in this
        // destructor.  If you needed final state from the CRTS filter
        // you needed to get that from when execute() is called or
        // in filterShutdown().
        ~TestRx(void) {  DSPEW(); };

        void execute(CRTSControl *c);

        // This is called by each CRTS Filter as it finishes running.
        // We don't get access to the CRTSFilter, we get access to the
        // CRTSControl that the filter makes.
        void shutdown(CRTSControl *c)
        {
            DSPEW("last use of CRTS Control named \"%s\"",
                    c->getName());
        };

    private:

        RxControl *rx;

        double maxFreq, minFreq;
        uint64_t nextBytesOut;

};

// In mega HZ
#define DEFAULT_MAXFREQ (915.5)
#define DEFAULT_MINFREQ (914.5)


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
"\n"
"   --maxFreq MAX    set the maximum carrier frequency to MAX Mega Hz.\n"
"                    The default MAX is %g\n"
"\n"
"\n"
"   --minFreq MIN    set the minimum carrier frequency to MIN Mega Hz.\n"
"                    The default MIN is %g\n"
"\n"
"\n", name, DEFAULT_RXCONTROL_NAME, DEFAULT_MAXFREQ, DEFAULT_MINFREQ);
}


#define BYTES_PER_CHANGE (1000000)


TestRx::TestRx(int argc, const char **argv): nextBytesOut(BYTES_PER_CHANGE)
{
    CRTSModuleOptions opt(argc, argv, usage);

    const char *controlName = opt.get("--control", DEFAULT_RXCONTROL_NAME);
    rx = getControl<RxControl *>(controlName);

    if(rx == 0) throw "could not get RxControl";

    maxFreq = opt.get("--maxFreq", DEFAULT_MAXFREQ) * 1.0e6;
    minFreq = opt.get("--minFreq", DEFAULT_MINFREQ) * 1.0e6;

    DSPEW("controlName=\"%s\" maxFreq=%lg minFreq=%lg",
            controlName, maxFreq, minFreq);
}


void TestRx::execute(CRTSControl *c)
{
    // We apply an action at every so many bytes out.
    //
    // rx->totalBytesOut() is the total size that will have been written
    // after this function returns.
    //
    // By taking a difference we can let the counters wrap through 0.
    if(rx->totalBytesOut() - nextBytesOut >= nextBytesOut) return;

    nextBytesOut += BYTES_PER_CHANGE;

    // Or you could do:
    // nextBytesOut = BYTES_PER_CHANGE + rx->totalBytesOut();


    INFO("totalBytesIn=%" PRIu64
        " totalBytesOut=%" PRIu64,
        rx->totalBytesIn(),
        rx->totalBytesOut());
}


// Define the module loader stuff to make one of these class objects.
CRTSCONTROLLER_MAKE_MODULE(TestRx)
