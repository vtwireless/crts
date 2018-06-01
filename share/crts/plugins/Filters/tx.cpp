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

        std::string uhd_args, subdev;
        std::vector<size_t> channels;
        std::vector<double> freq, rate, gain;

        uhd::usrp::multi_usrp::sptr usrp;
        uhd::tx_streamer::sptr tx_stream;

        uhd::tx_metadata_t metadata;

        size_t numTxChannels;
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
"       crts_radio -f tx [ --uhd addr=192.168.10.3 --freq 932 ] -f stdout\n"
"\n"
"  Most options are used to configure the lib UHD multi_usrp object at startup.\n"
"\n"
"\n"
"  ---------------------------------------------------------------------------\n"
"                           OPTIONS\n"
"  ---------------------------------------------------------------------------\n"
"\n"
"\n"
"                  FREQ, GAIN, and RATE may be a single number or a list of\n"
"                  numbers with each number separated by a comma (,).\n"
"\n"
"\n"
"   --channels CH   which channel(s) to use (specify \"0\", \"1\", \"0,1\", etc)\n"
"\n"
"\n"
"   --control NAME  set the name of the CRTS control to NAME.  The default value of\n"
"                   NAME is \"" DEFAULT_TXCONTROL_NAME "\".\n"
"\n"
"\n"
"   --freq FREQ     set the initial receiver frequency to FREQ MHz.  The default\n"
"                   initial receiver frequency is %g MHz.\n"
"\n"
"\n"
"   --gain GAIN     set the initial receiver gain to GAIN.  The default initial\n"
"                   receiver gain is %g.\n"
"\n"
"\n"
"   --rate RATE     set the initial receiver sample rate to RATE million samples\n"
"                   per second.  The default initial receiver rate is %g million\n"
"                   samples per second.\n"
"\n"
"\n"
"   --subdev DEV    UHD subdev spec.\n"
"\n"
"                           Example: %s [ --subdev \"0:A 0:B\" ]\n"
"\n"
"                   to get 2 channels on a Basic RX.\n"
"\n"
"\n"
"   --uhd ARGS      set the arguments to give to the uhd::usrp constructor.\n"
"\n"
"                                 Example: %s [ --uhd addr=192.168.10.3 ]\n"
"\n"
"                   will use the USRP (Universal Software Radio Peripheral)\n"
"                   which is accessible at Ethernet IP4 address 192.168.10.3\n"
"\n"
"\n",
        name,
        TX_FREQ, TX_GAIN, TX_RATE, name, name);

    errno = 0;
    throw "usage help";
    // This is how return an error from a C++ constructor
    // the module loader will catch this throw.
}



static const char *getControlName(int argc, const char **argv)
{
    CRTSModuleOptions opt(argc, argv, usage);

    return opt.get("--control", DEFAULT_TXCONTROL_NAME);
}



Tx::Tx(int argc, const char **argv):
    txControl(this, getControlName(argc, argv), usrp),
    usrp(0), tx_stream(0), numTxChannels(1)
{
    CRTSModuleOptions opt(argc, argv, usage);

    uhd_args = opt.get("--uhd", "");
    freq = opt.getV<double>("--freq", TX_FREQ);
    rate = opt.getV<double>("--rate", TX_RATE);
    gain = opt.getV<double>("--gain", TX_GAIN);
    channels = opt.getV<size_t>("--channels", 0);
    subdev = opt.get("--subdev", "");

    // Convert the rate and freq to Hz from MHz
    for(size_t i=0; i<freq.size(); ++i)
        freq[i] *= 1.0e6;
    for(size_t i=0; i<rate.size(); ++i)
        rate[i] *= 1.0e6;

    DSPEW();
}


Tx::~Tx(void)
{
    // TODO: delete usrp device; how?

    DSPEW();
}


static double getVal(std::vector<double> vec, size_t i)
{
    ASSERT(vec.size(), "");

    if(vec.size() > i)
        return vec[i];

    // return the last element
    return vec[vec.size() -1];
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
        // TODO: We need to add the ability to vary freq, gain, and rate
        // for each channel.

        // TODO: try catch etc.  The crts_radio code will catch any thrown
        // exceptions, but will not know what to do but fail which I
        // guess is fine.
        //
        usrp = uhd::usrp::multi_usrp::make(uhd_args);

        if(subdev.length())
        {
            DSPEW("setting subdev=\"%s\"", subdev.c_str());
            usrp->set_tx_subdev_spec(subdev);
            DSPEW("set subdev");
        }

        //usrp->set_time_now(uhd::time_spec_t(0.0), 0);


        uhd::stream_args_t stream_args("fc32", "sc16");

        stream_args.channels = channels;
        numTxChannels = channels.size();

        tx_stream = usrp->get_tx_stream(stream_args);

        for(size_t i=0; i<channels.size(); ++i)
            if(crts_usrp_tx_set(usrp,
                        getVal(freq,i),
                        getVal(rate,i),
                        getVal(gain,i), channels[i]))
            {
                stop(0,0);
                return true; // fail
            }
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
        // TODO: WTF ??
        //delete usrp;
        usrp = 0;
        tx_stream = 0;
    }

    return false; // success
}




// len is the number of bytes not complex floats or frames.
//
void Tx::input(void *buffer, size_t len, uint32_t channelNum)
{
    // We must write integer number of std::complex<float> 
    len -= len % (numTxChannels*sizeof(std::complex<float>));

    size_t numFrames = len/(numTxChannels*sizeof(std::complex<float>));

    // TODO: check for error here, and retry?
    size_t ret = tx_stream->send(buffer, numFrames, metadata);

    if(ret != numFrames)
        WARN("wrote only %zu frames not %zu", ret, numFrames);

    // Mark the number of bytes to advance the input buffer which is not
    // necessarily the same as "len" that was inputted.
    //
    advanceInput(len);

    if(metadata.start_of_burst)
        // In all future Tx::write() calls this is false.
        metadata.start_of_burst = false;
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(Tx)
