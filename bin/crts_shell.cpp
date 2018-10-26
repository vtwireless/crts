/* This program just reads stdin, then writes the FIFO_COMMAND_FILE, then
 * maybe reads the FIFO_REPLY_FILE and echos what it read from the
 * FIFO_REPLY_FILE, and then repeats.
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <readline/history.h>

#include <map>
#include <string>

#include "crts/debug.h"

static const char *prompt = "> ";
static bool stdin_isatty;
static bool use_readline;
static FILE *fifoCommand = 0, *fifoReply = 0;

static
void term_sighandler(int sig_num)
{
    printf("We caught signal %d\n", sig_num);
    printf("exiting\n");
    sleep(4);
    exit(1);
}

/* returns allocated pointers to white space separated args in the line
 * with \" and ' escaping just on the edges.  The strings that are pointed
 * to get reused in the next get_args() call, if you need them to persist
 * copy them.  If buf is set it will point to all the string memory and
 * the user may free it. */
static inline
const char **get_args(const char *line_in, int *argc, char **buf = 0)
{
    const char **arg = NULL;
    size_t arg_len = 0;
    size_t count = 0;
    int sq_escape = 0, dq_escape = 0;
    char *s;
    // This is not thread safe, and we have no threads.
    static char *line = 0;

    if(line) free(line);

    line = strdup(line_in);
    ASSERT(line,"strdup() failed");

    arg = (const char **) malloc((arg_len = 4)*sizeof(char *));
    ASSERT(arg, "malloc() failed");

    for(s = line; *s; )
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
            arg = (const char **) realloc(arg, (arg_len += 4)*sizeof(char *));
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


    if(buf)
    {
        // We let the user manage the line memory in this case.
        *buf = line;
        line = 0;
    }
    // else we free the line memory in the start of the next call.

    return arg;
}

static void sendCommand(const char *buf)
{
    size_t len;
    len = strlen(buf);

    if(fwrite(buf, len, 1, fifoCommand) != 1)
    {
        printf("fwrite(,\"%s\",FIFO) failed\n", buf);
        sleep(3);
        exit(6);
    }
    fprintf(fifoCommand, "\n");
    fflush(fifoCommand);
}



static inline
int Getline(char **line, size_t *len)
{
    if(use_readline)
    {
        if(*line)
            free(*line);
        *line = readline(prompt);
        if(*line)
            return 1;
        else
        {
            return 0;
        }
    }
    else
    {
        size_t l;

        printf("%s", prompt);
        fflush(stdout);

        if(getline(line, len, stdin) == -1)
            return 0;

        l = strlen(*line);
        ASSERT(l > 0,"");
        /* remove newline '\n' */
        (*line)[l-1] = '\0';
        if(!stdin_isatty)
            printf("%s\n", *line);
        return 1;
    }
}


static const char *commands0[] =
{
    "detach",
    "exit",
    "get",
    "help",
    "set",
    "sleep",
    0
};

static inline int printHelp(void)
{
    // TODO: run a pager like "less" and then resume after it exits.

    printf(
        "--------------------------- HELP -----------------------------\n"
        "--------------------------------------------------------------\n"
        "The simple CRTS shell controller\n"
        "\n"
        "Enter commands that are applied to the CRTS flow stream.\n"
        "The following commands are available:\n\n  ");
    for(const char **com = commands0; *com; ++com)
        printf(" %s", *com);
    printf(
        "\n"
        "The main commands are \"set\" and \"get\" which are used to\n"
        "set and get control parameters that are in the CRTS flow stream.\n"
        "\n"
        "   Pseudo Examples:\n"
        "\n"
        "        %sset FILTER_NAME PARAMETER_NAME VALUE\n"
        "\n"
        "where FILTER_NAME is the name of a filter (and control) name,\n"
        "PARAMETER_NAME is the name of a parameter in that filter, and\n"
        "VALUE is the floating point number that we are trying to set the\n"
        "parameter with PARAMETER_NAME to (though it may be converted to\n"
        "other types)\n"
        "\n"
        "        %sget FILTER_NAME PARAMETER_NAME\n"
        "\n"
        "can be used to print a filter parameter\n"
        "\n"
        "<TAB> completion is your friend.\n"
        "\n"
        "                        COMMANDS\n"
        "   detach       will terminate the shell without effecting the stream\n"
        "                flow.\n"
        "\n"
        "   exit         will terminate the shell and the CRTS stream.\n"
        "\n"
        "   get FN PN    will print the filter FN parameter named PN.\n"
        "\n"
        "   help         print this help\n"
        "\n"
        "   set FN PN VL will set the filter FN parameter named PN to VL.\n"
        "\n"
        "   sleep SEC    will pause this shell for SEC seconds.  This could\n"
        "                be useful for non-interactive use were command lines\n"
        "                are read from standard input.\n"
        "\n"
        "--------------------------------------------------------------\n"
        "--------------------------------------------------------------\n"
        "\n",
        prompt, prompt
    );

    

    return 1;
}


static inline bool isWhiteChar(const char c)
{
    // See 'man ascii'.
    //
    return (c > 0 && c <= ' ');
}


static inline int checkForLocalCommand(const char *line)
{
    // Strip leading white space.
    while(isWhiteChar(*line)) ++line;

    size_t llen = strlen(line);

    if(strncasecmp(line, "help", 4) == 0 || *line == '?')
        return printHelp(); // got it, stop parsing.

    if(strncmp(line, "detach", 6) == 0)
    {
        printf("shell exiting while stream is left running\n");
        sleep(4);
        exit(0);
    }

    if(strncmp(line, "sleep", 5) == 0)
    {
        int argC = 0;
        const char **argV = get_args(line, &argC);

        if(argC < 2)
        {
            printf("missing arguement\n");
            return 1;
        }
        char *endptr = 0;
        double dsec = strtod(argV[1], &endptr);
        if(endptr == argV[1] || dsec < 0.0)
        {
            printf("bad arguement: %s\n", argV[1]);
            return 1;
        }
        printf("shell sleeping %g seconds\n", dsec);

        // This is very crude, but is okay.
        unsigned int sec = dsec;
        useconds_t usec = (1000000 * (dsec - sec));
        if(sec)
            sleep(sec);
        if(usec)
            usleep(usec);
        return 1;
    }


    for(const char **com = commands0; *com; ++com)
    {
        size_t clen = strlen(*com);
        if(llen > clen && strncmp(*com, line, clen) == 0
                && isWhiteChar(line[clen]))
            // This may be a good command
            return 0; // go to the next parsing thing.
    }
    
    // They entered no valid command so we handle it here:
    printf("type help for help\n");
    return 1; // stop parsing.
}


static inline int checkForQuit(const char *line)
{
     while(isWhiteChar(*line)) ++line;

     if(strncmp(line, "quit", 4) == 0 ||
            strncmp(line, "exit", 4) == 0)
     {
         sendCommand("exit");
         return 1;
     }
     return 0;
}

static const char **commands;

static const char **setControlNames = 0;
static const char **getControlNames = 0;


// findWorkingArgIndex(() Examples:
//
// text="   " => 0  we are working on the first (0) argument
// text=" ff" => 0  we are working on the first (0) argument
// text=" f " => 1  we are working on the second (1) argument
// text=" f f" => 1 we are working on the second (1) argument
// text=" f f " => 2 we are working on the third (2) argument
// etc ....
//
static inline int findWorkingArgIndex(const char *text)
{
    int count = 0;

    while(*text)
    {
        while(isWhiteChar(*text)) ++text;
        if(! *text) return count;
        // now it's not white or 0
        while(*text && !isWhiteChar(*text)) ++text;
        if(isWhiteChar(*text)) ++count;
    }
    return count;
}

// A list of the parameter names keyed with control names
//
// Parameters we can set
static std::map<std::string, const char **> setParameters;
// Parameters we can get
static std::map<std::string, const char **> getParameters;


// The readline completion generator.
//
static char *generator(const char *text, int state)
{
    static int list_index, len;

#if 0
    int nargs = 0;
    char **command = get_args(rl_line_buffer, &nargs);
    
    printf("command=");
    for(int i=0; i<nargs; ++i)
        printf("|%s", command[i]);
    printf("\n");

    printf("\nstate=%d text=\"%s\" rl_line_buffer=\"%s\"> ", state, text, rl_line_buffer);

    free(command);
#endif

    if(!state)
    {
        list_index = 0;
        len = strlen(text);

        int argc = 0;
        int argIndex = findWorkingArgIndex(rl_line_buffer);
        const char **argv = get_args(rl_line_buffer, &argc);

        switch(argIndex)
        {
            case 0:
                commands = commands0;
                break;
    
            case 1:
                if(strcmp(argv[0],"set") == 0)
                    commands = setControlNames;
                else if(strcmp(argv[0],"get") == 0)
                    commands = getControlNames;
                else
                    commands = 0;
                break;

            case 2: // example:  set controlName

                if(strcmp(argv[0],"set") == 0)
                {
                    auto it = setParameters.find(argv[1]);

                    if(it != setParameters.end())
                    {
                        commands = it->second;
                    }
                    else
                    {
                        commands = 0;
                    }
                }
                else if(strcmp(argv[0],"get") == 0)
                {
                    auto it = getParameters.find(argv[1]);

                    if(it != getParameters.end())
                    {
                        commands = it->second;
                    }
                    else
                    {
                        commands = 0;
                    }
                }
                else
                    commands = 0;
                break;

            default:
                // nargs > 2
                commands = 0;
                break;
        };
    }

    const char *name;

    if(!commands) return 0;

    while((name = commands[list_index++]))
        if(strncmp(name, text, len) == 0)
            return strdup(name);

    return 0;
}

static char **completion(const char *text, int start, int end)
{
    rl_attempted_completion_over = 1;
    return rl_completion_matches(text, generator);
}



int main (int argc, char **argv)
{
    char *line = 0;
    size_t user_len = 0;


    if(argc != 4 ||
            strcmp(argv[1], "--help") == 0 ||
            strcmp(argv[1], "-h") == 0)
    {
        printf("Usage: %s FIFO_COMMAND_FILE FIFO_REPLY_FILE CONTROL_LIST\n", argv[0]);
        return 1;
    }

    signal(SIGTERM, term_sighandler);
    signal(SIGQUIT, term_sighandler);
    signal(SIGINT, term_sighandler);

    stdin_isatty = isatty(fileno(stdin));

    if(stdin_isatty)
        use_readline = 1;
    else
        use_readline = 0;

    if(use_readline)
        printf("Using readline version: %d.%d\n",
            RL_VERSION_MAJOR, RL_VERSION_MINOR);
    else
        printf("Using getline()\n");

    fifoCommand = fopen(argv[1], "w");
    // We remove this FIFO file so it's not seen by other processes and
    // keep using it until we are done.
    unlink(argv[1]);
    if(!fifoCommand)
    {
        printf("Failed to open FIFO command file \"%s\"\n", argv[1]);
        unlink(argv[2]);
        return 3;
    }
    // I think readline does this for me.
    //setlinebuf(fifoCommand);

    fifoReply = fopen(argv[2], "r");
    // We remove this FIFO file so it's not seen by other processes and
    // keep using it until we are done.
    unlink(argv[2]);
    if(!fifoReply)
    {
        printf("Failed to open FIFO reply file \"%s\"\n", argv[2]);
        return 4;
    }
    setlinebuf(fifoReply);

    FILE *controlList = fopen(argv[3], "r");
    if(!controlList)
    {
        printf("Failed to open control list file \"%s\"\n", argv[3]);
        sleep(100);
        return 5;
    }

    size_t numSetControlNames = 0, numGetControlNames = 0;

    // The lines should be like:
    //
    //    set filterName freq gain rate
    //
    //          or
    //
    //    get filterName freq gain rate
    //
    while(getline(&line, &user_len, controlList) != -1)
    {
        const char **argV;
        int argC = 0;
        char *buf;
        argV = get_args(line, &argC, &buf);
        ASSERT(argC >= 2, "");

        if(strcmp(argV[0],"set") == 0)
        {
            // We have a parameter we can set
            //
            // like:  set filterName freq gain rate
            //
            setControlNames = (const char **) realloc(setControlNames,
                    sizeof(const char *)*((++numSetControlNames)+1));
            ASSERT(setControlNames, "realloc() failed");
            setControlNames[numSetControlNames-1] = argV[1];
            ASSERT(setControlNames[numSetControlNames-1], "strdup() failed");
            setControlNames[numSetControlNames] = 0;

            setParameters[argV[1]] = &argV[2];
        }
        else if(strcmp(argV[0],"get") == 0)
        {
            // We have a parameter we can get
            //
            // like:  get filterName freq gain rate
            //
            getControlNames = (const char **) realloc(getControlNames,
                    sizeof(const char *)*((++numGetControlNames)+1));
            ASSERT(getControlNames, "realloc() failed");
            getControlNames[numGetControlNames-1] = argV[1];
            ASSERT(getControlNames[numGetControlNames-1], "strdup() failed");
            getControlNames[numGetControlNames] = 0;

            getParameters[argV[1]] = &argV[2];
        }
        else
        {
            // This should not happen
            WARN("Bad Control List line: %s", line);
        }
    }

    fclose(controlList);

    if(!setParameters.empty())
    {
        printf("\nParameters you can set\n"
                "------------------------------------\n");

        for(auto it = setParameters.begin(); it != setParameters.end(); ++it)
        {
            printf("%s:", it->first.c_str());
            for(const char **s = it->second; *s; ++s)
                printf(" %s", *s);
            printf("\n");
        }
        printf("\n");
    }

    if(!getParameters.empty())
    {
        printf("\nParameters you can get\n"
                "------------------------------------\n");

        for(auto it = getParameters.begin(); it != getParameters.end(); ++it)
        {
            printf("%s:", it->first.c_str());
            for(const char **s = it->second; *s; ++s)
                printf(" %s", *s);
            printf("\n");
        }
        printf("\n");
    }

    printf("Hit <TAB> for command line completion (like in a bash terminal)\n");

    sendCommand("start");

    rl_attempted_completion_function = completion;
    commands = commands0;

    while((Getline(&line, &user_len)))
    {
        if(!line[0]) continue;

        if(checkForQuit(line))
            break;

        add_history(line);
        
        if(checkForLocalCommand(line))
            continue;

        sendCommand(line);
        
        int argC = 0;
        const char **argV = get_args(line, &argC);
        if(strcmp(argV[0], "get") == 0 && argC >= 3)
        {
            // wait for a reply from this "get":
            char *lin = 0;
            size_t len = 0;
            ssize_t nread = getline(&lin, &len, fifoReply);
            if(nread == -1)
            {
                WARN("getline() failed to read reply");
                break;
            }

            // Print the reply and a new prompt.
            //
            DASSERT(lin, "");
            printf("%s", lin);
            if(lin[strlen(lin)-1] != '\n')
                printf("\n");
            //printf("%s", prompt);
            //fflush(stdout);
            free(lin);
        }
    }

    sendCommand("exit");

    printf("\n%s exiting with return status=%d\n", argv[0], 0);

    sleep(6); // so that they are see the printed output
              // before the terminal closes.

    return 0;
}
