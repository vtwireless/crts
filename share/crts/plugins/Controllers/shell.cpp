// This CRTSController module launches a separate crts_shell process that
// runs in an interactive terminal window, and reads commands from stdin
// in the terminal and than writes a "command FIFO" that this module reads
// in a separate thread in shellReceiver() than relays the commands to
// the CRTSController::execute() function in the filter thread.
// Replies from he CRTSController::execute() function are written to the
// "reply FIFO", which the crts_shell process reads and echos to the
// terminal window.
//
// TODO: Just use a full duplex UNIX domain socket in place of the two
// single duplex FIFOs.  But than again, the way we use FIFOs is more
// similar to how we would use regular files where one is the commands and
// the other is the replies, so maybe we could share them with many other
// processes, unlike a UNIX domain socket which only works with two
// processes reading and writing.


#include <stdio.h>
#include <math.h>
#include <values.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>

#include <string>
#include <list>
#include <map>

#include <crts/crts.hpp>
#include <crts/debug.h>
#include <crts/Controller.hpp>
#include <crts/Filter.hpp>


#define MUTEX_LOCK(shell)                                               \
    ASSERT((errno = pthread_mutex_lock(&shell->mutex)) == 0,            \
            "pthread_mutex_lock() failed")

#define MUTEX_UNLOCK(shell)                                             \
    ASSERT((errno = pthread_mutex_unlock(&shell->mutex)) == 0,          \
            "pthread_mutex_unlock() failed")



class Shell: public CRTSController
{
    public:

        Shell(int argc, const char **argv);
        ~Shell(void);

        void start(CRTSControl *c);
        void stop(CRTSControl *c);

        void execute(CRTSControl *c, const void *buffer, size_t len,
                uint32_t channelNum);

        CRTSControl *findControl(std::string name)
        {
            return getControl<CRTSControl *>(name, /*addController*/false);
        };

        pthread_mutex_t mutex;

        // We can't verify commands until we are in the execute() call.
        // So we just store the commands as strings as they come in.
        // Each CRTSControl has a list of commands.
        std::map<CRTSControl *, std::list<std::string>> commands;

        FILE *fifoReply;

        std::string fifoDirPath;
};

/* Find the path to the program "crts_shell" by assuming that it is in the
 * same directory as the current running program "crts_radio".
 *
 * Note: This code is very Linux specific, using /proc/.
 */
static inline std::string getShellPath(void)
{
    ssize_t bufLen = 128;
    char *buf = (char *) malloc(bufLen);
    ASSERT(buf, "malloc() failed");
    ssize_t rl = readlink("/proc/self/exe", buf, bufLen);
    ASSERT(rl > 0, "readlink(\"/proc/self/exe\",,) failed");
    while(rl >= bufLen)
    {
        DASSERT(bufLen < 1024*1024, ""); // it should not get this large.
        buf = (char *) realloc(buf, bufLen += 128);
        ASSERT(buf, "realloc() failed");
        rl = readlink("/proc/self/exe", buf, bufLen);
        ASSERT(rl > 0, "readlink(\"/proc/self/exe\",,) failed");
    }

    // Strip off the filename leaving just the directory file path.
    // Example:   /foo/bin/crts_radio  => /foo/bin
    char *s = &buf[rl];
    while(--s > buf && *s != '/') *s = '\0';
    ASSERT(s > buf, "Failed to find the path to this program");
    *s = '\0';

    std::string path = buf;
    path += "/crts_shell";
    free(buf);

    return path;
}

/* returns allocated pointers to white space separated args in the line
 * with \" and ' escaping just on the edges
 *
 * This is now thread safe. */
static inline
char **get_args(const char *line_in, int *argc, char * &buf)
{
    /* this is a single process, single thread server
     * so we can use one allocated arg array */
    char **arg = NULL;
    size_t arg_len = 0;
    size_t count = 0;
    int sq_escape = 0, dq_escape = 0;
    char *s;

    if(buf) free(buf);
    buf = strdup(line_in);

    ASSERT(buf,"strdup() failed");

    arg = (char **) malloc((arg_len = 4)*sizeof(char *));
    ASSERT(arg, "malloc() failed");

    for(s = buf; *s; )
    {
        while(isspace(*s)) ++s;
        if(*s == '\'')
        {
            sq_escape = 1;
            ++s;
        }
        if(*s == '"')
        {
            dq_escape = 1;
        ++s;
        }
        if(!*s) break;
        ++count;
        if(count+1 > arg_len)
        {
            arg = (char **) realloc(arg, (arg_len += 4)*sizeof(char *));
            ASSERT(arg, "realloc() failed");
        }
        arg[count-1] = s;
        while(*s && ((!isspace(*s) && !sq_escape && !dq_escape)
                 || (*s != '\'' && sq_escape)
                 || (*s != '"' && dq_escape))) ++s;
        sq_escape = dq_escape = 0;
        if(*s)
        {
            *s = '\0';
            ++s;
        }
    }
    arg[count] = NULL;

    *argc = count;
    return arg;
}

static inline void sendCommand(Shell *shell, CRTSControl *c, const char *line)
{
    MUTEX_LOCK(shell);
    // Add a command to the list of commands for this CRTSControl 
    shell->commands[c].push_back(line);
    MUTEX_UNLOCK(shell);
    DSPEW("Sent command: %s", line);
}


// Instead of polling for input in the Shell::execute() method we
// just use a blocking read in another thread, here:
//
static void *shellReceiver(Shell *shell)
{
    FILE *fifoCommand;
    FILE * &fifoReply = shell->fifoReply;

    std::string &fifoDirPath = shell->fifoDirPath;

    std::string fifoCommandPath = fifoDirPath + "/fifoCommand";
    ASSERT(mkfifo(fifoCommandPath.c_str(), 0600) == 0,
            "mkfifo(\"%s\") failed", fifoCommandPath.c_str());
    DSPEW("made FIFO \"%s\"", fifoCommandPath.c_str());


    std::string command = "gnome-terminal -e \"";

    command += getShellPath() + " " + fifoCommandPath +
        " " + fifoDirPath + "/fifoReply " +
        fifoDirPath + "/controlList\"";

    errno = 0;
    // I don't completely understand this, but this fopen() call will hang
    // forever if we use the "r" mode and not "r+".  I appears that with
    // "r" it maybe waiting for input.
    fifoCommand = fopen(fifoCommandPath.c_str(), "r+");
    if(!fifoCommand)
    {
        // Try to remove the temporary directory.
        system((std::string("rm -rf ") + fifoDirPath).c_str());
        throw "Failed to open command FIFO";
    }
    setlinebuf(fifoCommand);


    // This may fail.
    DSPEW("Running: %s", command.c_str());
    if(system(command.c_str()) != 0)
    {
        WARN("Running %s FAILED", command.c_str());
        throw std::string("Running ") + command + "FAILED";
    }

 
    // If we never read a FIFO than the shell failed...
    const char *connect = "start";
    const size_t clen = strlen(connect);
    char rbuf[clen + 1];

    // TODO: can this read hang forever?

    if(fread(rbuf, clen, 1, fifoCommand) != 1)
    {
        throw "Failed to fread FIFO";
    }

    rbuf[clen] = '\0';

    ASSERT(strcmp(rbuf, connect) == 0, "did not read \"%s\" from FIFO", connect);

    DSPEW("FIFO read=\"%s\"", rbuf);

    // Try to remove the temporary directory now that we know that
    // the shell is connected to the FIFOs.
    system((std::string("rm -rf ") + fifoDirPath).c_str());

    char *line = 0;
    size_t len = 0;
    ssize_t nread;
    char *buf = 0;

    // getline() calls blocking read call.  We wait for input here:
    //
    while((nread = getline(&line, &len, fifoCommand)) != -1)
    {
        // len is the length of the buffer not necessarily the string.
        size_t l = strlen(line);
        if(l > 0 && line[l-1] == '\n')
            // remove the '\n'
            line[--l] = '\0';
        if(l < 1) continue;

        DSPEW("got command line=\"%s\"", line);

        if(strcmp(line, "exit") == 0)
        {
            auto control = shell->commands.begin()->first;
            control->filter->stream->isRunning = false;

            break;
        }

        // The line must be like:
        //
        //     set controlName freq 123
        //
        //        or
        //
        //     get controlName freq
        //
        if(strncmp(line, "set", 3) != 0 && strncmp(line, "get", 3) != 0)
        {
            DSPEW("Got bad command: %s\nnot set or get", line);
            continue;
        }

        int argc;
        char **argv = get_args(line, &argc, buf);

        if(!(argc == 4 && strncmp(argv[0], "set", 3) == 0) &&
            !(argc == 3 && strncmp(argv[0], "get", 3) == 0))
        {
            DSPEW("Got bad command: %s\nwrong number of words", line);
            if(argv) free(argv);
            continue;
        }

        CRTSControl *c;
        // argv[2] must be the name of a control, assuming this is a get
        // or set command.
        if((c = shell->findControl(argv[1])) == 0)
        {
            DSPEW("Got bad command: %s\ncontrol %s was not found", line, argv[1]);
            fprintf(fifoReply, "Bad control name %s\n", argv[1]);
            if(argv) free(argv);
            continue;
        }

        if(argv) free(argv);

        sendCommand(shell, c, line);
    }



    // Cleanup:
    //
    if(line) free(line);

    // This hangs forever.  Could be a kernel bug.
    // Threads and buffer streams have issues.
    if(fifoCommand) fclose(fifoCommand);

    // ~Shell() will close the fifoReply so that we may
    // get any remaining commands in the stream.

    // TODO: EXIT??


    return 0;
}


static void usage(void)
{
    char nameBuf[64], *name;
    name = CRTS_BASENAME(nameBuf, 64);
    fprintf(stderr,
"\n"
"\n"
"Usage: %s [ OPTIONS ]\n"
"\n"
"  OPTIONS are optional.  Transmitter Controller test module.\n"
"\n"
"\n"
"  ---------------------------------------------------------------------------\n"
"                           OPTIONS\n"
"  ---------------------------------------------------------------------------\n"
"\n"
"\n"
"   --help             print this help\n"
"\n", name);

}



Shell::Shell(int argc, const char **argv):
    mutex(PTHREAD_MUTEX_INITIALIZER), fifoReply(0)
{
    CRTSModuleOptions opt(argc, argv, usage);

    // This Controller should be loaded after all Filters that you wish to
    // control.
    //

    {
        // Create a tmp directory that will hold our two FIFOs
        // and the controls list file.
        //
        char temp[] = "/tmp/crts_XXXXXX";
        char *tmp_dirname = mkdtemp(temp);
        ASSERT(tmp_dirname, "mkdtemp() failed");
        fifoDirPath = tmp_dirname;
    }

    {
        std::string controlListPath = fifoDirPath + "/controlList";
        FILE *controlList = fopen(controlListPath.c_str(), "w");
        if(!controlList)
        {
            WARN("fopen(\"%s\", \"w\") failed", controlListPath.c_str());
            system((std::string("rm -rf ") + fifoDirPath).c_str());
            std::string s("fopen(\"");
            throw s + controlListPath + "\", \"w\") failed";
        }

        CRTSControl *c;
        while((c = getControl<CRTSControl *>()))
        {
            std::list<std::string> list;
            commands[c] = list;

            int i = 0;
            std::string name = c->getNextParameterName(true/*start*/);
            while(name.length())
            {
                if(i++ == 0) fprintf(controlList, "%s", c->getName());
                fprintf(controlList, " %s", name.c_str());
                name = c->getNextParameterName();
            }
            if(i)
            {
                putc('\n', controlList);
                DSPEW("added shell command interface for control: %s with %d parameters",
                        c->getName(), i);
            }
        }

        fclose(controlList);
    }

    if(commands.size() <= 0) 
        throw "No controls were found";


    std::string fifoReplyPath = fifoDirPath + "/fifoReply";
    ASSERT(mkfifo(fifoReplyPath.c_str(), 0600) == 0,
            "mkfifo(\"%s\") failed", fifoReplyPath.c_str());
    DSPEW("made FIFO \"%s\"", fifoReplyPath.c_str());
    errno = 0;
    // This hangs forever if we use mode "w" and not "w+"
    fifoReply = fopen(fifoReplyPath.c_str(), "w+");
    if(!fifoReply)
    {
        // Try to remove the temporary directory.
        system((std::string("rm -rf ") + fifoDirPath).c_str());
        throw "Failed to open reply FIFO";
    }
    setlinebuf(fifoReply);




    pthread_t thread;

    ASSERT(pthread_create(&thread, 0,
                (void *(*) (void *)) shellReceiver,
                (void *) this) == 0, "pthread_create() failed");
    ASSERT(pthread_detach(thread) == 0, "pthread_detach() failed");

    DSPEW();
}


Shell::~Shell(void)
{

    // This hangs forever??
    if(fifoReply) fclose(fifoReply);

    DSPEW();
};


void Shell::start(CRTSControl *c)
{
    DSPEW("Shell::start(%s)", c->getName());
}


void Shell::stop(CRTSControl *c)
{
    DSPEW("Shell::start(%s)", c->getName());
}


void Shell::execute(CRTSControl *c, const void *buffer,
        size_t len, uint32_t channelNum)
{
    //DSPEW("control=\"%s\"", c->getName());

    std::list<std::string> &list = commands[c];

    MUTEX_LOCK(this);

    char *buf = 0;
    
    while(!list.empty())
    {
        std::string *commandLine = &list.front();

        DSPEW("Control \"%s\" got command line=\"%s\"",
                c->getName(),
                commandLine->c_str());

        int argc;
        double val = NAN;
        char **argv = get_args(commandLine->c_str(), &argc, buf);

        if(strcmp(argv[0], "set") == 0 && argc >= 4)
        {
            // Like:
            //
            //   set tx freq 900
            //
            // argv[1] should be the control name.
            //
            char *end = 0;
            val = strtod(argv[3], &end);
            if(end == argv[3])
                NOTICE("Bad double value from \"%s\"", argv[3]);
            else
                c->setParameter(argv[2], val);
        } 
        else if(strcmp(argv[0], "get") == 0)
        {
            // Like" get tx freq

            if(argc >= 3)
                val = c->getParameter(argv[2]);

            std::string reply(argv[1]);
            reply += " ";
            reply += argv[2];
            reply += " ";
            reply += std::to_string(val);
            reply += "\n";

            DSPEW("argc=%d val=%g writing reply: %s", argc, val, reply.c_str());

            // We assume that writing to a FILE is thread safe.
            //
            if(1 != fwrite(reply.c_str(), reply.length(), 1, fifoReply))
            {
                WARN("failed to write reply to fifo");
            }
        }

        if(argv) free(argv);

        list.pop_front();

        if(buf)
        {
            free(buf);
            buf = 0;
        }
    }

    MUTEX_UNLOCK(this);
}


// Define the module loader stuff to make one of these class objects.
CRTSCONTROLLER_MAKE_MODULE(Shell)
