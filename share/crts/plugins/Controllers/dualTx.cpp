#include <stdio.h>
#include <math.h>

#include <crts/crts.hpp>
#include <crts/debug.h>
#include <crts/Controller.hpp>

class DualTx: public CRTSController
{
    public:

        DualTx(int argc, const char **argv);

        ~DualTx(void) {  DSPEW(); };

        void execute(CRTSControl *c, const void *buffer, size_t len, uint32_t channelNum);
        void start(CRTSControl *c, uint32_t numIn, uint32_t numOut);
        void stop(CRTSControl *c);

    private:

        double maxFreq, minFreq;
        uint64_t nextBytesIn;
};

// In mega HZ
#define DEFAULT_MAXFREQ (915.5)
#define DEFAULT_MINFREQ (914.5)

#define DEFAULT_TXCONTROL_NAME "tx"


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


#define BYTES_PER_CHANGE (50000000)


DualTx::DualTx(int argc, const char **argv): nextBytesIn(BYTES_PER_CHANGE)
{
    CRTSModuleOptions opt(argc, argv, usage);

    const char *controlName = opt.get("--control", DEFAULT_TXCONTROL_NAME);
    CRTSControl *c = getControl<CRTSControl *>(controlName);

    if(c == 0) throw "could not get Tx Control";

    maxFreq = opt.get("--maxFreq", DEFAULT_MAXFREQ) * 1.0e6;
    minFreq = opt.get("--minFreq", DEFAULT_MINFREQ) * 1.0e6;
}


static inline double FartherFrom(double val, double x, double y)
{
    if(fabs(x - val) < fabs(y - val))
        return y;
    return x;
}


void DualTx::start(CRTSControl *c, uint32_t numIn, uint32_t numOut)
{
    c->setParameter("freq", maxFreq);

    DSPEW("maxFreq=%lg minFreq=%lg", maxFreq, minFreq);
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
    if(c->totalBytesIn() - nextBytesIn >= nextBytesIn) return;

    nextBytesIn += BYTES_PER_CHANGE;


    // Set a different carrier frequency:
    double freq = FartherFrom(c->getParameter("freq"), maxFreq, minFreq);
    c->setParameter("freq", freq);

    // Or you could do:
    // nextBytesIn = BYTES_PER_CHANGE + tx->totalBytesIn();

    INFO("totalBytesIn=%" PRIu64
        " totalBytesOut=%" PRIu64 " freq=%lg",
        c->totalBytesIn(),
        c->totalBytesOut(), c->getParameter("freq"));
}


// Define the module loader stuff to make one of these class objects.
CRTSCONTROLLER_MAKE_MODULE(DualTx)
