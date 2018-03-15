#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <sys/syscall.h>
#include <pthread.h>

#ifndef SYS_gettid
#  error "SYS_gettid unavailable on this system"
#endif

#include "crts/debug.h"


static void _vspew(FILE *stream, int errn, const char *pre, const char *file,
        int line, const char *func,
        const char *fmt, va_list ap)
{
    // We try to buffer this so that prints do not get intermixed with
    // other prints.
#define BUFLEN  1024
    char buffer[BUFLEN];
    int len;

    if(errn)
    {
        char estr[128];
        strerror_r(errn, estr, 128);
        len = snprintf(buffer, BUFLEN, "%s%s:%d:pid=%u:%zu %s():errno=%d:%s: ",
                pre, file, line,
                getpid(), syscall(SYS_gettid), func,
                errn, estr);
    }
    else
        len = snprintf(buffer, BUFLEN, "%s%s:%d:pid=%u:%zu %s(): ", pre, file, line,
                getpid(), syscall(SYS_gettid), func);

    if(len < 10 || len > BUFLEN - 40)
    {

        //
        // Try again without buffering
        //
        if(errn)
        {
            char estr[128];
            strerror_r(errn, estr, 128);
            fprintf(stream, "%s%s:%d:pid=%u:%zu %s():errno=%d:%s: ",
                    pre, file, line,
                    getpid(), syscall(SYS_gettid), func,
                    errn, estr);
        }
        else
            fprintf(stream, "%s%s:%d:pid=%u:%zu %s(): ", pre, file, line,
                    getpid(), syscall(SYS_gettid), func);
    
        vsnprintf(&buffer[len], BUFLEN - len,  fmt, ap);

        return;
    }
    vsnprintf(&buffer[len], BUFLEN - len,  fmt, ap);
    fputs(buffer, stream);
}

void _spew(FILE *stream, int errn, const char *pre, const char *file,
        int line, const char *func, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    _vspew(stream, errn, pre, file, line, func, fmt, ap);
    va_end(ap);
}


void _assertAction(FILE *stream)
{
    pid_t pid;
    pid = getpid();
#ifdef ASSERT_ACTION_EXIT
    fprintf(stream, "Will exit due to error\n");
    exit(1); // atexit() calls are called
    // See `man 3 exit' and `man _exit'
#else // ASSERT_ACTION_SLEEP
    int i = 1; // User debugger controller, unset to effect running code.
    fprintf(stream, "  Consider running: \n\n  gdb -pid %u\n\n  "
        "pid=%u:%zu will now SLEEP ...\n", pid, pid, syscall(SYS_gettid));
    while(i) { sleep(1); }
#endif
}


