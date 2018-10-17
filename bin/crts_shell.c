/* This program just reads stdin, then writes the FIFO, then reads the
 * FIFO, then echos what it read from the FIFO, and then repeats.
 *
 * What ever runs this program needs to connect to the FIFO before running
 * this.  This program will unlink the FIFO file after opening it, so that
 * no other programs may see the FIFO, and it will be cleaned up by the
 * OS.
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

#include "crts/debug.h"

static char *prompt = "> ";
static bool stdin_isatty;
static bool use_readline;
static FILE *fifo_file = 0;

static
void term_sighandler(int sig_num)
{
    printf("We caught signal %d\n", sig_num);
    printf("exiting\n");
    exit(1);
}

/* returns allocated pointers to white space separated args in the line
 * with \" and ' escaping just on the edges */
static inline
char **get_args(char *line, int *argc)
{
    /* this is a single process, single thread server
     * so we can use one allocated arg array */
    char **arg = NULL;
    size_t arg_len = 0;
    size_t count = 0;
    int sq_escape = 0, dq_escape = 0;
    char *s;

    ASSERT(line,"");

    arg = malloc((arg_len = 4)*sizeof(char *));
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
            arg = realloc(arg, (arg_len += 4)*sizeof(char *));
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

int sendToFifo(const char *buf)
{
    size_t len;
    len = strlen(buf);

    if(fwrite(buf, len, 1, fifo_file) != 1)
    {
        printf("fwrite(,\"%s\",FIFO) failed\n", buf);
        fclose(fifo_file);
        fifo_file = 0;
        return 1;
    }
    fprintf(fifo_file, "\n");
    fflush(fifo_file);
    return 0; // success
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


int checkForQuit(const char *line)
{
     while(isspace(*line)) ++line;
     if(strncmp(line, "quit", 4) == 0 ||
            strncmp(line, "exit", 4) == 0)
         return 1;
     return 0;
}

char *commands[] = {
    "set",
    "get",
    "help",
    "Zaphod",
    NULL
};


char *generator(const char *text, int state)
{
    static int list_index, len;
    char *name;

    //printf("\nstate=%d > ", state);

    if (!state) {
        list_index = 0;
        len = strlen(text);
    }

    while ((name = commands[list_index++])) {
        if (strncmp(name, text, len) == 0) {
            return strdup(name);
        }
    }

    return NULL;
}

char **completion(const char *text, int start, int end)
{
    rl_attempted_completion_over = 1;
    return rl_completion_matches(text, generator);
}



int main (int argc, char **argv)
{
    char *user_line = NULL;
    size_t user_len = 0;
    int ret = 0;

    if(argc != 2 ||
            strcmp(argv[1], "--help") == 0 ||
            strcmp(argv[1], "-h") == 0)
    {
        printf("Usage: %s FIFO_FILENAME\n", argv[0]);
        ret = 1;
        goto cleanup;
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


    fifo_file = fopen(argv[1], "r+");
    // We remove this FIFO file so it's not seen by other processes and
    // keep using it until we are done.
    unlink(argv[1]);
    if(!fifo_file)
    {
        printf("Failed to open FIFO file \"%s\"\n", argv[1]);
        ret = 3;
        goto cleanup;
    }
    //setlinebuf(fifo_file);

    if(sendToFifo("shell connecting"))
    {
        ret = 4;
        goto cleanup;
    }

    rl_attempted_completion_function = completion;
    
    while((Getline(&user_line, &user_len)))
    {
        if(!user_line[0]) continue;

        if(checkForQuit(user_line))
            break;

        add_history(user_line);

        if(sendToFifo(user_line))
        {
            ret = 4;
            break;
        }
    }


cleanup:

    if(fifo_file)
        sendToFifo("shell exiting");

    printf("\n%s exiting with return status=%d\n", argv[0], ret);
    
    return ret;
}

