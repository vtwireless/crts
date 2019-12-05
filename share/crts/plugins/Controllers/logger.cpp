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
"Usage: %s -C logger [ --file FILE FILTER PARAMETER ... [ OPTIONS ] ]\n"
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
"   --file FILE FILTER PARAMETER0 [ PARAMETER1 ... ]\n"
"\n"
"                         Write FILE with time in seconds since starting or since\n"
"                         OFFSET_SECONDS, if that option was given, then write the\n"
"                         value of the listed parameters.  There must be at least\n"
"                         one --file option, and there may be any number of --file\n"
"                         options in the command line.\n"
" parameters may be freq, totalBytesIn, totalBytesOut,\n"
" numChannelsIn, numChannelsOut, \n"
"Example : -C logger [ --file test.txt tx freq totalBytesIn]\n"
"\n"
"   --help                print this help\n"
"\n"
"\n  TODO:\n"
"   --period SECONDS      Compute a running average of all entries rate\n"
"                         over this period SECONDS adding a rate after\n"
"                         each parameter.\n"
"\n"
"\n"
"   --time-offset SECONDS Subtract this offset to the times reported.  The default\n"
"                         offset will be the time at the start.  This time is the\n"
"                         standard UNIX time, the number seconds since 1970.\n"
"\n",
    name);
}



class Logger: public CRTSController
{
    public:

        Logger(int argc, const char **argv);
        ~Logger(void);

        void start(CRTSControl *c,
                uint32_t numChannelsIn,
                uint32_t numChannelsOut);
        void stop(CRTSControl *c) { run(c); };

        void execute(CRTSControl *c, const void *buffer, size_t len,
                uint32_t channelNum) { run(c); };
        DSPEW("testing constructor");
    private:

        double GetTime(void)
        {
            struct timespec t;
            ASSERT(clock_gettime(CLOCK_TYPE, &t) == 0, "");
            return (double) t.tv_sec - offset.tv_sec +
                (t.tv_nsec - offset.tv_nsec)/1.0e9;
        };

        void run(CRTSControl *c);

        struct timespec offset;
        double d_offset;
        double period;

        // To keep track of files that we will be writing to:
        std::map<CRTSControl *, FILE *> fileMap;
        std::map<CRTSControl *, std::list<const char *>> parameterMap;
        FILE **files;
};

#define DEFAULT_DOUBLE_OFFSET (-1.0)


Logger::Logger(int argc, const char **argv):
    files(0)
{
    INFO("logger constructor");
    // To parse the --help we call:
    CRTSModuleOptions opt(argc, argv, usage);

    d_offset = opt.get("--offset", DEFAULT_DOUBLE_OFFSET);
    period = opt.get("--period", 0.0);

    int nFiles = 0;
    DSPEW("argc %d", argc);

    for(int i=0; i<argc;)
    {
        if(strcmp(argv[i], "--file") == 0)
        {
            ++i;
            DSPEW("argc %d", argc);
            if(((i+2)>=argc) || (strncmp("--", argv[i+1], 2) == 0))
            {
                ERROR("bad --file option");
                usage();
                throw "bad --file option";
            }

            FILE *file = fopen(argv[i], "w");
            if(!file)
            {
                ERROR("fopen(\"%s\", w) failed", argv[i]);
                usage();
                throw (std::string("fopen(\"") +
                        argv[i] + "\", w) failed");
            }
            DSPEW("opened log file \"%s\"", argv[i]);

            ++nFiles;
            files = (FILE **) realloc(files, sizeof(FILE *)*(nFiles+1));
            files[nFiles-1] = file;
            files[nFiles] = 0;
            const char *controlName = argv[++i];
            CRTSControl *c = getControl<CRTSControl *>(controlName);
            if(!c)
            {
                ERROR("failed to get controller: %s", argv[i]);
                throw (std::string("failed to get controller: ") + argv[i]); 
            }
            fileMap[c] = file;
            fprintf(file, "time_seconds");
            std::list<const char *> parameters;
            while(++i<argc && strncmp("--", argv[i], 2) != 0)
            {
                fprintf(file, " %s:%s", controlName, argv[i]);
                parameters.push_back(argv[i]);
            }
            fprintf(file, "\n");
            if(parameters.size() == 0)
            {
                ERROR("bad --file option no parameters listed");
                usage();
                throw "bad --file option no parameters listed";
            }
            // Copy this list into the map of parameters;
            // we are assuming that it's not a big list.
            parameterMap[c] = parameters;
            continue; // next
        }
        // else skip this argument
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



void Logger::start(CRTSControl *c,
                uint32_t numChannelsIn,
                uint32_t numChannelsOut)
{
    INFO("Logger start");
    if(d_offset == DEFAULT_DOUBLE_OFFSET)
        ASSERT(clock_gettime(CLOCK_TYPE, &offset) == 0, "");
    else
    {
        offset.tv_sec = d_offset;
        offset.tv_nsec = fmod(d_offset, 1.0)*1000000000;
    }
}


void Logger::run(CRTSControl *c)
{
    INFO("logger run");
    //DSPEW("control=\"%s\"", c->getName());

    FILE *file = fileMap[c];

    fprintf(file, "%.22lg", GetTime()); 

    // C++11
    for(auto parameter: parameterMap[c])
        fprintf(file, " %.22lg", c->getParameter(parameter));
    fprintf(file, "\n");
}


// Define the module loader stuff to make one of these class objects.
CRTSCONTROLLER_MAKE_MODULE(Logger)
