#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <string>
#include <uhd/usrp/multi_usrp.hpp>

#include "crts/debug.h"
#include "crts/Filter.hpp"
#include "crts/crts.hpp"
#include "crts/Control.hpp"
#include "crts/usrp_set_parameters.hpp" // UHD usrp wrappers

#include "defaultUSRP.hpp" // defaults: RX_FREQ, RX_RATE, RX_GAIN

#include "rxControl.hpp"



class Rx : public CRTSFilter
{
    public:

        Rx(int argc, const char **argv);
        ~Rx(void);

        bool start(uint32_t numInChannels, uint32_t numOutChannels);
        bool stop(uint32_t numInChannels, uint32_t numOutChannels);
        void input(void *buffer, size_t len, uint32_t inChannelNum);

    private:

        RxControl rxControl;

        std::string uhd_args, subdev, channels;
        double freq, rate, gain;

        uhd::usrp::multi_usrp::sptr usrp;
        uhd::rx_streamer::sptr rx_stream;

        // Related to output buffer size.
        size_t max_num_samps, numRxChannels;
};



// This is called if the user ran something like: 
//
//    crts_radio -f rx [ --help ]
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
"       crts_radio -f rx [ --uhd addr=192.168.10.3 --freq 932 ] -f stdout\n"
"\n"
"  Most options are used to configure the lib UHD multi_usrp object at startup.\n"
"\n"
"\n"
"  ---------------------------------------------------------------------------\n"
"                           OPTIONS\n"
"  ---------------------------------------------------------------------------\n"
"\n"
"\n"
"   --channels CH   which channel(s) to use (specify \"0\", \"1\", \"0,1\", etc)\n"
"\n"
"\n"
"   --control NAME  set the name of the CRTS control to NAME.  The default value of\n"
"                   NAME is \"" DEFAULT_RXCONTROL_NAME "\".\n"
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
        RX_FREQ, RX_GAIN, RX_RATE, name, name);

    errno = 0;
    throw "usage help"; // This is how return an error from a C++ constructor
    // the module loader will catch this throw.
}


// Get the control name from the command line arguments.
static const char *getControlName(int argc, const char **argv)
{
    CRTSModuleOptions opt(argc, argv, usage);
    return opt.get("--control", DEFAULT_RXCONTROL_NAME);
}


// The constructor gets parameters but does not initialize
// the hardware.  It provides a way to get --help without
// initializing stuff.
//
Rx::Rx(int argc, const char **argv):
    rxControl(this, getControlName(argc, argv), usrp),
    usrp(0), rx_stream(0), max_num_samps(0), numRxChannels(1)
{
    CRTSModuleOptions opt(argc, argv, usage);

    uhd_args = opt.get("--uhd", "");
    freq = opt.get("--freq", RX_FREQ);
    rate = opt.get("--rate", RX_RATE);
    gain = opt.get("--gain", RX_GAIN);
    subdev = opt.get("--subdev", "");
    channels = opt.get("--channels", "");

    // Convert the rate and freq to Hz from MHz
    freq *= 1.0e6;
    rate *= 1.0e6;

    DSPEW();
}


Rx::~Rx(void)
{
    DSPEW();
}


bool Rx::start(uint32_t numInChannels, uint32_t numOutChannels)
{
    DSPEW();
    if(numInChannels != 1)
    {
        WARN("Should have 1 input channel got %" PRIu32, numInChannels);
        return true; // fail
    }

    if(numOutChannels < 1)
    {
        WARN("Should have 1 or more output channels, got %" PRIu32,
            numOutChannels);
        return true; // fail
    }



    if(usrp == 0)
    {
        usrp = uhd::usrp::multi_usrp::make(uhd_args);

        if(subdev.length())
        {
            DSPEW("setting subdev=\"%s\"", subdev.c_str());
            usrp->set_rx_subdev_spec(subdev);
            DSPEW("set subdev");
        }

        //usrp->set_time_now(uhd::time_spec_t(0.0), 0);



        uhd::stream_args_t stream_args("fc32"); //complex floats

        std::vector<size_t> channel_nums;
        for(const char *s=channels.c_str(); *s;)
        {
            char *end = 0;
            errno = 0;
            size_t ch = strtoul(s, &end, 10);
            if(errno)
            {
                WARN("Bad channel number in \"%s\"",
                        channels.c_str());
                return true;
            }
            DSPEW("channel %zu", ch);
            channel_nums.push_back(ch);
            if(!(*end) || end == s) break;
            s = end+1; // go to next
        }

        if(channel_nums.size())
        {
            stream_args.channels = channel_nums;
            numRxChannels = channel_nums.size();
        }
        else
        {
            numRxChannels = 1;
            channel_nums = { 0 };
        }

        rx_stream = usrp->get_rx_stream(stream_args);

        for(size_t i=0; i<channel_nums.size(); ++i)
            if(crts_usrp_rx_set(usrp, freq, rate, gain, channel_nums[i]))
            {
                stop(0,0);
                return true; // fail
            }
    }

    uhd::stream_cmd_t stream_cmd(
            uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    //stream_cmd.stream_now = false;
    //stream_cmd.time_spec = uhd::time_spec_t(seconds_in_future);
    // tell all channels to stream
    rx_stream->issue_stream_cmd(stream_cmd);

    max_num_samps = rx_stream->get_max_num_samps();

    // We use the same ring buffer for all output channels
    //
    createOutputBuffer(max_num_samps *
            numRxChannels * sizeof(std::complex<float>), ALL_CHANNELS);

    DSPEW("usrp->get_pp_string()=\n%s",
            usrp->get_pp_string().c_str());
    DSPEW("max_num_samps=%zu", max_num_samps);

    return false; // success
}


// TODO: UHD has no example code that show how to clean up the usrp
// object.
//
// This function should clean up all the USRP RX software resources or
// whatever like thing.  It's up to you to define what reset means.
//
bool Rx::stop(uint32_t numInChannels, uint32_t numOutChannels)
{
    if(usrp == 0)
    {
        WARN("usrp == 0");
    }
        
    if(rx_stream)
    {
        uhd::stream_cmd_t stream_cmd(
                uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
        // Tell all the channels to stop
        rx_stream->issue_stream_cmd(stream_cmd);

        // ???
        // delete usrp
        //
        //usrp = 0;
        //rx_stream = 0;
    }

    return false; // success
}



void Rx::input(void *buffer, size_t len, uint32_t channelNum)
{
    errno = 0;

    // This filter is a source so there no data passed to
    // whatever called this write()
    //
    // TODO:  or we could just ignore the input buffer??
    DASSERT(len == 0,
            "This is just a source filter len=%zu", len);

    buffer = getOutputBuffer(0);

    uhd::rx_metadata_t metadata; // set by recv();

    size_t numSamples = rx_stream->recv(
            buffer, max_num_samps, metadata, 2.0);


    if(numSamples != max_num_samps)
    {
        if(uhd::rx_metadata_t::ERROR_CODE_TIMEOUT ==
                metadata.error_code)
            NOTICE("RX recv TIMEOUT numSamples=%zu", numSamples);
        else
            NOTICE("RX recv error_code=%d numSamples=%zu",
                metadata.error_code, numSamples);
    }

    if(numSamples)
        output(numSamples*numRxChannels*
                sizeof(std::complex<float>),
                CRTSFilter::ALL_CHANNELS);
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(Rx)
