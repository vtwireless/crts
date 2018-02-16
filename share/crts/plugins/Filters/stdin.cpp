#include <stdio.h>

#include "crts/debug.h"
#include "crts/Filter.hpp"

#define DBDELETE


class Stdin : public CRTSFilter
{
    public:

        Stdin(int argc, const char **argv);
#ifdef DBDELETE
        ~Stdin(void);
#endif

        ssize_t write(void *buffer, size_t bufferLen,
                uint32_t channelNum);
};


Stdin::Stdin(int argc, const char **argv)
{
    DSPEW();
#if 0 // TODO: remove this DEBUG SPEW
    DSPEW("  GOT ARGS");
    for(int i=0; i<argc; ++i)
        DSPEW("    ARG[%d]=\"%s\"", i, argv[i]);
    DSPEW();
#endif

    setlinebuf(stdin);
}

#ifdef DBDELETE
Stdin::~Stdin(void)
{
    DSPEW();
}
#endif

ssize_t Stdin::write(void *buffer, size_t len, uint32_t channelNum)
{
    // This filter is a source so there no data passed to
    // whatever called this write().
    //
    // Source filters are different than non-source filters in that they
    // run a loop like this until they are stopped by the isRunning flag
    // or an external end condition like, in this case, end of file.
    //
    DASSERT(buffer == 0, "");

    if(feof(stdin)) 
    {
            // end of file
        stream->isRunning = false;
        NOTICE("read end of file");
        return 0; // We are done.
    }
 
    // Recycle the buffer and len argument variables.
    len = 1024;
    // Get a buffer from the buffer pool.
    buffer = (uint8_t *) getBuffer(len);

    // This filter is a source, it reads stdin which is not a
    // part of this filter stream.
    size_t ret = fread(buffer, 1, len, stdin);

    if(ret != len)
        NOTICE("fread(,1,%zu,stdin) only read %zu bytes", len, ret);

    if(ret > 0)
        // Send this buffer to the next readers write call.
        writePush(buffer, ret, ALL_CHANNELS);

    return 1;
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(Stdin)
