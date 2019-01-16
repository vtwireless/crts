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


class Rx : public CRTSFilter
{
    public:

        Rx(int argc, const char **argv);
        ~Rx(void);

        bool start(uint32_t numInChannels, uint32_t numOutChannels);
        bool stop(uint32_t numInChannels, uint32_t numOutChannels);
        void input(void *buffer, size_t len, uint32_t inChannelNum);

    private:

        std::string uhd_args, subdev;
        std::vector<size_t> channels;
        std::vector<double> freq, rate, gain;

        uhd::usrp::multi_usrp::sptr usrp;
        uhd::rx_streamer::sptr rx_stream;

        // Related to output buffer size.
        size_t max_num_samps, numRxChannels;

        
        ////////////////////////////////////////////////////////////////
        // Inline helper/wrapper utilities to set and get parameters
        // for this filter.
        ////////////////////////////////////////////////////////////////

        bool setFreq(const double &f, size_t chan=0)
        {
            uhd::tune_request_t tune;
            tune.rf_freq_policy = uhd::tune_request_t::POLICY_MANUAL;
            tune.dsp_freq_policy = uhd::tune_request_t::POLICY_MANUAL;
            tune.rf_freq = f;
            tune.dsp_freq = 0;

            // TODO: check the return value???
            //uhd::tune_result_t result =
            //
            usrp->set_rx_freq(tune, chan);
            return true;
        };

        bool setRate(const double &r, size_t chan=0)
        {
            // TODO: check return?
            //
            usrp->set_rx_rate(r, chan);
            return true;
        };

        bool setGain(const double &g, size_t chan=0)
        {
            // TODO: check return?
            //
            usrp->set_rx_gain(g, chan);
            return true;
        };

        double getFreq(size_t chan=0)
        {
            // TODO: error check??
            //
            return usrp->get_rx_freq(chan);
        };

        double getRate(size_t chan=0)
        {
            // TODO: error check??
            //
            return usrp->get_rx_rate(chan);
        };

        double getGain(size_t chan=0)
        {
            // TODO: error check??
            //
            return usrp->get_rx_gain(chan);
        };

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
"                  FREQ, GAIN, and RATE may be a single number or a list of\n"
"                  numbers with each number separated by a comma (,).\n"
"\n"
"\n"
"   --channels CH   which channel(s) to use (specify \"0\", \"1\", \"0,1\", etc)\n"
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


// The constructor gets parameters but does not initialize
// the hardware.  It provides a way to get --help without
// initializing stuff.
//
Rx::Rx(int argc, const char **argv):
    usrp(0), rx_stream(0), max_num_samps(0), numRxChannels(1)
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

    ////////////////////////////////////////////////////////////////////////
    // Check that USRP parameter vectors are consistent, that is, we have
    // the same same number of freq, rate, gain as there are USRP
    // channels.
    ////////////////////////////////////////////////////////////////////////


    if(channels.size() != freq.size())
    {
        ERROR("The number of frequencies, %zu, is "
            "not the same as the number of channels, %zu",
            channels.size(), freq.size());
        throw "number of frequencies != number of channels";
    }

    DASSERT(rate.size() >= 1, "no rates");
    ASSERT(rate.size() <= channels.size(), "more rates than channels");
    DASSERT(gain.size() >= 1, "no gains");
    ASSERT(gain.size() <= channels.size(), "more gains than channels");

    if(channels.size() > rate.size())
    {
        INFO("The number of rates, %zu, is "
            "not the same as the number of channels, %zu,"
            " so we'll add some rates copying the last one",
            channels.size(), rate.size());
        size_t lastI = rate.size()-1;
        for(size_t i=rate.size(); i<channels.size(); ++i)
            rate[i] = rate[lastI];

    }

    if(channels.size() > gain.size())
    {
        INFO("The number of gains, %zu, is "
            "not the same as the number of channels, %zu,"
            " so we'll add some gains copying the last one",
            channels.size(), gain.size());
        size_t lastI = gain.size()-1;
        for(size_t i=gain.size(); i<channels.size(); ++i)
            gain[i] = gain[lastI];
    }

    /////////////////////////////////////////////////////////////////
    // Setup parameters set and get callbacks for freq, rate, gain
    /////////////////////////////////////////////////////////////////

    if(channels.size() == 1)
    {
        // Simple case where we have just one freq, rate, and gain
        // parameter.
        //
        addParameter("freq",
                [&]() { return getFreq(); },
                [&](double x) { return setFreq(x); }
        );
        addParameter("rate",
                [&]() { return getRate(); },
                [&](double x) { return setRate(x); }
        );
        addParameter("gain",
                [&]() { return getGain(); },
                [&](double x) { return setGain(x); }
        );
    }
    else
    {
        // The more complex case where we have more than one USRP channel
        // making more than one freq, rate, and gain parameters, so we
        // must generate names like: freq0, freq1, freq2, ... one for each
        // USRP channel:
        //
        for(size_t i=0; i<channels.size(); ++i)
        {
            std::string s;
            s = "freq";
            s += std::to_string(i); // freq0, freq1, freq2, freq3, ...
            addParameter(s,
                [&]() { return getFreq(channels[i]); },
                [&](double x) { return setFreq(x, channels[i]); }
            );
            s = "rate";
            s += std::to_string(i); // rate0, rate1, rate2, rate3, ...
            addParameter(s,
                [&]() { return getRate(channels[i]); },
                [&](double x) { return setRate(x, channels[i]); }
            );
            s = "gain";
            s += std::to_string(i); // gain0, gain1, gain2, gain3, ...
            addParameter(s, 
                [&]() { return getGain(channels[i]); },
                [&](double x) { return setGain(x, channels[i]); }
            );
        }
    }

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

        rx_stream = usrp->get_rx_stream(stream_args);

        for(size_t i=0; i<channels.size(); ++i)
            if(crts_usrp_rx_set(usrp,
                        freq[i],
                        rate[i],
                        gain[i], channels[i]))
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
