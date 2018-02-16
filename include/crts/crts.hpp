#ifndef __crts_hpp__
#define __crts_hpp__

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdarg.h>


#define CRTSFILTER_NAME(buf, len)   getModuleName(buf, len, __BASE_FILE__)


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


#endif //#ifndef __crts_hpp__
