#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include "crts/crts.hpp"
#include "crts/debug.h"
#include "crts/Filter.hpp"


#define DEFAULT_BYTES  ((size_t) 512)    // Bytes

#define DEFAULT_PERIOD  ((double) 0.1) // Seconds




static void usage(void)
{
    char nameBuf[64], *name;
    name = CRTS_BASENAME(nameBuf, 64);

    fprintf(stderr,
"\n"
"\n"
"Usage: %s [ OPTIONS ]\n"
"\n"
"   This is a choke, throttle, or slow downer.  It restricts the flow of data by\n"
"   blocking the first channel (0) the running of this filters thread for some time\n"
"   for each input.\n"
"\n"
"   This filter must not be a source filter and it must have one input channel.\n"
"   There may be any number of outputs including zero.  The input will be sprayed\n"
"   to all output channels.  In the case with no output, this filter just serves to\n"
"   delay (throttle) the consumption of the input, bottle-necking the flow at the\n"
"   end.\n"
"\n"
"  ---------------------------------------------------------------------------\n"
"                           OPTIONS\n"
"  ---------------------------------------------------------------------------\n"
"\n"
"\n"
"   --bytes BYTES     number of bytes written per period.  The default is %zu .\n"
"                     BYTES may not be changed while the stream is running.\n"
"\n"
"\n"
"   --period SEC      time to delay in seconds before outputing data. The default\n"
"                     is %g seconds.  The period can be changed while the stream is\n"
"                     running.\n"
"\n",
    name, DEFAULT_BYTES, DEFAULT_PERIOD);

    errno = 0;
    throw "usage help"; // This is how return an error from a C++ constructor
    // the module loader will catch this throw.
}



class Throttle : public CRTSFilter
{
    public:

        Throttle(int argc, const char **argv);
        ~Throttle(void);

        bool start(uint32_t numInChannels, uint32_t numOutChannels);
        void input(void *buffer, size_t bufferLen, uint32_t inChannelNum);

    private:

        bool setBytes(double bytes_in)
        {
            size_t b = bytes_in;
            if(b < 1)
                return false;

            bytes = b;
            return true;
        };

        double getBytes(void)
        {
            return (double) bytes;
        };

        bool setPeriod(double period_in)
        {
            if(period_in < 1.0e-9 || period_in > 1.0e+8)
                return false;
    
            period.tv_sec = period_in;
            period.tv_nsec = (period_in - period.tv_sec)*(1.0e+9);

            return true;
        };

        double getPeriod(void)
        {
            return (double) period.tv_sec + period.tv_nsec/1000000000.0F;
        };

        size_t bytes;
        struct timespec period, rem; // Time to sleep and remaining time
        uint32_t numOutputs;
};


Throttle::Throttle(int argc, const char **argv)
{
    CRTSModuleOptions opt(argc, argv, usage);

    bytes = opt.get("--bytes", DEFAULT_BYTES);
    double seconds = opt.get("--period", DEFAULT_PERIOD);
    period.tv_sec = seconds;
    period.tv_nsec = (seconds - period.tv_sec)*(1.0e+9);

    // We can change the period on the fly, but we can't change the chunk
    // size (bytes) on the fly (while the stream is running); because
    // we can't change the ring buffers while it is running.

    addParameter("period",
                [&]() { return getPeriod(); },
                [&](double x) { return setPeriod(x); }
    );

    DSPEW();
}


bool Throttle::start(uint32_t numInChannels, uint32_t numOutChannels)
{
    ASSERT(!isSource() && numInChannels == 1,
            "The number of input channels is not the same as"
            " the number of output channels");

    numOutputs = numOutChannels;

    if(numOutputs)
        createPassThroughBuffer(0, 0,
                bytes /*maxBufferLen promise*/, bytes/*threshold amount*/);

    for(uint32_t i=1; i<numOutChannels; ++i)
        createPassThroughBuffer(i, i,
                bytes /*maxBufferLen promise*/, bytes);

    // We will never input() more than bytes.
    setChokeLength(bytes, ALL_CHANNELS);

    rem = {0,0};

    DSPEW();
    return false; // success
}



static inline
struct timespec AddTimes(const struct timespec a, const struct timespec b)
{
    long nsec = a.tv_nsec + b.tv_nsec;
    time_t seconds = a.tv_sec + b.tv_sec;

    if(nsec > 1000000000)
    {
        time_t rem = nsec/1000000000.0F;
        seconds += rem;
        nsec -= rem * 1000000000;
    }

    return
    {
        seconds,
        nsec // nano seconds
    };
}


void Throttle::input(void *buffer, size_t len, uint32_t inChannelNum)
{
    DASSERT(inChannelNum == 0, "");
    DASSERT(bytes >= len,"");

    if(stream->isRunning)
    {
        DASSERT(bytes == len,"");

        struct timespec t = AddTimes(period, rem);
        errno = 0;

SPEW("t.tv_sec=%ld  t.tv_nsec=%ld", t.tv_sec, t.tv_nsec);

        // TODO: consider using an interval timer instead.
        //
        // TODO: We could be interrupted more than once.
        //
        if(nanosleep(&t, &rem) == -1 && errno == EINTR &&
                (rem.tv_sec || rem.tv_nsec))
        {
            t = rem;
            rem = { 0, 0 };
            // We where interrupted so do it again.
            nanosleep(&t, &rem);
        }
    }

    if(numOutputs)
        output(len, 0);

    //else // This is done automatically.
        //advanceInput(len);
}


Throttle::~Throttle(void)
{
    // stop() should have been called if start was ever called,
    // so we have nothing to do here in this destructor.
    //
    DSPEW();
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(Throttle)
