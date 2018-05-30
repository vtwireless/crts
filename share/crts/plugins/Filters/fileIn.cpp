#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "crts/crts.hpp"
#include "crts/debug.h"
#include "crts/Filter.hpp"

#define BUFLEN (1024)

#define DEFAULT_FILENAME  "-"



static void usage(void)
{
    char nameBuf[64], *name;
    name = CRTS_BASENAME(nameBuf, 64);

    fprintf(stderr,
"\n"
"\n"
"Usage: %s [ OPTIONS ]\n"
"\n"
"\n    Read a file (raw) without a stream buffer.\n"
"\n"
"  ---------------------------------------------------------------------------\n"
"                           OPTIONS\n"
"  ---------------------------------------------------------------------------\n"
"\n"
"\n"
"    --file FILE  open and read file FILE.  The default file is " DEFAULT_FILENAME
"\n"
"                 which is STDIN_FILENO the standard input.\n"
"\n"
"\n",
    name);

    errno = 0;
    throw "usage help"; // This is how return an error from a C++ constructor
    // the module loader will catch this throw.
}


class FileIn : public CRTSFilter
{
    public:

        FileIn(int argc, const char **argv);
        ~FileIn(void);

        bool start(uint32_t numInChannels, uint32_t numOutChannels);
        bool stop(uint32_t numInChannels, uint32_t numOutChannels);
        void input(void *buffer, size_t bufferLen, uint32_t inChannelNum);

    private:

        const char *filename;

        int fd;
};


FileIn::FileIn(int argc, const char **argv): fd(-1)
{
    CRTSModuleOptions opt(argc, argv, usage);

    filename = opt.get("--file", "stdin");

    DSPEW();
}


bool FileIn::start(uint32_t numInChannels, uint32_t numOutChannels)
{
    if(!isSource())
    {
        WARN("This must be a source");
        return true; // fail.
    }

    if(!numOutChannels)
    {
        WARN("There must be an output channel");
        return true; // fail
    }

    if(strcmp(filename, "stdin") && strcmp(filename, "-"))
        fd = open(filename, O_RDONLY);
    else
        fd = STDIN_FILENO;

    if(fd < 0)
    {
        WARN("open(\"%s\", O_RDONLY) failed", filename);
        return true; // fail
    }
 
    // We use one buffer for the source of each output Channel
    // that all share the same ring buffer.
    createOutputBuffer(BUFLEN, ALL_CHANNELS);

    DSPEW();
    return false; // success
}


bool FileIn::stop(uint32_t numInChannels, uint32_t numOutChannels)
{
    // In this function we could flush away all the input data to stdin,
    // but if the input never stops that could be a problem.  We could
    // close a file here, but that's not a good idea either.  So stop() in
    // this filter does nothing, any input to stdin will just accumulate.
    //
    // We have no reactor core to shutdown in this, stdin, case.
    //
    // Buffers all get destroyed automatically and if a start() happens
    // again buffers get re-created.

    if(fd > -1 && fd != STDIN_FILENO)
        close(fd);
    fd = -1;

    DSPEW();
    return false; // success
}


FileIn::~FileIn(void)
{
    // Do nothing...

    DSPEW();
}


void FileIn::input(void *buffer_in, size_t len, uint32_t inputChannelNum)
{
    uint8_t *buffer = (uint8_t *) getOutputBuffer(0);

    ssize_t ret = read(fd, buffer, BUFLEN);

    if(ret < 1)
    {
        NOTICE("read(,,%zu) returned %zd", BUFLEN, ret);
        // end of file
        stream->isRunning = false;
        return;
    }

    // Send the output to other down-stream filters.
    output(ret, ALL_CHANNELS);
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(FileIn)
