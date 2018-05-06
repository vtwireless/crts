#include <stdio.h>

#include "crts/crts.hpp"
#include "crts/debug.h"
#include "crts/Filter.hpp"



#define BUFLEN            ((size_t) 600) // Bytes

#define DEFAULT_THRESHOLD ((size_t) 1) // Bytes



static void usage(void)
{
    char nameBuf[64], *name;
    name = CRTS_BASENAME(nameBuf, 64);

    fprintf(stderr,
"\n"
"\n"
"Usage: %s [ --threshold BYTES ]\n"
"\n"
"   This filter copies input channels data to output channels.\n"
"\n"
"   The default threshold is %zu\n"
"\n"
"\n",
    name, DEFAULT_THRESHOLD);

    errno = 0;
    throw "usage help"; // This is how return an error from a C++ constructor
    // the module loader will catch this throw.
}



class Copy : public CRTSFilter
{
    public:

        Copy(int argc, const char **argv);
        ~Copy(void);

        bool start(uint32_t numInChannels, uint32_t numOutChannels);
        bool stop(uint32_t numInChannels, uint32_t numOutChannels);
        void input(void *buffer, size_t bufferLen, uint32_t inChannelNum);

    private:

        size_t threshold;
};



Copy::Copy(int argc, const char **argv)
{
    CRTSModuleOptions opt(argc, argv, usage);

    threshold = opt.get("--threshold", DEFAULT_THRESHOLD);

    DSPEW();
}


bool Copy::start(uint32_t numInChannels, uint32_t numOutChannels)
{
    ASSERT(numInChannels && numOutChannels &&
            numInChannels == numOutChannels,
            "needs equal input and output");

    for(uint32_t i=0; i<numInChannels; ++i)
    {
        setInputThreshold(threshold, i);
        createOutputBuffer(BUFLEN, i);
    }

    DSPEW();

    return false; // success
}


bool Copy::stop(uint32_t numInChannels, uint32_t numOutChannels)
{
    DSPEW();

    return false; // success
}


void Copy::input(void *buffer, size_t len, uint32_t inChannelNum)
{
    // Does this threshold thing work?
    //
    DASSERT(len >= threshold, "");

    memcpy(getOutputBuffer(inChannelNum), buffer, len);

    output(len, inChannelNum/*same as output channel number*/);
}


Copy::~Copy(void)
{
    DSPEW();
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(Copy)
