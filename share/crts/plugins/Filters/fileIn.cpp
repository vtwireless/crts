#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <string>

#include "crts/crts.hpp"
#include "crts/debug.h"
#include "crts/Filter.hpp"


class FileIn : public CRTSFilter
{
    public:

        FileIn(int argc, const char **argv);
        ~FileIn(void);

        ssize_t write(void *buffer, size_t bufferLen,
                uint32_t channelNum);
    private:

        FILE *file;
 };


// This is called if the user ran something like: 
//
//    crts_radio -f file [ --help ]
//
//
static void usage(void)
{
    char name[64];
    fprintf(crtsOut, "Usage: %s [ IN_FILENAME ]\n"
            "\n"
            "  The option IN_FILENAME is optional.\n"
            "  The default input is stdin.\n"
            "\n"
            "\n"
            , CRTS_BASENAME(name, 64));

    throw ""; // This is how return an error from a C++ constructor
    // the module loader with catch this throw.
}



FileIn::FileIn(int argc, const char **argv)
{
    const char *filename = 0;
    if(argc > 1 || (argc == 1 && argv[0][0] == '-'))
        usage();
    else if(argc == 1)
        filename = argv[0];

    if(filename)
    {
        errno = 0;
        file = fopen(filename, "r");
        if(!file)
        {
            ERROR("fopen(\"%s\", \"r\") failed", filename);
            throw "failed to open file";
        }
        INFO("opened file: %s", filename);
    }
    else
        file = stdin;

    DSPEW();
}


FileIn::~FileIn(void)
{
    if(file && file != stdin)
        fclose(file);
    file = 0;

    DSPEW();
}


ssize_t FileIn::write(void *buffer, size_t len, uint32_t channelNum)
{
    // This filter is a source so there no data passed to
    // whatever called this write().
    //
    DASSERT(buffer == 0, "");

    if(feof(file)) 
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

    // This filter is a source, it reads file which is not a
    // part of this filter stream.
    size_t ret = fread(buffer, 1, len, file);

    if(ret != len)
        NOTICE("fread(,1,%zu,file) only read %zu bytes", len, ret);

    if(ret > 0)
        // Send this buffer to the next readers write call.
        writePush(buffer, ret, ALL_CHANNELS);

    return 1;
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(FileIn)
