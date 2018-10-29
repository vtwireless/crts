#include <stdio.h>
#include <values.h>

#include <crts/crts.hpp>
#include <crts/debug.h>
#include <crts/Controller.hpp>
#include <linux/input.h>
#include <linux/joystick.h>
#include <atomic>


class JoystickTx: public CRTSController
{
    public:

        JoystickTx(int argc, const char **argv);

        ~JoystickTx(void) {  DSPEW(); };

        void start(CRTSControl *c);
        void stop(CRTSControl *c);

        void execute(CRTSControl *c, const void *buffer, size_t len,
                uint32_t channelNum);

    private:

        CRTSControl *tx;
        CRTSControl *js;

        double maxFreq, minFreq, lastFreq;

        std::atomic<double> freq;
};


// in mega-herz (M Hz)
#define DEFAULT_MAXFREQ (915.5)
#define DEFAULT_MINFREQ (914.5)

#define DEFAULT_TXCONTROL_NAME            "tx"
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
"\n", name, DEFAULT_JSCONTROL_NAME, DEFAULT_TXCONTROL_NAME,
    DEFAULT_MAXFREQ, DEFAULT_MINFREQ);
}


JoystickTx::JoystickTx(int argc, const char **argv)
{
    CRTSModuleOptions opt(argc, argv, usage);
    const char *controlName;

    controlName = opt.get("--controlTx", DEFAULT_TXCONTROL_NAME);
    tx = getControl<CRTSControl *>(controlName);
    controlName = opt.get("--controlJs", DEFAULT_JSCONTROL_NAME);
    js = getControl<CRTSControl *>(controlName);

    maxFreq = opt.get("--maxFreq", DEFAULT_MAXFREQ) * 1.0e6;
    minFreq = opt.get("--minFreq", DEFAULT_MINFREQ) * 1.0e6;

    freq = lastFreq = maxFreq;
    DSPEW();
}


void JoystickTx::start(CRTSControl *c)
{
    if(c->getId() != js->getId())
    {
        // This call is from the CRTS Tx filter
        tx->setParameter("freq", maxFreq);
        // TODO: check that the freq was really set.
    }

    DSPEW("controlJs=\"%s\" controlTx=\"%s\" maxFreq=%lg minFreq=%lg",
            js->getName(), tx->getName(), maxFreq, minFreq);
}


void JoystickTx::stop(CRTSControl *c)
{
    DSPEW();
}


// This function is called by two different threads one from the Tx filter
// and one from the joystick filter, hence the shared variable freq is
// atomic.
//
void JoystickTx::execute(CRTSControl *c, const void *buffer,
        size_t len, uint32_t channelNum)
{
    if(c->getId() != js->getId())
    {
        // This call is from the CRTS Tx filter

        // atomic get freq.
        double f = freq;

        if(f != lastFreq)
        {
            fprintf(stderr, "   Setting carrier frequency to  %lg Hz\n", f);
            tx->setParameter("freq", f);
            lastFreq = f;
        }

        return;
    }

    // Else this call is from the CRTS joystick filter

    //DSPEW("got joystick input");

    // In this case we are getting the joystick event data from the
    // stream.  That may not be the only and best way to do this.

    struct js_event *e = (struct js_event *) buffer;

    DASSERT(len % sizeof(struct js_event) == 0, "");

    size_t n = len/sizeof(struct js_event);

    for(size_t i=0; i<n; ++i)
    {
        if(!(e[i].type & JS_EVENT_AXIS) || e[i].number != 0)
            continue;

        // atomic set freq.
        //
        freq = minFreq +
            (((double) e[i].value - SHRT_MIN)/((double) SHRT_MAX - SHRT_MIN)) * 
            (maxFreq - minFreq);
    }
}


// Define the module loader stuff to make one of these class objects.
CRTSCONTROLLER_MAKE_MODULE(JoystickTx)
