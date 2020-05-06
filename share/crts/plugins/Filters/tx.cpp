#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <string>
#include <uhd/usrp/multi_usrp.hpp>
#include <liquid/liquid.h>

#include "crts/debug.h"
#include "crts/Filter.hpp"
#include "crts/crts.hpp"
#include "crts/usrp_set_parameters.hpp" // UHD usrp wrappers

#include "defaultUSRP.hpp" // defaults: TX_FREQ, TX_RATE, TX_GAIN



class Tx : public CRTSFilter
{
    public:

        Tx(int argc, const char **argv);
        ~Tx(void);

        bool start(uint32_t numInChannels, uint32_t numOutChannels);
        bool stop(uint32_t numInChannels, uint32_t numOutChannels);
        void input(void *buffer, size_t len, uint32_t inChannelNum);

    private:

        // These will store values from before start() to
        // initialize in start().  Not to be confused with
        // what the current values are at running time.
        // They are just for initialize in start() that
        // may be gotten from the command line.
        std::vector<double> freq, rate, gain;


        std::string uhd_args, subdev;
        std::vector<size_t> channels; // USRP channels

        // Stupid libuhd state; they spill into three
        // objects that we need to keep.
        uhd::usrp::multi_usrp::sptr usrp;
        uhd::tx_streamer::sptr tx_stream;
        uhd::tx_metadata_t metadata;

        // This is the libuhd URRP channels not a CRTS filter/stream
        // channel.  It will be the size of the std::vector<> variables
        // above.
        size_t numTxChannels;

        // Arbitrary rate resampler to allow dynamic bandwidth adjustment
        resamp_crcf resamp;
        std::complex<float> * buffer_resamp; // TODO: use std lib container instead?

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
            usrp->set_tx_freq(tune, chan);
            return true;
        };

        bool setRate(const double &r, size_t chan=0)
        {
            // TODO: check return?
            //
            usrp->set_tx_rate(r, chan);
            return true;
        };

        bool setRateResamp(const double &r, size_t chan=0)
        {
            if (r < 0.01 || r > 1.0) {
                WARN("software resampling rate must be in [0.01, 1]");
                return false;
            }
            resamp_crcf_set_rate(resamp, (float)r);
            return true;
        };

        bool setGain(const double &g, size_t chan=0)
        {
            // TODO: check return?
            //
            usrp->set_tx_gain(g, chan);
            return true;
        };

        double getFreq(size_t chan=0)
        {
            // TODO: error check??
            //
            return usrp->get_tx_freq(chan);
        };

        double getRate(size_t chan=0)
        {
            // TODO: error check??
            //
            return usrp->get_tx_rate(chan);
        };

        double getGain(size_t chan=0)
        {
            // TODO: error check??
            //
            return usrp->get_tx_gain(chan);
        };
};


// This is called if the user ran something like: 
//
//    crts_radio -f tx [ --help ]
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
"                   to get 2 channels on a Basic TX.\n"
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



Tx::Tx(int argc, const char **argv):
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

    // instantiate arbitrary rate resampler and appropriate output buffer
    resamp = resamp_crcf_create(1.0f, 12, 0.495f, 60.0f, 64);
    buffer_resamp = new std::complex<float>[2048];

    DSPEW();
}


Tx::~Tx(void)
{
    // TODO: delete usrp device; how?
    // I have not found that in the libuhd documentation
    // or the libuhd examples.
    // Or should stop() be the place to cleanup most of the
    // libuhd stuff.

    // destroy arbitrary rate resampler and free output buffer
    resamp_crcf_destroy(resamp);
    delete [] buffer_resamp;

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
                        freq[i],
                        rate[i],
                        gain[i], channels[i]))
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

    // resample result; note, because the resampler is restricted to always decimate,
    // the size of the output buffer only needs to be at least the same size as the input
    // TODO: increase output buffer size appropriately
    unsigned int numFramesResamp;
    resamp_crcf_execute(resamp, buffer, numFrames, buffer_resamp, &numFramesResamp);

    // TODO: check for error here, and retry?
    size_t ret = tx_stream->send(buffer_resamp, numFramesResamp, metadata);

    if(ret != numFramesResamp)
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
