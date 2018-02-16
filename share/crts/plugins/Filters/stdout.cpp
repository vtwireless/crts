#include <stdio.h>

#include "crts/debug.h"
#include "crts/Filter.hpp"
#include "crts/crts.hpp" // for:  FILE *crtsOut in place of stdout

#define DBDELETE

class Stdout : public CRTSFilter
{
    public:

        Stdout(int argc, const char **argv);
#ifdef DBDELETE
        ~Stdout(void);
#endif
        ssize_t write(void *buffer, size_t bufferLen, uint32_t channelNum);

    private:

        size_t totalOut, maxOut; // bytes
};


Stdout::Stdout(int argc, const char **argv): totalOut(0), maxOut(-1)
{
    DSPEW();
}


#ifdef DBDELETE
Stdout::~Stdout(void)
{
    DSPEW();
}
#endif


ssize_t Stdout::write(void *buffer, size_t len, uint32_t channelNum)
{
    DASSERT(buffer, "");
    DASSERT(len, "");

    // This filter is a sink, the end of the line, so we do not call
    // writePush().  crtsOut is used like stdout because libuhd screwed up
    // stdout.   It writes crtsOut which is not part of the filter
    // stream.

    errno = 0;

    if(len + totalOut > maxOut)
        // Let's not write more than maxOut.
        len = maxOut - totalOut;

    size_t ret = fwrite(buffer, 1, len, crtsOut);

    if(ret != len && errno == EINTR)
    {
        // One more try because we there interrupted.
        errno = 0;
        ret += fwrite(buffer, 1, len - ret, crtsOut);
    }

    totalOut += ret;

    if(ret != len)
        NOTICE("fwrite(,1,%zu,crtsOut) only wrote %zu bytes", len, ret);

    if(totalOut >= maxOut)
    {
        NOTICE("wrote %zu total, finished writing %zu bytes",
                totalOut, maxOut);
        stream->isRunning = false;
    }

    fflush(crtsOut);

    return ret;
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(Stdout)
