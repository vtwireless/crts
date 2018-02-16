#include <stdio.h>

#include "crts/debug.h"
#include "crts/Filter.hpp"


class SourceFeed : public CRTSFilter
{
    public:

        SourceFeed(int argc, const char **argv);

        ssize_t write(void *buffer, size_t bufferLen,
                uint32_t channelNum);
};


SourceFeed::SourceFeed(int argc, const char **argv)
{
    DSPEW();
}

ssize_t SourceFeed::write(void *buffer, size_t len, uint32_t channelNum)
{
    // This filter is a source so there no data passed to
    // whatever called this write().
    //
    DASSERT(!buffer, "");

    writePush(buffer, len, ALL_CHANNELS);
    return 1;
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(SourceFeed)
