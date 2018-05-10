#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <string>
#include <uhd/usrp/multi_usrp.hpp>

#include "crts/debug.h"
#include "crts/Filter.hpp"
#include "crts/crts.hpp"
#include "crts/usrp_set_parameters.hpp" // UHD usrp wrappers

#include "defaultUSRP.hpp" // defaults: TX_FREQ, TX_RATE, TX_GAIN

#include "txControl.hpp"


class Tx : public CRTSFilter
{
    public:

        Tx(int argc, const char **argv);
        ~Tx(void);

        bool start(uint32_t numInChannels, uint32_t numOutChannels);
        bool stop(uint32_t numInChannels, uint32_t numOutChannels);
        void input(void *buffer, size_t len, uint32_t inChannelNum);

    private:

        TxControl txControl;

        std::string uhd_args;
        double freq, rate, gain;

        uhd::usrp::multi_usrp::sptr usrp;
        uhd::tx_streamer::sptr tx_stream;

        uhd::tx_metadata_t metadata;

};


// This is called if the user ran something like: 
//
//    crts_radio -f file [ --help ]
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
"  As an example you can run something like this:\n"
"\n"
"       crts_radio -f stdin -f tx [ --uhd addr=192.168.10.3 --freq 932 ]\n"
"\n"
"\n"
"  ---------------------------------------------------------------------------\n"
"                           OPTIONS\n"
"  ---------------------------------------------------------------------------\n"
"\n"
"\n"
"   --uhd ARGS      set the arguments to give to the uhd::usrp constructor.\n"
"\n"
"                                 Example: %s [ --uhd addr=192.168.10.3 ]\n"
"\n"
"                   will use the USRP (Universal Software Radio Peripheral)\n"
"                   which is accessible at Ethernet IP4 address 192.168.10.3\n"
"\n"
"\n"
"   --freq FREQ     set the initial transmitter frequency to FREQ MHz.  The default\n"
"                   initial transmitter frequency is %g MHz.\n"
"\n"
"\n"
"   --gain GAIN     set the initial transmitter gain to GAIN.  The default initial\n"
"                   transmitter gain is %g.\n"
"\n"
"\n"
"   --rate RATE     set the initial transmitter sample rate to RATE million samples\n"
"                   per second.  The default initial transmitter rate is %g million\n"
"                   samples per second.\n"
"\n"
"\n"
"   --control NAME  set the name of the CRTS control to NAME.  The default value of\n"
"                   NAME is \"%s\".\n"
"\n"
"\n",
        name, name,
        TX_FREQ, TX_GAIN, TX_RATE, DEFAULT_TXCONTROL_NAME);

    errno = 0;
    throw "usage help"; // This is how return an error from a C++ constructor
    // the module loader will catch this throw.
}



static const char *getControlName(int argc, const char **argv)
{
    CRTSModuleOptions opt(argc, argv, usage);

    return opt.get("--control", DEFAULT_TXCONTROL_NAME);
}



Tx::Tx(int argc, const char **argv):
    txControl(this, getControlName(argc, argv), usrp),
    usrp(0), tx_stream(0)
{
    CRTSModuleOptions opt(argc, argv, usage);

    uhd_args = opt.get("--uhd", "");
    freq = opt.get("--freq", TX_FREQ);
    rate = opt.get("--rate", TX_RATE);
    gain = opt.get("--gain", TX_GAIN);

    // Convert the rate and freq to Hz from MHz
    freq *= 1.0e6;
    rate *= 1.0e6;

    DSPEW();
}


Tx::~Tx(void)
{
    // TODO: delete usrp device; how?

    DSPEW();
}


bool Tx::start(uint32_t numInChannels, uint32_t numOutChannels)
{
    DSPEW();

    if(numOutChannels != 0)
    {
        WARN("Should have no output channels got %" PRIu32,
            numOutChannels);
        return true; // fail
    }

    if(usrp == 0)
    {
        // TODO: try catch etc.  The crts_radio code will catch any thrown
        // exceptions, but will not know what to do but fail which I
        // guess is fine.
        //
        usrp = uhd::usrp::multi_usrp::make(uhd_args);

        if(crts_usrp_tx_set(usrp, freq, rate, gain))
        {
            stop(0,0);
            return true; // fail
        }

        uhd::stream_args_t stream_args("fc64", "sc16");
        tx_stream = usrp->get_tx_stream(stream_args);
    }

    metadata.start_of_burst = true;
    metadata.end_of_burst = false;
    metadata.has_time_spec = false; // set to false to send immediately

    DSPEW("TX is initialized usrp->get_pp_string()=\n%s",
            usrp->get_pp_string().c_str());

    return false; // success
}


bool Tx::stop(uint32_t numInChannels, uint32_t numOutChannels)
{
    if(usrp)
    {
        // WTF ??
        //delete usrp;
        usrp = 0;
        tx_stream = 0;
    }

    return false; // success
}




// len is the number of bytes not complex floats.
//
void Tx::input(void *buffer, size_t len, uint32_t channelNum)
{
    // We must write integer number of std::complex<float> 
    len -= len % sizeof(std::complex<float>);

    // TODO: check for error here, and retry?
    size_t ret = tx_stream->send(buffer, len/sizeof(std::complex<float>),
            metadata);

    if(ret != len/sizeof(std::complex<float>))
    {
        WARN("wrote only %zu of complex values not %zu", ret,
                len/sizeof(std::complex<float>));
    }

    // Mark the number of bytes to advance the input buffer which is not
    // necessarily the same as what was inputted.
    //
    advanceInput(len);

    if(metadata.start_of_burst)
        // In all future Tx::write() calls this is false.
        metadata.start_of_burst = false;
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(Tx)
