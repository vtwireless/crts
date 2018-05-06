#include <stdio.h>
#include <errno.h>

#include "crts/crts.hpp"
#include "crts/debug.h"
#include "crts/Filter.hpp"


#define BUFLEN (1024)



static void usage(void)
{
    char nameBuf[64], *name;
    name = CRTS_BASENAME(nameBuf, 64);

    fprintf(stderr,
"\n"
"Usage: %s\n"
"\n"
"  This is a simple example CRTS Filter module that reads standard input and\n"
"  writes it to all connected filters.\n"
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
        bool stop(uint32_t numInChannels, uint32_t numOutChannels);
        void input(void *buffer, size_t bufferLen, uint32_t inChannelNum);
};


Stdin::Stdin(int argc, const char **argv)
{
    CRTSModuleOptions opt(argc, argv, usage);

    // This is so we have a working --help|-h option.
    opt.get("--foo", "");

    DSPEW();
}


bool Stdin::start(uint32_t numInChannels, uint32_t numOutChannels)
{
    if(numOutChannels == 0)
    {
        WARN("There is output to write to.");
        return true; // fail
    }

    if(!isSource())
    {
        WARN("This needs to be a source filter.");
        return true; // fail
    }

    // All the output channels will use the same ring buffer.
    //
    createOutputBuffer(BUFLEN, ALL_CHANNELS);

    DSPEW();
    return false; // success
}


bool Stdin::stop(uint32_t numInChannels, uint32_t numOutChannels)
{
    DSPEW();
    return false; // success
}


Stdin::~Stdin(void)
{
    // Do nothing...

    DSPEW();
}


void Stdin::input(void *buffer, size_t len, uint32_t inputChannelNum)
{
    ASSERT(len == 0, "We got input to our stdin source");

    buffer = getOutputBuffer(0);

    if(feof(stdin))
    {
        // end of file
        stream->isRunning = false;
        NOTICE("read end of file");
        return; // We are done.
    }

    // This filter is a source. It reads stdin which is not a
    // part of this CRTS filter stream. 
    //
    len = fread(buffer, 1, BUFLEN, stdin);

    if(len != BUFLEN)
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

    if(len)
        // Send the output to other down-stream filters.
        output(len, ALL_CHANNELS);
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(Stdin)
