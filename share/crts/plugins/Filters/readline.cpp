#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "crts/debug.h"
#include "crts/Filter.hpp"
#include "crts/crts.hpp" // for:  FILE *crtsOut


#define BUFLEN 1024


class Readline : public CRTSFilter
{
    public:

        Readline(int argc, const char **argv);
        ~Readline(void);

        ssize_t write(void *buffer, size_t len, uint32_t channelNum);

    private:

        void run(void);

        const char *prompt;
        const char *sendPrefix;
        size_t sendPrefixLen;
        char *line; // last line read buffer
};


static void usage(const char *arg = 0)
{
    char name[64];

    if(arg)
        fprintf(stderr, "module: %s: unknown option arg=\"%s\"\n",
                CRTS_BASENAME(name, 64), arg);

    fprintf(stderr,
"\n"
"\n"
"Usage: %s [ OPTIONS ]\n"
"\n"
"  OPTIONS are optional.\n"
"\n"
"  As an example you can run something like this:\n"
"\n"
"       crts_radio -f rx [ --uhd addr=192.168.10.3 --freq 932 ] -f stdout\n"
"\n"
"\n"
"  ---------------------------------------------------------------------------\n"
"                           OPTIONS\n"
"  ---------------------------------------------------------------------------\n"
"\n"
"\n"
"    --prompt PROMPT   \n"
"\n"
"\n"
"\n  --send-prefix PREFIX   \n"
"\n"
"\n"
"\n"
"\n",
    CRTS_BASENAME(name, 64));

    errno = 0;
    throw "usage help"; // This is how return an error from a C++ constructor
    // the module loader will catch this throw.
}


Readline::Readline(int argc, const char **argv):
    prompt("> "),
    sendPrefix("received: "),
    sendPrefixLen(strlen(sendPrefix)),
    line(0)
{
    int i = 0;

    while(i<argc)
    {
        // TODO: Keep argument option parsing simple??
        //
        // argv[0] is the first argument
        //
        if(!strcmp("--prompt", argv[i]) && i+1 < argc)
        {
            prompt = argv[++i];
            ++i;
            continue;
        }
        if(!strcmp("--send-prefix", argv[i]) && i+1 < argc)
        {
            sendPrefix = argv[++i];

            sendPrefixLen = strlen(sendPrefix);
            if(sendPrefixLen > BUFLEN/2)
            {
                fprintf(stderr,"argument: \"%s\" is to long\n\n",
                        argv[i-1]);
                usage();
            }
            ++i;
            continue;
        }

        usage(argv[i]);
    }

    // Because libuhd pollutes stdout we must use a different readline
    // prompt stream:
    rl_outstream = crtsOut;

    DSPEW();
}


Readline::~Readline(void)
{
    if(line)
    {
        free(line);
        line = 0;
    }

    // TODO: cleanup readline?
}



#define IS_WHITE(x)  ((x) < '!' || (x) > '~')
//#define IS_WHITE(x)  ((x) == '\n')


void Readline::run(void)
{
    // get a line

    line = readline(prompt);
    if(!line)
    {
        stream->isRunning = false;
        return;
    }
#if 0
    fprintf(stderr, "%s:%d: GOT: \"%s\"  prompt=\"%s\"\n",
            __BASE_FILE__, __LINE__, line, prompt);
#endif
    // Strip off trailing white chars:
    size_t len = strlen(line);
    while(len && IS_WHITE(line[len -1]))
        line[--len] = '\0';

    //fprintf(stderr, "\n\nline=\"%s\"\n\n", line);

    if(len < 1)
    {
        free(line);
        line = 0;
        return; // continue to next loop readline()
    }

    // TODO: add tab help, history saving, and other readline user
    // interface stuff.

    if(!strcmp(line, "exit") || !strcmp(line, "quit"))
    {
        stream->isRunning = false;
        return;
    }

    ssize_t bufLen = len + sendPrefixLen + 2;
    char *buffer = (char *) getBuffer(bufLen);

    memcpy(buffer, sendPrefix, sendPrefixLen);
    memcpy(&buffer[sendPrefixLen], line, len);
    buffer[bufLen-2] = '\n';
    buffer[bufLen-1] = '\0';


    // We do not add the sendPrefix to the history.  Note: buffer was
    // '\0' terminated just above, so the history string is cool.
    add_history(line);

    free(line); // reset readline().
    line = 0;

    writePush((void *) buffer, bufLen, CRTSFilter::ALL_CHANNELS);
}



// This call will block until we get input.
//
// We write to crtsOut
//
ssize_t Readline::write(void *buffer_in, size_t bufferLen_in, uint32_t channelNum)
{
    // This module is a source.
    DASSERT(!buffer_in, "");

    // Source filters loop like this, regular filters do not.
    while(stream->isRunning)
        run();

    return 0; // done
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(Readline)
