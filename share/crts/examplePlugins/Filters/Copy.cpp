#include <stdio.h>

#include "crts/crts.hpp"
#include "crts/debug.h"
#include "crts/Filter.hpp"


#define BUF_LEN            ((size_t) 1024)  // Bytes
#define DEFAULT_THRESHOLD  ((size_t) 1)     // Bytes



static void usage(void)
{
    char nameBuf[64], *name;
    name = CRTS_BASENAME(nameBuf, 64);

    fprintf(stderr,
"\n"
"\n"
"Usage: %s [ OPTIONS ]\n"
"\n"
"   This filter copies input channels data to output channels.\n"
"\n"
"  ---------------------------------------------------------------------------\n"
"                           OPTIONS\n"
"  ---------------------------------------------------------------------------\n"
"\n"
"   --threshold LEN      set the input threshold, for all channels, to activate this\n"
"                        filter, to LEN bytes.  The default threshold is %zu byte(s).\n"
"                        By setting a threshold one can force this filter to accumulate\n"
"                        data before flushing all the data to adjacent filters.\n"
"                        Keep in mind, that the adjacent filters may have different\n"
"                        thresholds.\n"
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

        size_t maxBufferLen, threshold;

};



Copy::Copy(int argc, const char **argv)
{
    CRTSModuleOptions opt(argc, argv, usage);

    threshold = opt.get("--threshold", DEFAULT_THRESHOLD);

    if(threshold <= BUF_LEN)
        maxBufferLen = BUF_LEN;
    else
        maxBufferLen = threshold;

    DSPEW();
}


bool Copy::start(uint32_t numInChannels, uint32_t numOutChannels)
{
    if(isSource())
    {
        WARN("This filter cannot be a source");
        return true; // fail
    }

    if(numInChannels != numOutChannels)
    {
        WARN("The number of inputs (%" PRIu32 ") is not the same"
                " as the number of outputs (%" PRIu32 ")",
                numInChannels, numOutChannels);
        return true; // fail
    }

    for(uint32_t i=0; i<numInChannels; ++i)
    {
        createOutputBuffer(maxBufferLen, i);
        setInputThreshold(threshold, i);
    }

    DSPEW();

    return false; // success
}


bool Copy::stop(uint32_t numInChannels, uint32_t numOutChannels)
{
    // Reset the filter parameters, but alas there are none
    // that need resetting.

    DSPEW();

    return false; // success
}


void Copy::input(void *buffer, size_t len, uint32_t inChannelNum)
{
    memcpy(getOutputBuffer(inChannelNum), buffer, len);

    // Push input to output; in this case without modification, but we
    // could do something like multiply by a constant.  Then again we did
    // not type this data, so how could we change it.
    //
    // This writes to the output buffer.  It knows that it must copy from
    // the current input buffer to outputChannel[i], because this is the
    // source of outputChannel[inChannelNum] via createOutputBuffer() in
    // start().
    //
    output(len, inChannelNum);
}


Copy::~Copy(void)
{
    // stop() should have been called if start was ever called,
    // so we have nothing to do here.
    //
    DSPEW();
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(Copy)
