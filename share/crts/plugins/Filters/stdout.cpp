#include <stdio.h>

#include "crts/debug.h"
#include "crts/Filter.hpp"
#include "crts/crts.hpp" // for:  FILE *crtsOut in place of stdout

#define DEFAULT_BUFFERED  "LINE"


static void usage(void)
{
    char nameBuf[64], *name;
    name = CRTS_BASENAME(nameBuf, 64);

    fprintf(stderr,
"\n"
"\n"
"Usage: %s [ OPTIONS ]\n"
"\n"
"  OPTIONS are optional.\n"
"\n"
"\n"
"  ---------------------------------------------------------------------------\n"
"                           OPTIONS\n"
"  ---------------------------------------------------------------------------\n"
"\n"
"\n"
"   --buffered NO|FULL|LINE   the default is \"" DEFAULT_BUFFERED "\"\n"
"\n"
"\n",
    name);

    errno = 0;
    throw "usage help"; // This is how return an error from a C++ constructor
    // the module loader will catch this throw.
}


class Stdout : public CRTSFilter
{
    public:

        Stdout(int argc, const char **argv);
        ~Stdout(void);
        ssize_t write(void *buffer, size_t bufferLen, uint32_t channelNum);

    private:

        size_t totalOut, maxOut; // bytes
};


Stdout::Stdout(int argc, const char **argv): totalOut(0), maxOut(-1)
{
    CRTSModuleOptions opt(argc, argv, usage);

    const char * buffered = opt.get("--buffered", DEFAULT_BUFFERED);

    if(!strcmp(buffered, "NO"))
    {
        setvbuf(crtsOut, 0, _IONBF, 0);
        INFO("Set crtsOut to unbuffered");
    }
    else if(!strcmp(buffered, "FULL"))
    {
        setvbuf(crtsOut, 0, _IOFBF, 0);
        INFO("Set crtsOut to full buffered");
    }
    else // default is line buffered
    {
        setlinebuf(crtsOut);
        INFO("Set crtsOut to line buffered");
    }

    DSPEW();
}


Stdout::~Stdout(void)
{
    DSPEW();
}


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
