#include <stdio.h>
#include <errno.h>

#include "crts/crts.hpp"
#include "crts/debug.h"
#include "crts/Filter.hpp"


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


class Stdin : public CRTSFilter
{
    public:

        Stdin(int argc, const char **argv);
        ~Stdin(void);

        ssize_t write(void *buffer, size_t bufferLen,
                uint32_t channelNum);
};


Stdin::Stdin(int argc, const char **argv)
{
    CRTSModuleOptions opt(argc, argv, usage);

    const char * buffered = opt.get("--buffered", DEFAULT_BUFFERED);

    if(!strcmp(buffered, "NO"))
    {
        setvbuf(stdin, 0, _IONBF, 0);
        INFO("Set stdin to unbuffered");
    }
    else if(!strcmp(buffered, "FULL"))
    {
        setvbuf(stdin, 0, _IOFBF, 0);
        INFO("Set stdin to full buffered");
    }
    else // default is line buffered
    {
        setlinebuf(stdin);
        INFO("Set stdin to line buffered");
    }

    DSPEW();
}


Stdin::~Stdin(void)
{
    DSPEW();
}


ssize_t Stdin::write(void *buffer, size_t len, uint32_t channelNum)
{
    // This filter is a source so there no data passed to
    // whatever called this write().
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
