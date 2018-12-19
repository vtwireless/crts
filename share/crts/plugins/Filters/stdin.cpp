#include <stdio.h>
#include <errno.h>

#include "crts/crts.hpp"
#include "crts/debug.h"
#include "crts/Filter.hpp"


#define DEFAULT_BUFFERED  "LINE"
#define BUFLEN (1024)



static void usage(void)
{
    char nameBuf[64], *name;
    name = CRTS_BASENAME(nameBuf, 64);

    fprintf(stderr,
"\n"
"\n"
"Usage: %s [ OPTIONS ]\n"
"\n"
"  All input channels into this filter will be terminated, therefore input\n"
"  will only serve to trigger this filter to read stdin.\n"
"\n"
"  All output channels from this filter will be the full content of stdin,\n"
"  which will be sent every time this filter is triggered.\n"
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

        bool start(uint32_t numInChannels, uint32_t numOutChannels);
        void input(void *buffer, size_t bufferLen, uint32_t inChannelNum);

    private:

        FILE *file;
};


Stdin::Stdin(int argc, const char **argv): file(0)
{
    CRTSModuleOptions opt(argc, argv, usage);

    const char * buffered = opt.get("--buffered", DEFAULT_BUFFERED);

    if(!strncasecmp(buffered, "NO", 1))
    {
        setvbuf(stdin, 0, _IONBF, 0);
        INFO("Set stdin to unbuffered");
    }
    else if(!strncasecmp(buffered, "FULL", 1))
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


bool Stdin::start(uint32_t numInChannels, uint32_t numOutChannels)
{
    if(numOutChannels)
    {
        // We will read stdin.
        file = stdin;

        // We use one buffer for the source of each output Channel
        // that all share the same ring buffer.
        createOutputBuffer(BUFLEN, ALL_CHANNELS);
    }

    if(numInChannels)
        setInputThreshold(0, ALL_CHANNELS);

    // We will merge data from any input channels.
    //
    // If there are no inputs this Filer becomes a source filter which
    // gets write(0,0,0) calls in a loop, which is the start of a buffer
    // flow.

    DSPEW();
    return false; // success
}


Stdin::~Stdin(void)
{
    // Do nothing...

    DSPEW();
}


void Stdin::input(void *buffer_in, size_t len, uint32_t inputChannelNum)
{
    if(!file)
    {
        // There are no output channels so we are done.
        //
        // This is a stupid use case.
        //
        return;
    }

    uint8_t *buffer = (uint8_t *) getOutputBuffer(0);

    if(len)
    {
        // We have input and not just a 0 length trigger.
        // We append this input to our output.
        //
        DASSERT(buffer_in, "");
        memcpy(buffer, buffer_in, len);
        buffer += len;
    }


    if(feof(file))
    {
        // end of file

        if(len)
            // Send the outputs to all down-stream filters.
            output(len, ALL_CHANNELS);
        else
        {
            // We a have 0 trigger input.
            // In this case we are done with stdin.
            stream->isRunning = false;
            file = 0;
        }

        NOTICE("read end of file");
        return; // We are done.
    }

    size_t ret;

    // This filter is a source. It reads stdin which is not a
    // part of this CRTS filter stream.  We append stdin to
    // the output buffer.
    //
    ret = fread(buffer, 1, BUFLEN, file);

    if(ret != BUFLEN)
    {
        NOTICE("fread(,1,%zu,stdin) only read %zu bytes", BUFLEN, len);
        if(feof(stdin))
        {
            // end of file
            //
            // This will be the last call to this write() function for
            // this start-stop interval.  And since this is stdin, we
            // can't start again, so this whole stream may be done.
            stream->isRunning = false;
            NOTICE("read end of file");
        }
    }

    if(len + ret > 0)
        // Send the output to other down-stream filters.
        output(len + ret, ALL_CHANNELS);
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(Stdin)
