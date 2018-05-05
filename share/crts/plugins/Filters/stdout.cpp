#include <stdio.h>

#include "crts/debug.h"
#include "crts/Filter.hpp"
#include "crts/crts.hpp" // for:  FILE *crtsOut in place of stdout


#define DEFAULT_BUFFERED  "LINE"

#define OUTPUT_BUFFER_LEN ((size_t) 1025)

#define DEFAULT_THRESHOLD ((size_t) 1)


static void usage(void)
{
    char nameBuf[64], *name;
    name = CRTS_BASENAME(nameBuf, 64);

    fprintf(stderr,
"\n"
"\n"
"Usage: %s [ OPTIONS ]\n"
"\n"
"  OPTIONS are optional.  This CRTS filter does not write data to the CRTS stream\n"
"  but it can trigger downward flows with zero bytes of input each time it is\n"
"  triggered.\n"
"\n"
"\n"
"  ---------------------------------------------------------------------------\n"
"                           OPTIONS\n"
"  ---------------------------------------------------------------------------\n"
"\n"
"\n"
"   --buffered NO|FULL|LINE   the default is \"" DEFAULT_BUFFERED "\"\n"
"\n"
"   --threadhold LEN          set the threshold into that triggers write to this\n"
"                             filter to LEN bytes.  The default is %zu byte(s).\n"
"\n"
"\n",
    name, DEFAULT_THRESHOLD);

    errno = 0;
    throw "usage help"; // This is how return an error from a C++ constructor
    // the module loader will catch this throw.
}


class Stdout : public CRTSFilter
{
    public:

        Stdout(int argc, const char **argv);
        ~Stdout(void);

        bool start(uint32_t numInChannels, uint32_t numOutChannels);
        bool stop(uint32_t numInChannels, uint32_t numOutChannels);
        void write(void *buffer, size_t bufferLen, uint32_t inChannelNum);

    private:

        size_t threshold; // bytes input

        uint32_t numOutChannels;
};


#define BUFLEN (1024)


Stdout::Stdout(int argc, const char **argv)
{
    CRTSModuleOptions opt(argc, argv, usage);

    const char * buffered = opt.get("--buffered", DEFAULT_BUFFERED);
    threshold = opt.get("--threshold", DEFAULT_THRESHOLD);

    if(!strncasecmp(buffered, "NO", 1))
    {
        setvbuf(crtsOut, 0, _IONBF, 0);
        INFO("Set crtsOut to unbuffered");
    }
    else if(!strncasecmp(buffered, "FULL", 1))
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


bool Stdout::start(uint32_t numInChannels, uint32_t numOutChannels_in)
{
    // For all input channels.
    setInputThreshold(threshold);

    numOutChannels = numOutChannels_in;

    if(numOutChannels)
    {
        uint32_t outChannels[numOutChannels+1];
        uint32_t i = 0;
        for(; i < numOutChannels; ++i)
            outChannels[i] = i;
        outChannels[i] = NULL_CHANNEL;

        // In the case that we have output channels
        // we create a shared output buffer for all of them.
        createOutputBuffer(OUTPUT_BUFFER_LEN, outChannels);
    }


    DSPEW();
    return false; // success
}


bool Stdout::stop(uint32_t numInChannels, uint32_t numOutChannels)
{
    DSPEW();
    return false; // success
}


void Stdout::write(void *buffer, size_t len, uint32_t inChannelNum)
{
    //WARN("buffer=%p buffer=\"%s\" len=%zu", buffer, (char *) buffer, len);
    // crtsOut is used like stdout because libuhd screwed up stdout.   It
    // writes crtsOut which is not part of the CRTS filter stream.

    errno = 0;

    size_t ret = fwrite(buffer, 1, len, crtsOut);

    // Mark this much input as consumed.
    advanceInputBuffer(len);


    if(ret != len && errno == EINTR)
    {
        // One more try because we there interrupted.
        errno = 0;
        ret += fwrite(buffer, 1, len - ret, crtsOut);
    }

    if(ret != len)
    {
        if(errno == EPIPE)
            // This is very common.
            NOTICE("got broken pipe");
        else
            // The output may be screwed at this point, given we could
            // not write what came in.
            WARN("fwrite(,1,%zu,crtsOut) only wrote %zu bytes", len, ret);
    }

    if(ret)
    {
        if(numOutChannels)
        {
            buffer = getOutputBuffer(0);
            writePush(ret, ALL_CHANNELS);
        }

        fflush(crtsOut);
    }
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(Stdout)
