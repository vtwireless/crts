#include <unistd.h>

#include "crts/crts.hpp"
#include "crts/debug.h"
#include "crts/Filter.hpp"

#define DEFAULT_USLEEP ((useconds_t) 1000000) // 1 usec = (1/1000000) second


static void usage(void)
{
    char nameBuf[64], *name;
    name = CRTS_BASENAME(nameBuf, 64);

    fprintf(stderr,
"\n"
"\n"
"Usage: %s [ OPTIONS ]\n"
"\n"
"\n"
"            OPTIONS\n"
"\n"
"    --utime USECS  set the sleep time in micro seconds to USECS micro seconds.\n"
"                   The default sleep time is %" PRIu32 " micro seconds.\n"
"\n",
    name, DEFAULT_USLEEP);

    errno = 0;
    throw "usage help"; // This is how return an error from a C++ constructor
    // the module loader will catch this throw.
}



class SleepPassThrough : public CRTSFilter
{
    public:

        SleepPassThrough(int argc, const char **argv);
        ~SleepPassThrough(void);

        bool start(uint32_t numInChannels, uint32_t numOutChannels);
        bool stop(uint32_t numInChannels, uint32_t numOutChannels);
        void input(void *buffer, size_t bufferLen, uint32_t inChannelNum);

    private:

    useconds_t usleepTime;


};



SleepPassThrough::SleepPassThrough(int argc, const char **argv)
{
    CRTSModuleOptions opt(argc, argv, usage);

    usleepTime = opt.get("--utime", DEFAULT_USLEEP);

    // Check stupid size crap.
    DASSERT(sizeof(useconds_t) == sizeof(uint32_t), "");

    DSPEW();
}


bool SleepPassThrough::start(uint32_t numInChannels, uint32_t numOutChannels)
{
    if(isSource())
    {
        WARN("This filter should not be a source.");
        return true; // fail
    }

    if(numInChannels != numOutChannels)
    {
        WARN("This filter must have numInChannels("
                PRIu32 ") == numOutChannels(" PRIu32
                ")", numInChannels, numOutChannels);
        return true; // fail
    }

    for(uint32_t i=0; i<numInChannels; ++i)
        createPassThroughBuffer(i/*input channel*/,
                i/*output channel*/,
                1 /*maxBufferLen promise*/, 1 /*threshold*/);
    DSPEW();
    return false; // success
}

bool SleepPassThrough::stop(uint32_t numInChannels, uint32_t numOutChannels)
{
    // Reset the filter parameters, nothing in this case.
    DSPEW();
    return false; // success
}


void SleepPassThrough::input(void *buffer, size_t len, uint32_t inChannelNum)
{
    if(usleepTime)
        ASSERT(usleep(usleepTime) == 0, "");

    if(len)
        output(len, inChannelNum);
}


SleepPassThrough::~SleepPassThrough(void)
{
    // stop() should have been called if start was ever called,
    // so we have nothing to do here in this destructor.
    //
    DSPEW();
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(SleepPassThrough)
