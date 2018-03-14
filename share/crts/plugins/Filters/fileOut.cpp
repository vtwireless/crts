#include <stdio.h>
#include <string.h>
#include <string>

#include "crts/debug.h"
#include "crts/Filter.hpp"
#include "crts/crts.hpp" // for:  FILE *crtsOut


class FileOut : public CRTSFilter
{
    public:

        FileOut(int argc, const char **argv);
        ~FileOut(void);

        ssize_t write(void *buffer, size_t bufferLen, uint32_t channelNum);

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
    fprintf(crtsOut, "Usage: %s [ OUT_FILENAME ]\n"
            "\n"
            "  The option OUT_FILENAME is optional.  The default\n"
            "  output file is something like stdout.\n"
            "\n"
            "\n"
            , CRTS_BASENAME(name, 64));

    throw ""; // This is how return an error from a C++ constructor
    // the module loader with catch this throw.
}


FileOut::FileOut(int argc, const char **argv): file(0)
{
    const char *filename = 0;
    DSPEW();

    if(argc > 1 || (argc == 1 && argv[0][0] == '-'))
        usage();
    else if(argc == 1)
        filename = argv[0];

    if(filename)
    {
        file = fopen(filename, "a");
        if(!file)
        {
            std::string str("fopen(\"");
            str += filename;
            str += "\", \"a\") failed";
            // This is how return an error from a C++ constructor
            // the module loader with catch this throw.
            throw str;
        }
        INFO("opened file: %s", filename);
    }
    else
        file = crtsOut; // like stdout but not polluted by libuhd

    DSPEW();
}


FileOut::~FileOut(void)
{
    if(file && file != crtsOut)
        fclose(file);
    file = 0;

    DSPEW();
}


ssize_t FileOut::write(void *buffer, size_t len, uint32_t channelNum)
{
    DASSERT(buffer, "");
    DASSERT(len, "");

    // This filter is a sink, the end of the line, so we do not call
    // writePush().  We write no matter what channel it is.
    errno = 0;

    size_t ret = fwrite(buffer, 1, len, file);

    if(ret != len && errno == EINTR)
    {
        // One more try, because we where interrupted
        errno = 0;
        ret += fwrite(buffer, 1, len - ret, file);
    }

    if(ret != len)
        NOTICE("fwrite(,1,%zu,) only wrote %zu bytes", len, ret);

    fflush(file);

    return ret;
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(FileOut)
