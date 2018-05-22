#include <stdio.h>
#include <errno.h>

#include "crts/crts.hpp"
#include "crts/debug.h"
#include "crts/Filter.hpp"


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
    CRTSModuleOptions opt(argc, argv);
    DSPEW();
}


#define BUFLEN (1024)


bool Stdin::start(uint32_t numInChannels, uint32_t numOutChannels)
{
    if(!isSource())
    {
        WARN("This must be a source filter");
        return true; // fail
    }

    // We use one ring buffer for the source of each output Channel that
    // all share the same ring buffer.
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


void Stdin::input(void *buffer_in, size_t len, uint32_t inputChannelNum)
{
    uint8_t *buffer = (uint8_t *) getOutputBuffer(0);


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
