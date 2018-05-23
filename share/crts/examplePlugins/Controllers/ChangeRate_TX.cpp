#include <stdio.h>
#include <math.h>

#include <crts/crts.hpp>
#include <crts/debug.h>
#include <crts/Controller.hpp>

#include "../Filters/txControl.hpp"

class DualTx: public CRTSController
{
    public:

        DualTx(int argc, const char **argv);

        ~DualTx(void) {  DSPEW(); };

        void execute(CRTSControl *c, const void *buffer, size_t len, uint32_t channelNum);
        void start(CRTSControl *c);
        void stop(CRTSControl *c);

    private:

        TxControl *tx;

        double maxFreq, minFreq;
        uint64_t nextBytesIn;
        const char *controlName;
};

// In mega HZ
#define DEFAULT_MAXFREQ (915.5)
#define DEFAULT_MINFREQ (914.5)


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


DualTx::DualTx(int argc, const char **argv): nextBytesIn(BYTES_PER_CHANGE)
{
    CRTSModuleOptions opt(argc, argv, usage);

    controlName = opt.get("--control", DEFAULT_TXCONTROL_NAME);
    tx = getControl<TxControl *>(controlName);

    if(tx == 0) throw "could not get TxControl";

    maxFreq = opt.get("--maxFreq", DEFAULT_MAXFREQ) * 1.0e6;
    minFreq = opt.get("--minFreq", DEFAULT_MINFREQ) * 1.0e6;
}


static inline double FartherFrom(double val, double x, double y)
{
    if(fabs(x - val) < fabs(y - val))
        return y;
    return x;
}


void DualTx::start(CRTSControl *c)
{
    DASSERT(tx && tx->usrp, "");

    tx->usrp->set_tx_freq(maxFreq);

    DSPEW("controlName=\"%s\" maxFreq=%lg minFreq=%lg",
            controlName, maxFreq, minFreq);
}


void DualTx::stop(CRTSControl *c)
{
    DSPEW();
}


void DualTx::execute(CRTSControl *c, const void *buffer, size_t len, uint32_t channelNum)
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
    double freq = FartherFrom(tx->usrp->get_tx_freq(), maxFreq, minFreq);
    tx->usrp->set_tx_freq(freq);

    // Or you could do:
    // nextBytesIn = BYTES_PER_CHANGE + tx->totalBytesIn();

    INFO("totalBytesIn=%" PRIu64
        " totalBytesOut=%" PRIu64 " freq=%lg",
        tx->totalBytesIn(),
        tx->totalBytesOut(), tx->usrp->get_tx_freq());
}


// Define the module loader stuff to make one of these class objects.
CRTSCONTROLLER_MAKE_MODULE(DualTx)
