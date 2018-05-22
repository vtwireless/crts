#include <stdio.h>

#include "crts/debug.h"
#include "crts/crts.hpp"
#include "crts/Filter.hpp"


class Sink : public CRTSFilter
{
    public:

        Sink(int argc, const char **argv);
        ~Sink(void) { DSPEW(); };

        bool start(uint32_t numInChannels, uint32_t numOutChannels);
        bool stop(uint32_t numInChannels, uint32_t numOutChannels);
        void input(void *buffer, size_t len, uint32_t inChannelNum);
};


static void usage(void)
{
    char name[64];
    fprintf(crtsOut, "  Usage: %s [ ]\n"
            "\n"
            "\n"
            , CRTS_BASENAME(name, 64));
    throw "module usage";
}


Sink::Sink(int argc, const char **argv)
{
    CRTSModuleOptions opt(argc, argv, usage);
    DSPEW();
}

bool Sink::start(uint32_t numInChannels, uint32_t numOutChannels)
{
    DSPEW();
    return false; // success
}

bool Sink::stop(uint32_t numInChannels, uint32_t numOutChannels)
{
    DSPEW();
    return false; // success
}

void Sink::input(void *buffer, size_t len, uint32_t inChannelNum)
{
}

// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(Sink)
