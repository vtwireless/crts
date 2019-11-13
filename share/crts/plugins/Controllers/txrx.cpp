#include <stdio.h>
#include <math.h>

#include <crts/crts.hpp>
#include <crts/debug.h>
#include <crts/Controller.hpp>

class TxRx: public CRTSController
{
    public:

        TxRx(int argc, const char **argv);

        ~TxRx(void) {  DSPEW(); };

        void execute(CRTSControl *t, CRTSControl *r, const void *buffer, size_t len, uint32_t channelNum);
        void start(CRTSControl *t, CRTSControl *r, uint32_t numIn, uint32_t numOut);
        void stop(CRTSControl *t, CRTSControl *r);

    private:

        double maxFreq, minFreq;
        uint64_t nextTxBytesIn, nextRxBytesIn;
};

// In mega HZ
#define DEFAULT_MAXFREQ (915.5)

#define DEFAULT_MINFREQ (914.5)

#define DEFAULT_TXCONTROL_NAME "tx"

#define DEFAULT_RXCONTROL_NAME "rx"


static void usage(void)
{
    char nameBuf[64], *nameTx, *nameRx;
    nameTx = CRTS_BASENAME(nameBuf, 64);
    nameRx = CRTS_BASENAME(nameBuf, 64);
    fprintf(stderr,
"\n"
"\n"
"Usage: %s [ OPTIONS ]\n"
"\n"
"  OPTIONS are optional.  Transmitter Receiver Controller test module.\n"
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
"   --control NAME   connect to RX control named NAME.  The default TX\n"
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
"\n", nameTx, DEFAULT_TXCONTROL_NAME, nameRx, DEFAULT_RXCONTROL_NAME  DEFAULT_MAXFREQ, DEFAULT_MINFREQ);
}


#define BYTES_PER_CHANGE (5000000)


TxRx::TxRx(int argc, const char **argv): nextTxBytesIn(BYTES_PER_CHANGE), nextRxBytesIn(BYTES_PER_CHANGE)
{
    CRTSModuleOptions opt(argc, argv, usage);

    const char *controlTxName = opt.get("--control", DEFAULT_TXCONTROL_NAME);
    const char *controlRxName = opt.get("--control", DEFAULT_RXCONTROL_NAME);
    CRTSControl *t = getControl<CRTSControl *>(controlTxName);
    CRTSControl *r = getControl<CRTSControl *>(controlRxName);

    if(t == 0) throw "could not get Tx Control";
    if(r == 0) throw "could not get Rx Control";

    maxFreq = opt.get("--maxFreq", DEFAULT_MAXFREQ) * 1.0e6;
    minFreq = opt.get("--minFreq", DEFAULT_MINFREQ) * 1.0e6;
}


static inline double FartherFrom(double val, double x, double y)
{
    if(fabs(x - val) < fabs(y - val))
        return y;
    return x;
}


void TxRx::start(CRTSControl *t, CRTSControl *r, uint32_t numIn, uint32_t numOut)
{
    t->setParameter("freq", maxFreq);

    DSPEW("Tx maxFreq=%lg minFreq=%lg", maxFreq, minFreq);

    r->setParameter("freq", maxFreq);

    DSPEW("Rx maxFreq=%lg minFreq=%lg", maxFreq, minFreq);
}


void TxRx::stop(CRTSControl *t, CRTSControl *r)
{
    DSPEW();
}

void TxRx::execute(CRTSControl *t, CRTSControl *r,const void *buffer, size_t len, uint32_t channelNum){
    // We apply an action at every so many bytes out.
    //
    // tx->totalBytesOut() is the total size that will have been written
    // after this function returns.
    //
    // By taking a difference we can let the counters wrap through 0 and
    // not have a hick up.
    if(t->totalBytesIn() - nextTxBytesIn >= nextTxBytesIn) return;
    if(r->totalBytesIn() - nextRxBytesIn >= nextRxBytesIn) return;

    nextTxBytesIn = BYTES_PER_CHANGE + tx->totalBytesIn();
    nextRxBytesIn = BYTES_PER_CHANGE + rx->totalBytesIn();


    // Set a different carrier frequency:
    double freq = FartherFrom(t->getParameter("freq"), maxFreq, minFreq);
    t->setParameter("freq", freq);
    r->setParameter("freq", freq);



    INFO("totalBytesIn=%" PRIu64
        " totalBytesOut=%" PRIu64 " freq=%lg",
        t->totalBytesIn(),
        t->totalBytesOut(), t->getParameter("freq"));

    INFO("totalBytesIn=%" PRIu64
        " totalBytesOut=%" PRIu64 " freq=%lg",
        r->totalBytesIn(),
        r->totalBytesOut(), r->getParameter("freq"));

}


// Define the module loader stuff to make one of these class objects.
CRTSCONTROLLER_MAKE_MODULE(TxRx)

