#include <stdio.h>
#include <errno.h>

#include "crts/crts.hpp"
#include "crts/debug.h"
#include "crts/Filter.hpp"


#define CHUNK_LEN (1024/8)

#define BUFLEN (CHUNK_LEN * sizeof(uint64_t))


static void usage(void)
{
    char nameBuf[64], *name;
    name = CRTS_BASENAME(nameBuf, 64);

    fprintf(stderr,
"\n"
"\n"
"Usage: %s\n"
"\n"
"    Write unsigned 64 bit counter to the stream, starting at 1.\n"
"    Each subsequent value is computed from a maximum length sequence\n"
"    defined by reference:\n"
"           https://en.wikipedia.org/wiki/Maximum_length_sequence .\n"
"\n"
"    This filter can only be a source filter.  You can connect it\n"
"    to any number of filters.\n"
"\n"
"\n",
    name);

    errno = 0;
    throw "usage help"; // This is how return an error from a C++ constructor
    // the module loader will catch this throw.
}


class Count : public CRTSFilter
{
    public:

        Count(int argc, const char **argv);

        bool start(uint32_t numInChannels, uint32_t numOutChannels);
        void input(void *buffer, size_t bufferLen, uint32_t inChannelNum);

    private:

        uint64_t count; // next value in the sequence

};


Count::Count(int argc, const char **argv)
{
    CRTSModuleOptions opt(argc, argv, usage); // --help

    DSPEW();
}


bool Count::start(uint32_t numInChannels, uint32_t numOutChannels)
{
    DASSERT(isSource(), "This must be a source filter");

    count = 1;

    createOutputBuffer(BUFLEN, ALL_CHANNELS);

    DSPEW();
    return false; // success
}


uint64_t GetNextValue(uint64_t count)
{
    uint64_t highestBit = ((count & 01) + ((count & 02) >> 1)) << 63;
    return (count >> 1) | highestBit;
}


void Count::input(void *buffer_in, size_t len, uint32_t inputChannelNum)
{
    if(inputChannelNum > 0) 
        return;

    // We just write when input channel zero is triggered.
    uint64_t *buf = (uint64_t *) getOutputBuffer(0);

    // We can only write out multiples of length of uint64_t
    //
    for(len = CHUNK_LEN; len; --len)
    {
        // Write a count to the next 64 bit integer to the buffer.
        *(buf++) = count;
        count = GetNextValue(count);
    }

    // Send the output to all down-stream filters.
    output(BUFLEN, ALL_CHANNELS);
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(Count)
