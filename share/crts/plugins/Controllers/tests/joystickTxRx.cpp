#include <stdio.h>
#include <values.h>

#include <crts/crts.hpp>
#include <crts/debug.h>
#include <crts/Controller.hpp>
#include <linux/input.h>
#include <linux/joystick.h>

#define DEFAULT_TXCONTROL_NAME "tx"
#define DEFAULT_RXCONTROL_NAME "rx"


class JoystickTxRx: public CRTSController
{
    public:

        JoystickTxRx(int argc, const char **argv);

        ~JoystickTxRx(void) {  DSPEW(); };

        void start(CRTSControl *c);
        void stop(CRTSControl *c);

        void execute(CRTSControl *c, const void *buffer, size_t len,
                uint32_t channelNum);

    private:

        CRTSControl *tx;
        CRTSControl *rx;
        CRTSControl *js;

        double maxFreq, minFreq, rxFreq, lastRxFreq, txFreq, lastTxFreq;
};


// in mega-herz (M Hz)
#define DEFAULT_MAXFREQ (915.5)
#define DEFAULT_MINFREQ (914.5)

#define DEFAULT_JSCONTROL_NAME      "joystick"


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
"   --controlJs NAME   connect to joystick control named NAME.  The default\n"
"                      joystick control name is \"%s\".\n"
"\n"
"\n"
"   --controlRx NAME   connect to RX control named NAME.  The default RX\n"
"                      control name is \"%s\".\n"
"\n"
"\n"
"   --controlTx NAME   connect to TX control named NAME.  The default TX\n"
"                      control name is \"%s\".\n"
"\n"
"\n"
"   --maxFreq MAX      set the maximum carrier frequency to MAX Mega Hz.  The\n"
"                      default MAX is %lg\n"
"\n"
"\n"
"   --minFreq MIN      set the minimum carrier frequency to MIN Mega Hz. The\n"
"                      default MIN is %lg\n"
"\n"
"\n", name, DEFAULT_JSCONTROL_NAME, DEFAULT_RXCONTROL_NAME,
    DEFAULT_TXCONTROL_NAME,
    DEFAULT_MAXFREQ, DEFAULT_MINFREQ);
}


JoystickTxRx::JoystickTxRx(int argc, const char **argv)
{
    CRTSModuleOptions opt(argc, argv, usage);
    const char *controlName;

    controlName = opt.get("--controlTx", DEFAULT_TXCONTROL_NAME);
    tx = getControl<CRTSControl *>(controlName);
    controlName = opt.get("--controlRx", DEFAULT_RXCONTROL_NAME);
    rx = getControl<CRTSControl *>(controlName);
    controlName = opt.get("--controlJs", DEFAULT_JSCONTROL_NAME);
    js = getControl<CRTSControl *>(controlName);

    maxFreq = opt.get("--maxFreq", DEFAULT_MAXFREQ) * 1.0e6;
    minFreq = opt.get("--minFreq", DEFAULT_MINFREQ) * 1.0e6;

    txFreq = lastTxFreq = maxFreq;
    rxFreq = lastRxFreq = maxFreq;

    DSPEW();
}


void JoystickTxRx::start(CRTSControl *c)
{
    if(c->getId() == tx->getId())
    {
        // This call is from the CRTS Tx filter
        tx->setParameter("freqi", txFreq);
        lastTxFreq = txFreq;
        // TODO: check that the freq was really set.
    }

    if(c->getId() == rx->getId())
    {
        // This call is from the CRTS Tx filter
        rx->setParameter("freq", rxFreq);
        lastRxFreq = rxFreq;
        // TODO: check that the freq was really set.
    }

    DSPEW("control=\"%s\"", c->getName());
}


void JoystickTxRx::stop(CRTSControl *c)
{
    DSPEW();
}


void JoystickTxRx::execute(CRTSControl *c, const void *buffer,
        size_t len, uint32_t channelNum)
{
    if(c->getId() == tx->getId())
    {
        // This call is from the CRTS Tx filter

        if(txFreq != lastTxFreq)
        {
            fprintf(stderr, "   Setting TX carrier frequency to  %lg Hz\n", txFreq);
            tx->setParameter("freq", txFreq);
            lastTxFreq = txFreq;
        }

        return;
    }

    if(c->getId() == rx->getId())
    {
        // This call is from the CRTS Rx filter

        if(rxFreq != lastRxFreq)
        {
            fprintf(stderr, "   Setting RX carrier frequency to  %lg Hz\n", rxFreq);
            rx->setParameter("freq", rxFreq);
            lastRxFreq = rxFreq;
        }

        return;
    }


    // Else this call is from the CRTS joystick filter

    //DSPEW("got joystick input");

    struct js_event *e = (struct js_event *) buffer;

    DASSERT(len % sizeof(struct js_event) == 0, "");

    size_t n = len/sizeof(struct js_event);

    for(size_t i=0; i<n; ++i)
    {
        if(!(e[i].type & JS_EVENT_AXIS) || e[i].number != 0)
            continue;

        txFreq = rxFreq = minFreq +
            (((double) e[i].value - SHRT_MIN)/((double) SHRT_MAX - SHRT_MIN)) * 
            (maxFreq - minFreq);
    }
}


// Define the module loader stuff to make one of these class objects.
CRTSCONTROLLER_MAKE_MODULE(JoystickTxRx)
