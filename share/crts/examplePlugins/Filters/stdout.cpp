#include <stdio.h>

#include "crts/debug.h"
#include "crts/Filter.hpp"
#include "crts/crts.hpp" // for:  FILE *crtsOut in place of stdout


static void usage(void)
{
    char nameBuf[64], *name;
    name = CRTS_BASENAME(nameBuf, 64);

    fprintf(stderr,
"\n"
"Usage: %s\n"
"\n"
"  This is a simple example CRTS Filter module that write to standard output\n"
"  all the data it reads from its' connected input filters.\n"
"\n"
"\n",
    name);

    errno = 0;
    throw "usage help"; // This is how return an error from a C++ constructor
    // the module loader will catch this throw.
}


class Stdout : public CRTSFilter
{
    public:

        Stdout(int argc, const char **argv);
        ~Stdout(void);

        bool start(uint32_t numInChannels, uint32_t numOutChannels);
        bool stop(uint32_t numInChannels, uint32_t numOutChannels);
        void input(void *buffer, size_t bufferLen, uint32_t inChannelNum);
};



Stdout::Stdout(int argc, const char **argv)
{
    CRTSModuleOptions opt(argc, argv, usage);
    // dummy option to add --help|-h
    opt.get("--foo", "");

    DSPEW();
}


Stdout::~Stdout(void)
{
    DSPEW();
}


bool Stdout::start(uint32_t numInChannels, uint32_t numOutChannels)
{
    if(numOutChannels)
    {
        WARN("We do not write output to filters.");
        return true; // fail
    }

    DSPEW();
    return false; // success
}


bool Stdout::stop(uint32_t numInChannels, uint32_t numOutChannels)
{
    DSPEW();
    return false; // success
}


void Stdout::input(void *buffer, size_t len, uint32_t inChannelNum)
{
    errno = 0;

    ASSERT(len, "No data in.");

    size_t ret = fwrite(buffer, 1, len, crtsOut);


    if(ret != len && errno == EINTR)
    {
        // One more try because we there interrupted.
        errno = 0;
        ret += fwrite(buffer, 1, len - ret, crtsOut);
    }

    if(ret != len)
    {
        if(errno == EPIPE)
            // This is very common.
            NOTICE("got broken pipe");
        else
            // The output may be screwed at this point, given we could
            // not write what came in.
            WARN("fwrite(,1,%zu,crtsOut) only wrote %zu bytes", len, ret);

        stream->isRunning = false;
        return;

    }

    if(ret)
    {
        // We may or may not want to do this.
        //
        fflush(crtsOut);
    }
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(Stdout)
