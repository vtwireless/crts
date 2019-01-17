// This CRTSController module connects to a TCP/IP server and relays
// commands from that server to the filter stream.
// 
#include <stdio.h>
#include <math.h>
#include <values.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>

#include <string>
#include <list>
#include <map>
#include <atomic>

#include <crts/crts.hpp>
#include <crts/debug.h>
#include <crts/Controller.hpp>
#include <crts/Filter.hpp>


#define CLOCK_TYPE (CLOCK_REALTIME_COARSE)


static void usage(void)
{
    char nameBuf[64], *name;
    name = CRTS_BASENAME(nameBuf, 64);
    fprintf(stderr,
"\n"
"\n"
"Usage: %s --file FILE Filter ... [ OPTIONS ]\n"
"\n"
"  Write log files.  TODO: Add a tcp or TLS connection to the web server that\n"
"  sends this to web clients.  TODO: Easyer may be to publish web viewable plots.\n"
"\n"
"  TODO: This code uses libc buffered stream files.  The callbacks that write\n"
"  are in the filters input controller execute functions, and so when the streams\n"
"  flush, the filters input may be delayed.  Since these are buffered streams there\n"
"  will not be system write calls at every filter input, and that's good, but when\n"
"  the stream flushes there may be an interruption of the stream flow.\n"
"\n"
"\n"
"  ---------------------------------------------------------------------------\n"
"                           OPTIONS\n"
"  ---------------------------------------------------------------------------\n"
"\n"
"\n"
"   --file FILE FILTER PARAMETER0 PARAMETER1 ...\n"
"                                  Write FILE with time in seconds since starting\n"
"                                  or since OFFSET_SECONDS, if that option was given,\n"
"                                  than write the value of the listed parameters.\n"
"                                  There must be at least one --file option.\n"
"\n"
"\n"
"\n  TODO:\n"
"   --period SECONDS               Compute a running average of all entries rate\n"
"                                  over this period SECONDS adding a rate after\n"
"                                  each parameter.\n"
"\n"
"\n"
"   --help                         print this help\n"
"\n",
    name);
}



class Logger: public CRTSController
{
    public:

        Logger(int argc, const char **argv);
        ~Logger(void);

        void stop(CRTSControl *c) { run(c); };

        void execute(CRTSControl *c, const void *buffer, size_t len,
                uint32_t channelNum) { run(c); };

    private:

        void run(CRTSControl *c);

        // To keep track of files that we will be writing to:
        std::map<CRTSControl *, FILE *> fileMap;
        FILE **files;
};


Logger::Logger(int argc, const char **argv):
    files(0)
{
    // To parse the --help we call:
    CRTSModuleOptions opt(argc, argv, usage);

    int nFiles = 0;

    for(int i=0; i<argc;)
    {
        if(strcmp(argv[i], "--file") == 0)
        {
            ++i;

            if(((i+1)>=argc) || (strncmp("--", argv[i+1], 2) == 0))
            {
                ERROR("bad --file option i+1=%d argc=%d argv[i+1]=\"%s\"", i+1, argc, argv[i+1]);
                usage();
                throw "bad --file option";
            }

            FILE *file = fopen(argv[i], "w");
            if(!file)
            {
                ERROR("fopen(\"%s\", w) failed", argv[i]);
                usage();
                throw "bad --file option";
            }
            DSPEW("opened log file \"%s\"", argv[i]);

            ++nFiles;
            files = (FILE **) realloc(files, sizeof(FILE *)*(nFiles+1));
            files[nFiles-1] = file;
            files[nFiles] = 0;

            while(++i<argc && strncmp("--", argv[i], 2) != 0)
            {
                CRTSControl *c = getControl<CRTSControl *>(argv[i]);
                fileMap[c] = file;
            }
        }
        else
            ++i;
    }

    if(nFiles == 0)
    {
        ERROR("No valid --file options given");
        usage();
        throw "No valid --file options given";
    }

    DSPEW();
}


Logger::~Logger(void)
{
    if(files)
    {
        // files is NULL terminated.
        for(FILE **file = files; *file; ++file)
            fclose(*file);
        free(files);
    }

    DSPEW();
};



void Logger::run(CRTSControl *c)
{
    DSPEW("control=\"%s\"", c->getName());

    fprintf(fileMap[c], "%.22lg %.22lg\n",
            c->getParameter("totalBytesIn"),
            c->getParameter("totalBytesOut"));
}


// Define the module loader stuff to make one of these class objects.
CRTSCONTROLLER_MAKE_MODULE(Logger)
