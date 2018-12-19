#include <stdio.h>
#include <math.h>

#include "crts/debug.h"
#include "crts/Filter.hpp"
#include "crts/crts.hpp"



static void usage(void)
{
    char nameBuf[64], *name;
    name = CRTS_BASENAME(nameBuf, 64);

    fprintf(stderr,
"\n"
"\n"
"Usage: %s\n"
"\n"
"   This filter looks at the data flowing through it and measures stuff.\n"
"   This filter read only one channel in.  This filter can be a sink or\n"
"   can have any number of output channels.  If there is output, this filter\n"
"   uses the input buffer as a pass through buffer.\n"
"\n"
"\n",
    name);

    errno = 0;
    throw "usage help"; // This is how return an error from a C++ constructor
    // the module loader will catch this throw.
}


class CountCheck : public CRTSFilter
{
    public:

        CountCheck(int argc, const char **argv);
        ~CountCheck(void);

        bool start(uint32_t numInChannels, uint32_t numOutChannels);
        bool stop(uint32_t numInChannels, uint32_t numOutChannels);
        void input(void *buffer, size_t bufferLen, uint32_t inChannelNum);

    private:

        uint64_t expectedCount;

        double sumSq, numSamples, standardDev;

        double getStandardDev(void)
        {
            return standardDev;
        };

};



CountCheck::CountCheck(int argc, const char **argv)
{
    CRTSModuleOptions opt(argc, argv, usage);

    addParameter("standardDev",
            [&]() { return getStandardDev(); });

    DSPEW();
}


CountCheck::~CountCheck(void)
{
    DSPEW();
}


bool CountCheck::start(uint32_t numInChannels, uint32_t numOutChannels)
{
    ASSERT(numInChannels == 1 && !isSource(),
            "The number of input channels must be one");

    for(uint32_t i=0; i<numOutChannels; ++i)
        createPassThroughBuffer(0, i,
                sizeof(uint64_t)/*maxBufferLen promise to eat*/,
                sizeof(uint64_t)/*threshold length*/);

    expectedCount = (uint64_t) -1;
    standardDev = 0.0;
    sumSq = 0.0;
    numSamples = 0.0;

    setParameter("standardDev", standardDev);

    DSPEW();
    return false; // success
}


bool CountCheck::stop(uint32_t numInChannels, uint32_t numOutChannels)
{
    DSPEW();
    return false; // success
}


void CountCheck::input(void *buffer, size_t len, uint32_t inChannelNum)
{
    DASSERT(len > 0, "");

    // We must read full uint64_t integers.
    //
    if(stream->isRunning)
    {
        DASSERT(len >= sizeof(uint64_t), "");
        // In the case where we are finishing running we will
        // need to output len even if we can't measure it.
        //
        // We must read full uint64_t integers.
        len -= len % sizeof(uint64_t);
    
        // We can't advance (process) the whole length that came in if it
        // was not a multiple of length sizeof(uint64_t).
        //
        // It's okay if "len" did not change, we're doing what would have
        // been done anyway.
        //
        advanceInput(len);
    }


    // number of uint64_t counts.
    size_t num = len/sizeof(uint64_t);
    uint64_t *count = (uint64_t *) buffer;
    size_t i = 0;

    if(numSamples == 0.0)
    {
        expectedCount = *(count++) + 1;
        numSamples = 1.0;
        ++i;
    }

    for(; i<num; ++i)
    {
        numSamples += 1.0;

        if(*count != ++expectedCount)
        {
            sumSq += (*count - expectedCount)*(*count - expectedCount);
            ++count;
        }
        // else
        //      We do no need to add to sumSq.
    }

    // There is a popular singular case where standardDev == 0
    // and there are no errors in the data, so in that case
    // we will not keep setting the parameter standardDev to 0.
    // In all other cases it's likely that standardDev will change
    // at every input() call.

    double oldStandardDev = standardDev;

    standardDev = sqrt(sumSq/(numSamples-1));

    // The "standardDev" parameter is only set here by this filter,
    // and can't be set by a CRTSController.

    if(oldStandardDev != standardDev)
        setParameter("standardDev", standardDev);

    output(len, 0);
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(CountCheck)
