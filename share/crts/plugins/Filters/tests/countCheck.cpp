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
        void input(void *buffer, size_t bufferLen, uint32_t inChannelNum);

    private:

        uint64_t expectedCount;

        double sumSq, numSamples, standardDev;

        double getStandardDev(void)
        {
            return standardDev;
        };

        uint32_t numOutputs;

        bool doneCheck;

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
    numOutputs = numOutChannels;
    doneCheck = false;

    setParameter("standardDev", standardDev);

    DSPEW();
    return false; // success
}


void CountCheck::input(void *buffer, size_t len, uint32_t inChannelNum)
{
    DASSERT(len > 0, "");

    ASSERT(doneCheck == false,
        "We got input less than sizeof(uint64_t) already");

    if(len < sizeof(uint64_t))
    {
        // We can't handle this small an amount of data
        // unless this is just cruft on the end and we
        // are done, and this is the last input.
        advanceInput(len);
        doneCheck = true;
        return;
    }

    // We must read full uint64_t integers.
    //
    // We must read full uint64_t integers.
    len -= (len % sizeof(uint64_t));

    // number of uint64_t counts.
    size_t num = len/sizeof(uint64_t);
    uint64_t *count = (uint64_t *) buffer;
    size_t i = 0;

    if(numSamples == 0.0)
    {
        expectedCount = *count + 1;
        ++count;
        numSamples = 1.0;
        ++i;
    }

    for(; i<num; ++i)
    {
        numSamples += 1.0;

        if(*count != expectedCount)
        {
            //DSPEW("count = %" PRIu64 " expectedCount = %" PRIu64, *count, expectedCount);
            sumSq += (*count - expectedCount)*(*count - expectedCount);
            expectedCount = *count;
        }
        // else
        //      We do not need to add to sumSq.
        ++count;
        ++expectedCount;
    }

    // There is a popular singular case where standardDev == 0
    // and there are no errors in the data, so in that case
    // we will not keep setting the parameter standardDev to 0.
    // In all other cases it's likely that standardDev will change
    // at every input() call.
    //
    // TODO: we need a better measure that does not change if
    // the count is good over some given length.

    if(num)
    {
        double oldStandardDev = standardDev;

        if(numSamples > 1)
            standardDev = sqrt(sumSq/(numSamples-1));

        // The "standardDev" parameter is only set here by this filter,
        // and can't be set by a CRTSController.

        if(oldStandardDev != standardDev)
            setParameter("standardDev", standardDev);
    }

    if(numOutputs)
        output(len, ALL_CHANNELS);
    else
        // If len was changed we need to advance the buffer not
        // the full len that came in.
        advanceInput(len);

}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(CountCheck)
