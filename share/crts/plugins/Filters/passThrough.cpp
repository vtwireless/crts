#include <stdio.h>

#include "crts/debug.h"
#include "crts/Filter.hpp"

#define DBDELETE

class PassThrough : public CRTSFilter
{
    public:

        PassThrough(int argc, const char **argv);
#ifdef DBDELETE
        ~PassThrough(void);
#endif
        ssize_t write(void *buffer, size_t bufferLen,
                uint32_t channelNum);
};

// TODO: Connect channels:  inChannel N to outChannel M
// For now connect N to N
PassThrough::PassThrough(int argc, const char **argv) {DSPEW();}

ssize_t PassThrough::write(void *buffer, size_t len, uint32_t channelNum)
{
    DASSERT(buffer, "");
    DASSERT(len, "");

    // Send this buffer to the next readers write call
    // on all channels.
    writePush(buffer, len, CRTSFilter::ALL_CHANNELS);

    return len;
}


#ifdef DBDELETE
PassThrough::~PassThrough(void)
{
    DSPEW();
}
#endif
 

// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(PassThrough)
