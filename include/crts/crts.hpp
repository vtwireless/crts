#ifndef __crts_hpp__
#define __crts_hpp__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdarg.h>
#include <errno.h>
#include <string>
#include <typeinfo>

#include <crts/debug.h>


#define CRTS_BASENAME(buf, len)   getModuleName(buf, len, __BASE_FILE__)


extern bool crtsRequireModule(const char *name,
        int argc = 0, const char **argv = 0);



#ifdef __cplusplus
extern "C" {
#endif
    


// Thread safe.
// Returns a pointer to some char in the buf that was passed in.
// Writes to memory in buf.
// buf must not be 0.  len must be a non-zero length of buf.
static inline char *getModuleName(char *buf, size_t len, const char *base)
{
    snprintf(buf, len, "%s", base);
    len = strlen(buf);
    if(len < 4 || strcmp(".cpp", &buf[len-4]))
        // It does not end in ".cpp"
        return buf;

    // It does end in .cpp, now strip it off
    buf[len-4] = '\0';

    // Remove all chars before the last '/'.
    // TODO: port '/' to non UNIX
    char *s = &buf[len-4];

    while(s != buf && *s != '/') --s;

    if(*s == '/')
        return s+1;

    return buf;
}

extern FILE *crtsOut;


#ifdef __cplusplus
}
#endif



class CRTSModuleOptions
{
    public:

        CRTSModuleOptions(int argc_in, const char **argv_in,
                void (*usage)(void) = 0):
            argc(argc_in), argv(argv_in)
        { };

        virtual ~CRTSModuleOptions(void) { };

        const char *get(const char *optName, const char * defaultVal)
        {
            const char *arg = defaultVal;
            const char *opt = 0;
            getInit(arg, opt, optName);
            return arg;
        };

        double get(const char *optName, double defaultVal)
        {
            const char *arg = 0;
            const char *opt = 0;
            double ret = defaultVal;
            getInit(arg, opt, optName);
            if(!arg) return ret;

            char *endptr = 0;
            errno = 0;
            ret = strtod(arg, &endptr);
            if(errno || arg == endptr)
            {
                fprintf(stderr, "Bad option: %s %s\n\n", opt, arg);
                usage();
                throw "usage help";
            }
            return ret;
        };

        float get(const char *optName, float defaultVal)
        {
            const char *arg = 0;
            const char *opt = 0;
            float ret = defaultVal;
            getInit(arg, opt, optName);
            if(!arg) return ret;

            char *endptr = 0;
            errno = 0;
            ret = strtof(arg, &endptr);
            if(errno || arg == endptr)
            {
                fprintf(stderr, "Bad option: %s %s\n\n", opt, arg);
                usage();
                throw "usage help";
            }
            return ret;
        };

        long get(const char *optName, long defaultVal)
        {
            const char *arg = 0;
            const char *opt = 0;
            long ret = defaultVal;
            getInit(arg, opt, optName);
            if(!arg) return ret;

            char *endptr = 0;
            errno = 0;
            ret = strtol(arg, &endptr, 10);
            if(errno || arg == endptr)
            {
                fprintf(stderr, "Bad option: %s %s\n\n", opt, arg);
                usage();
                throw "usage help";
            }
            return ret;
        };

    private:

        void getInit(const char * &arg, const char * &opt, const char *optName)
        {
            DASSERT(optName && optName[0], "");
            int i;
            for(i=0; i<argc; ++i)
            {
                if(usage &&
                        (!strcmp(argv[i], "-h") ||
                        !strcmp(argv[i], "--help")))
                {
                    usage();
                    throw "usage help";
                }
                if(!strcmp(argv[i], optName) && i<argc+1)
                {
                    opt = argv[i];
                    arg = argv[++i];
                    continue;
                }
            }
            // We don't need to check for --help or -h again.
            usage = 0;
        }

        int argc;
        const char **argv;
        void *(*usage)(void);
};



#endif //#ifndef __crts_hpp__
