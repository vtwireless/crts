#include <stdio.h>

#include "crts/debug.h"
#include "crts/crts.hpp"
#include "crts/Filter.hpp"


class Sink : public CRTSFilter
{
    public:

        Sink(int argc, const char **argv);
        ~Sink(void) { DSPEW(); };

        ssize_t write(void *buffer, size_t bufferLen, uint32_t channelNum)
        {
            return 1;
        };
};


static void usage(void)
{
    char name[64];
    fprintf(crtsOut, "  Usage: %s [ ]\n"
            "\n"
            "\n"
            , CRTS_BASENAME(name, 64));

    throw "module usage";
}




Sink::Sink(int argc, const char **argv)
{
    CRTSModuleOptions opt(argc, argv, usage);
    opt.get("--any_optionZZ", ""); // to get usage from --help

    DSPEW();
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(Sink)
