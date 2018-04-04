#include <stdio.h>
#include <values.h>

#include <crts/crts.hpp>
#include <crts/debug.h>
#include <crts/Controller.hpp>
#include <linux/input.h>
#include <linux/joystick.h>

#include "../Filters/txControl.hpp"

class JoystickTx: public CRTSController
{
    public:

        JoystickTx(int argc, const char **argv);

        ~JoystickTx(void) {  DSPEW(); };

        void execute(CRTSControl *c, void * &buffer, size_t &len, uint32_t channelNum);

        void shutdown(CRTSControl *c)
        {
            DSPEW("last use of CRTS Control named \"%s\"", c->getName());
        };

    private:

        TxControl *tx;
        CRTSControl *js;

        double maxFreq, minFreq, freq, lastFreq;
};


// In mega HZ
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
"                      default MAX is %g\n"
"\n"
"\n"
"   --minFreq MIN      set the minimum carrier frequency to MIN Mega Hz. The\n"
"                      default MIN is %g\n"
"\n"
"\n", name, DEFAULT_JSCONTROL_NAME, DEFAULT_TXCONTROL_NAME,
    DEFAULT_MAXFREQ, DEFAULT_MINFREQ);
}


JoystickTx::JoystickTx(int argc, const char **argv)
{
    CRTSModuleOptions opt(argc, argv, usage);
    const char *controlName;

    controlName = opt.get("--controlJs", DEFAULT_TXCONTROL_NAME);
    tx = getControl<TxControl *>(controlName);
    controlName = opt.get("--controlTx", DEFAULT_JSCONTROL_NAME);
    js = getControl<CRTSControl *>(controlName);

    if(tx == 0) throw "could not get TxControl";

    maxFreq = opt.get("--maxFreq", DEFAULT_MAXFREQ) * 1.0e6;
    minFreq = opt.get("--minFreq", DEFAULT_MINFREQ) * 1.0e6;

    freq = lastFreq = maxFreq;

    tx->usrp->set_tx_freq(maxFreq);

    DSPEW("controlJs=\"%s\" controlTx=\"%s\" maxFreq=%lg minFreq=%lg",
            js->getName(), tx->getName(), maxFreq, minFreq);

}



void JoystickTx::execute(CRTSControl *c, void * &buffer,
        size_t &len, uint32_t channelNum)
{
    if(c->getId() != js->getId())
    {
        // This call is from the CRTS Tx filter

        if(freq != lastFreq)
        {
            fprintf(stderr, "   Setting carrier frequency to  %lg Hz\n", freq);
            tx->usrp->set_tx_freq(freq);
            lastFreq = freq;
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

        freq = minFreq +
            (((double) e[i].value - SHRT_MIN)/((double) SHRT_MAX - SHRT_MIN)) * 
            (maxFreq - minFreq);
    }
}


// Define the module loader stuff to make one of these class objects.
CRTSCONTROLLER_MAKE_MODULE(JoystickTx)
