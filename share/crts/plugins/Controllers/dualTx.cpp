#include <stdio.h>
#include <math.h>

#include <crts/crts.hpp>
#include <crts/debug.h>
#include <crts/Controller.hpp>

#include "../Filters/txControl.hpp"

class TestDual: public CRTSController
{
    public:

        TestDual(int argc, const char **argv);

        // When this destructor is called there are no more CRTSFilter
        // objects, so you can't access any CRTSControl objects in this
        // destructor.  If you needed final state from the CRTS filter
        // you needed to get that from when execute() is called or
        // in filterShutdown().
        ~TestDual(void) {  DSPEW(); };

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
        void execute(CRTSControl *c);

        // This is called by each CRTS Filter as it finishes running.  We
        // don't get direct access to the CRTSFilter, we get access to the
        // CRTSControl that the filter makes.
        //
        void shutdown(CRTSControl *c)
        {
            DSPEW("last use of CRTS Control named \"%s\"", c->getName());
        };

    private:

        TxControl *tx;

        double maxFreq, minFreq;
        uint64_t nextBytesIn;

};

// In mega HZ
#define DEFAULT_MAXFREQ (915.5)
#define DEFAULT_MINFREQ (914.5)

#define DEFAULT_TXCONTROL_NAME "tx"


//
//    crts_radio -f testDual [ --help ]
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


#define BYTES_PER_CHANGE (5000000)


TestDual::TestDual(int argc, const char **argv): nextBytesIn(BYTES_PER_CHANGE)
{
    CRTSModuleOptions opt(argc, argv, usage);

    const char *controlName = opt.get("--control", DEFAULT_TXCONTROL_NAME);
    tx = getControl<TxControl *>(controlName);

    if(tx == 0) throw "could not get TxControl";

    maxFreq = opt.get("--maxFreq", DEFAULT_MAXFREQ) * 1.0e6;
    minFreq = opt.get("--minFreq", DEFAULT_MINFREQ) * 1.0e6;

    tx->usrp->set_tx_freq(maxFreq);
    
    DSPEW("controlName=\"%s\" maxFreq=%lg minFreq=%lg",
            controlName, maxFreq, minFreq);

}


static double fartherFrom(double val, double x, double y)
{
    if(fabs(x - val) < fabs(y - val))
        return y;
    return x;
}


void TestDual::execute(CRTSControl *c)
{
    // We apply an action at every so many bytes out.
    //
    // tx->totalBytesOut() is the total size that will have been written
    // after this function returns.
    //
    // By taking a difference we can let the counters wrap through 0 and
    // not have a hick up.
    if(tx->totalBytesIn() - nextBytesIn >= nextBytesIn) return;

    nextBytesIn += BYTES_PER_CHANGE;


    // Set a different carrier frequency:
    double freq = fartherFrom(tx->usrp->get_tx_freq(), maxFreq, minFreq);
    tx->usrp->set_tx_freq(freq);

    // Or you could do:
    // nextBytesIn = BYTES_PER_CHANGE + tx->totalBytesIn();


    INFO("totalBytesIn=%" PRIu64
        " totalBytesOut=%" PRIu64 " freq=%lg",
        tx->totalBytesIn(),
        tx->totalBytesOut(), tx->usrp->get_tx_freq());
}


// Define the module loader stuff to make one of these class objects.
CRTSCONTROLLER_MAKE_MODULE(TestDual)
