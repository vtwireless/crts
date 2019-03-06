#include <stdio.h>
#include <errno.h>

#include "crts/crts.hpp"
#include "crts/debug.h"
#include "crts/Filter.hpp"


#define BUFLEN (2*1024)



static void usage(void)
{
    char nameBuf[64], *name;
    name = CRTS_BASENAME(nameBuf, 64);

    fprintf(stderr,
"\n"
"\n"
"Usage: %s\n"
"\n"
"\n"
"\n",
    name);

    errno = 0;
    throw "usage help"; // This is how return an error from a C++ constructor
    // the module loader will catch this throw.
}


class TUN : public CRTSFilter
{
    public:

        TUN(int argc, const char **argv);
        ~TUN(void);

        bool start(uint32_t numInChannels, uint32_t numOutChannels);
        bool stop(uint32_t numInChannels, uint32_t numOutChannels);
        void input(void *buffer, size_t bufferLen, uint32_t inChannelNum);

    private:

        // Full duplex file to the TUN.
        int fd;
};


TUN::TUN(int argc, const char **argv): fd(-1)
{
    CRTSModuleOptions opt(argc, argv, usage);


    DSPEW();
}


bool TUN::start(uint32_t numInChannels, uint32_t numOutChannels)
{
    DSPEW();
    return false; // success
}

bool TUN::stop(uint32_t numInChannels, uint32_t numOutChannels)
{
    DSPEW();
    return false; // success
}


TUN::~TUN(void)
{
    // Do nothing...

    DSPEW();
}


void TUN::input(void *buffer_in, size_t len, uint32_t inputChannelNum)
{
    

}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(TUN)
