#include <stdio.h>

#include <crts/crts.hpp>
#include <crts/debug.h>
#include <crts/Controller.hpp>

//#include "../Filters/txControl.hpp"

class TestTx: public CRTSController
{
    public:

        TestTx(int argc, const char **argv);

        // When this destructor is called there are no more CRTSFilter
        // objects, so you can't access any CRTSControl objects in this
        // destructor.  If you needed final state from the CRTS filter
        // you needed to get that from when execute() is called or
        // in filterShutdown().
        ~TestTx(void) {  DSPEW(); };

        // This is called each time before the CRTS Filter is written to.
        // The tx->totalBytesIn() is the total amount that will have been
        // written to the CRTSFilter::write() just after this is called.
        //
        // We may attach to more than one CRTSControl for a given
        // CRTSController, hence the CRTSControl *c argument is passed to
        // this function, so that you can tell that cause this function to
        // be called.
        //
        // We don't get direct access to the CRTSFilter, we get access to
        // the CRTSControl that the filter makes.
        //
        void execute(CRTSControl *c, void * &buffer, size_t &len, uint32_t channelNum);

        // This is called by each CRTS Filter as it finishes running.  We
        // don't get direct access to the CRTSFilter, we get access to the
        // CRTSControl that the filter makes.
        //
        void shutdown(CRTSControl *c)
        {
            DSPEW("last use of CRTS Control named \"%s\"", c->getName());
        };

    private:

        CRTSControl *tx;

        double maxFreq, minFreq;
        uint64_t nextBytesOut;

};

// In mega HZ
#define DEFAULT_MAXFREQ (915.5)
#define DEFAULT_MINFREQ (914.5)

#define DEFAULT_TXCONTROL_NAME "tx"


//
//    crts_radio -f testTx [ --help ]
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
"  OPTIONS are optional.  Transmitter Controller test module.\n"
"\n"
"\n"
"  ---------------------------------------------------------------------------\n"
"                           OPTIONS\n"
"  ---------------------------------------------------------------------------\n"
"\n"
"\n"
"   --control NAME   connect to TX control named NAME.  The default TX\n"
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
"\n", name, DEFAULT_TXCONTROL_NAME, DEFAULT_MAXFREQ, DEFAULT_MINFREQ);
}


#define BYTES_PER_CHANGE (1000000)


TestTx::TestTx(int argc, const char **argv): nextBytesOut(BYTES_PER_CHANGE)
{
    CRTSModuleOptions opt(argc, argv, usage);

    const char *controlName = opt.get("--control", DEFAULT_TXCONTROL_NAME);
    tx = getControl<CRTSControl *>(controlName);

    if(tx == 0) throw "could not get TxControl";

    maxFreq = opt.get("--maxFreq", DEFAULT_MAXFREQ) * 1.0e6;
    minFreq = opt.get("--minFreq", DEFAULT_MINFREQ) * 1.0e6;

    DSPEW("controlName=\"%s\" maxFreq=%lg minFreq=%lg",
            controlName, maxFreq, minFreq);
}


void TestTx::execute(CRTSControl *c, void * &buffer, size_t &len, uint32_t channelNum)
{
    // We apply an action at every so many bytes out.
    //
    // tx->totalBytesOut() is the total size that will have been written
    // after this function returns.
    //
    // By taking a difference we can let the counters wrap through 0 and
    // not have a hick up.
    if(tx->totalBytesOut() - nextBytesOut >= nextBytesOut) return;

    nextBytesOut += BYTES_PER_CHANGE;

    // Or you could do:
    // nextBytesOut = BYTES_PER_CHANGE + tx->totalBytesOut();


    INFO("totalBytesIn=%" PRIu64
        " totalBytesOut=%" PRIu64,
        tx->totalBytesIn(),
        tx->totalBytesOut());
}


// Define the module loader stuff to make one of these class objects.
CRTSCONTROLLER_MAKE_MODULE(TestTx)
