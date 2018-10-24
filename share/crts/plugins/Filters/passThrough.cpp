#include <stdio.h>
#include <unistd.h>

#include "crts/crts.hpp"
#include "crts/debug.h"
#include "crts/Filter.hpp"


#define DEFAULT_THRESHOLD  ((size_t) 1)    // Bytes

// TODO: make this an option:
#define DEFAULT_BUFLEN     ((size_t) 1) // Bytes




static void usage(void)
{
    char nameBuf[64], *name;
    name = CRTS_BASENAME(nameBuf, 64);

    fprintf(stderr,
"\n"
"\n"
"Usage: %s [ OPTIONS ]\n"
"\n"
"  Pass through.  Ya, all input channels will be passed though to the corresponding\n"
"  output channel, and the channel buffers will be reused.  By default unpaired\n"
"  channels will be terminated on either end.\n"
"\n"
"  ---------------------------------------------------------------------------\n"
"                           OPTIONS\n"
"  ---------------------------------------------------------------------------\n"
"\n"
"\n"
"   --map CHANNEL_LIST   { --- TODO --- } for example:\n"
"\n"
"                                   --map 0 1 1 0\n"
"\n"
"                        will map input channel 0 to output channel 1, and input\n"
"                        channel 1 to output channel 0, and all other input and\n"
"                        output channels will be terminated.  This filter will not\n"
"                        check and fail for missing channel connections.\n"
"\n"
"   --threshold LEN      set the input threshold, for all channels, to activate this\n"
"                        filter, to LEN bytes.  The default threshold is %zu byte(s).\n"
"                        By setting a threshold one can force this filter to accumulate\n"
"                        data before flushing all the data to adjacent filters.\n"
"                        Keep in mind, that the adjacent filters may have a different\n"
"                        thresholds.\n"
"\n"
"   --usleep USECS       call usleep(USECS) at every input() call.  This is just a very\n"
"                        crude and simple way to throttle the rate at which data is\n"
"                        flowing.  See 'man usleep'.  Use the \"throttle\" filter for\n"
"                        better control.\n"
"\n"
"\n",
    name, DEFAULT_THRESHOLD);

    errno = 0;
    throw "usage help"; // This is how return an error from a C++ constructor
    // the module loader will catch this throw.
}



class PassThrough : public CRTSFilter
{
    public:

        PassThrough(int argc, const char **argv);
        ~PassThrough(void);

        bool start(uint32_t numInChannels, uint32_t numOutChannels);
        bool stop(uint32_t numInChannels, uint32_t numOutChannels);
        void input(void *buffer, size_t bufferLen, uint32_t inChannelNum);

    private:

        size_t threshold, maxBufferLen;
        uint32_t *outputChannel; // keyed by input channel number.
        uint32_t *nullOutputChannels;
        uint32_t startingSinkInputChannel;
        useconds_t usec;

};



PassThrough::PassThrough(int argc, const char **argv):
    outputChannel(0), nullOutputChannels(0),
    startingSinkInputChannel(NULL_CHANNEL)
{
    CRTSModuleOptions opt(argc, argv, usage);

    threshold = opt.get("--threshold", DEFAULT_THRESHOLD);
    usec = opt.get("--usleep", (long) 0);

    DSPEW();
}


bool PassThrough::start(uint32_t numInChannels, uint32_t numOutChannels)
{
    // Setup the filter parameters for this the particular channel
    // connection topology.

    // We call it "pass through"; we just pass through data without
    // copying it or anything.  So we do not create buffers for this
    // filter, we will pass through the input buffers that are the
    // same as the output buffers, at least for all channels that
    // have an input and a corresponding output.

    uint32_t i,
        // The number of channels going through is the lesser of the input
        // and output.   Any unpaired channels become terminated.  Any
        // unpaired input channels become a sink.  An unpaired output
        // becomes a null (zero byte) source that gets triggered by all
        // writes.
        maxPairs =
            (numInChannels < numOutChannels)?numInChannels:numOutChannels;

    if(maxPairs)
        outputChannel = new uint32_t[maxPairs];

    for(i=0; i<maxPairs; ++i)
    {
        // TODO: add the --map option here.  For now map i to i.
        //
        outputChannel[i] = i; // default map for in to out.
        createPassThroughBuffer(i, outputChannel[i],
                /* We promise to consume the data if it's at the
                 * threshold amount */
                threshold /*maxBufferLen promise*/, threshold);
    }

    if(numOutChannels > numInChannels)
    {
        uint32_t j = 0;
        // Do something with the extra output channels ...

        nullOutputChannels = new uint32_t[numOutChannels - numInChannels + 1];

        for(; i<numOutChannels; ++i)
            nullOutputChannels[j++] = i;
        // Null Terminate this nullOutputChannels[] array.
        nullOutputChannels[j] = NULL_CHANNEL;
    }
    else if(numInChannels > numOutChannels)
    {
        // we'll sink the extra input channels.
        startingSinkInputChannel = i;
    }

    DSPEW();

    return false; // success
}

bool PassThrough::stop(uint32_t numInChannels, uint32_t numOutChannels)
{
    // Reset the filter parameters.

    if(outputChannel)
    {
        delete outputChannel;
        outputChannel = 0;
    }

    if(nullOutputChannels)
    {
        delete nullOutputChannels;
        nullOutputChannels = 0;
    }
    startingSinkInputChannel =  NULL_CHANNEL;

    DSPEW();

    return false; // success
}


void PassThrough::input(void *buffer, size_t len, uint32_t inChannelNum)
{
    if(usec)
        usleep(usec);

    if(nullOutputChannels)
    {
        // We have unpaired output channels that become source triggers.
        //
        // We trigger the extra output channels with Null (0 length)
        // input.  We do this at every write() for all input channels.
        // The "source like" filters we are writing to can decide what
        // to do.
        //
        uint32_t *nullOutput = nullOutputChannels;
        while(*nullOutput != NULL_CHANNEL)
            output(0, *nullOutput++);
    }

    if(inChannelNum >= startingSinkInputChannel)
    {
        // Sink this input channel.  We pretend to read it by advancing to
        // the next chunk of data on this conveyor belt like buffer.
        //
        return;
    }

    // Push input to output; in this case without modification, but we
    // could do something like multiply by a constant.  Then again we did
    // not type this data, so how could we change it.  Bits is bits, just
    // push them along len bytes worth.
    //
    output(len, outputChannel[inChannelNum]);
}


PassThrough::~PassThrough(void)
{
    // stop() should have been called if start was ever called,
    // so we have nothing to do here in this destructor.
    //
    DSPEW();
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(PassThrough)
