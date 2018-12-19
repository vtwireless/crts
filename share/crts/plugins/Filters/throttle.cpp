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
"   TODO: currently the number of inputs must be the same as the number of outputs.\n"
"   With a little more code that restriction could be removed.\n"
"\n"
"  ---------------------------------------------------------------------------\n"
"                           OPTIONS\n"
"  ---------------------------------------------------------------------------\n"
"\n"
"\n"
"   --bytes BYTES     number of bytes written per period.  The default is %zu\n"
"\n"
"\n"
"   --period SEC      time to delay in seconds before outputing data. The default\n"
"                     is %g seconds.\n"
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
        bool stop(uint32_t numInChannels, uint32_t numOutChannels);
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
};


Throttle::Throttle(int argc, const char **argv)
{
    CRTSModuleOptions opt(argc, argv, usage);

    bytes = opt.get("--bytes", DEFAULT_BYTES);
    double seconds = opt.get("--period", DEFAULT_PERIOD);
    period.tv_sec = seconds;
    period.tv_nsec = (seconds - period.tv_sec)*(1.0e+9);

    addParameter("bytes",
                [&]() { return getBytes(); },
                [&](double x) { return setBytes(x); }
    );

    addParameter("period",
                [&]() { return getPeriod(); },
                [&](double x) { return setPeriod(x); }
    );

    DSPEW();
}


bool Throttle::start(uint32_t numInChannels, uint32_t numOutChannels)
{
    ASSERT(numInChannels && numInChannels == numOutChannels,
            "The number of input channels is not the same as"
            " the number of output channels");

    createPassThroughBuffer(0, 0,
                bytes /*maxBufferLen promise*/, bytes/*threshold amount*/);

    for(uint32_t i=1; i<numOutChannels; ++i)
        createPassThroughBuffer(i, i,
                bytes /*maxBufferLen promise*/, bytes);

    setChokeLength(bytes, ALL_CHANNELS);

    rem = {0,0};

    DSPEW();
    return false; // success
}


bool Throttle::stop(uint32_t numInChannels, uint32_t numOutChannels)
{
    DSPEW();
    return false; // success
}


static inline
struct timespec AddTimes(const struct timespec a, const struct timespec b)
{
    time_t seconds = a.tv_sec + b.tv_sec +
        (a.tv_nsec + b.tv_nsec)/1000000000.0F;

    return
    {
        seconds,
        (a.tv_nsec + b.tv_nsec) - seconds * 1000000000 // nano seconds
    };
}


void Throttle::input(void *buffer, size_t len, uint32_t inChannelNum)
{
    DASSERT(bytes >= len,"");

    if(inChannelNum != 0)
    {
        // For channel != 0 we just pass through.
        //
        output(len, inChannelNum);
        return;
    }

    if(!stream->isRunning)
    {
        // Just flush it without delay if we are finishing up.
        output(len, 0);
        return;
    }

    // inChannelNum == 0
    //
    struct timespec t = AddTimes(period, rem);
    errno = 0;
    // TODO: consider using an interval timer instead.
    //
    // TODO: We could be interrupted more than once.
    //
    if(nanosleep(&t, &rem) == -1 && errno == EINTR &&
            (rem.tv_sec || rem.tv_nsec))
    {
        t = rem;
        rem = { 0, 0 };
        nanosleep(&t, &rem);
    }

    output(len, 0);
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
