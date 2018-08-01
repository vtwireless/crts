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
#include <vector>
#include <gnu/lib-names.h>

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


static inline void _crtsThrowUsage(const char *arg, const char *opt,
        void (*usage)(void))
{
    if(arg)
        fprintf(stderr,
"\n"
"    Got exceptional CRTS plug-in module option: %s%s%s"
"\n",
    arg?arg:"", opt?" ":"", opt?opt:"");

    if(usage)
        usage();

    throw "CRTS plug-in module usage";
}


static inline void _crtsDefaultUsage(void)
{
    fprintf(stderr,
"\n"
"    Who ever wrote this CRTS module did not pass in a\n"
"  usage() function.  So this text is what you get.\n"
"\n"
"\n");
}

// A very small option argument parsing class for CRTS modules.  Meant to
// be declared on the stack in your super class CRTSFilter, CRTSModule, or
// CRTSController constructor.  This seems to be less messy than having
// this object built into the CRTS base module class.  The CRTS has the
// option of not using this, and using it just adds one line of code to
// their constructor and one function call per option.  It can't get less
// intrusive than that...
//
// TODO: Have the options parsing built into the CRTS base module class
// object. I don't think so at this time...
//
// TODO: Plug-in module configuration from configuration files and script
// interpreters (like Python).  Hence the name ModuleOptions and not
// ParseArgumentOptions or whatever.  So now we just parse command line
// arguments, but we could extend it to parsing a configuration file.
// This interface does not seem to fit the script interpreter idea, but
// it will interfere with it.
//
class CRTSModuleOptions
{
    public:

        CRTSModuleOptions(int argc_in, const char **argv_in,
                void (*usage_in)(void) = _crtsDefaultUsage):
            argc(argc_in), argv(argv_in), usage(usage_in)
        {
            // We start by checking for --help|-h
            //
            // argv is not necessarily Null terminated, so
            // we use an "i" counter.
            for(int i=0; i<argc; ++i)
                if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
                    _crtsThrowUsage(argv[i], 0, usage);
        };

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
                _crtsThrowUsage(arg, optName, usage);
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
                _crtsThrowUsage(arg, optName, usage);
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
                _crtsThrowUsage(arg, optName, usage);
            return ret;
        };

        uint32_t get(const char *optName, uint32_t defaultVal)
        {
            return get(optName, (unsigned long) defaultVal);
        };

        // This seems to get size_t too.
        unsigned long get(const char *optName, unsigned long defaultVal,
                char **end = 0)
        {
            const char *arg = 0;
            const char *opt = 0;
            long ret = defaultVal;
            getInit(arg, opt, optName);
            if(!arg) return ret;

            char *endptr = 0;
            errno = 0;
            ret = strtoul(arg, &endptr, 10);
            if(errno || arg == endptr)
                _crtsThrowUsage(arg, optName, usage);
            if(end) *end = endptr;
            return ret;
        };



        template <class T>
        std::vector<T> getV(const char *optName, T defaultVal)
        {
            const char *arg = 0;
            const char *opt = 0;
            std::vector<T> ret;
            getInit(arg, opt, optName);
            if(!arg)
            {
                ret = { defaultVal };
                return ret;
            }

            opt = arg;
            char *end = 0;
            do
                ret.push_back(strtoT<T>(arg, &end, optName));
            while(*end && end);

            std::string str = "[";
            for(size_t i=0; i<ret.size(); ++i)
                str += std::to_string(ret[i]) + ",";
            str += "]";

            DSPEW("Got option vector \"%s\" from arg=\"%s\"",
                    str.c_str(), arg);

            return ret;
        };


#if 0
        size_t get(const char *optName, size_t defaultVal)
        {
            return get(optName, (unsigned long) defaultVal);
        };
#endif

    private:


        template <class T>
        T strto(const char * s, char **end, int base)
        {
            if(std::is_same<T, double>::value)
                return (T) strtod(s, end);
            if(std::is_same<T, unsigned long>::value)
                return (T) strtoul(s, end, base);
            return 0;
        };

        template <class T>
        T strtoT(const char * &s, char **end, const char *optName)
        {
            errno = 0;
            T val = strto<T>(s, end, 10);

            if(errno || *end == s)
                _crtsThrowUsage(s, optName, usage);
            if(!(*end) || *end == s)
                *end = 0;
            else
                s = *end+1; // go to next
            return val;
        };


        // It could be that opt and optName are not the same if we
        // parse option arguments like: --file=foobar
        //
        void getInit(const char * &arg, const char * &opt, const char *optName)
        {
            DASSERT(optName && optName[0], "");
            DASSERT(usage, "");

            // argv is not necessarily Null terminated, so
            // we use an "i" counter.
            for(int i=0; i<argc; ++i)
                if(!strcmp(argv[i], optName) && i<argc+1)
                {
                    opt = argv[i];
                    arg = argv[++i];
                    continue;
                }
        };

        int argc;
        const char **argv;
        void (*usage)(void);
};



// The small crappy TCP/IP socket wrapper that sends a very limited form
// of JSON to and from the server that it connects to.
//
// We assume that messages are small BUFSIZE so that no memory allocation
// is necessary.
//
// CRTSTcpConnection::CRTSTcpConnection() will throw an exception if it
// fails.
//
class CRTSTcpConnection
{
    public:


        CRTSTcpConnection(const char *firstMessage, const char *toAddress,
                int port);
        virtual ~CRTSTcpConnection(void);

        bool load(const char *key, double value);
        bool send(const char *buf = 0); // like flush

        char *receive(void);

    private:

        int fd; // socket file descriptor.

        // fixed size of send and recv buffer.
        static const size_t BUFSIZE = (1024 * 2); 

        char recvBuffer[BUFSIZE];
        char sendBuffer[BUFSIZE];
};




#endif //#ifndef __crts_hpp__
