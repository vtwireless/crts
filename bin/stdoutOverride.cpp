#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#include "crts/crts.hpp"
#include "crts/debug.h"

// This gets called before we can know whither or not we really need to do
// this, because we cannot parse command-line options before libuhd.so is
// loaded.

// If libboost*.so and libuhd.so get fixed, and stops writing to stdout
// when they are loaded, this code will not be required for crts_radio to
// be able to use bash pipe lines, but this hack will not do anything that
// bad.
//
// Since now we are loading libuhd only in plugins it will be loaded
// after the calling of main().  That was not the case when we linked
// with libuhd with crts_radio.

//
// In the past: we needed to link to a dynamic linked library to programs that link
// with libuhd.so.  We get this library to load before libuhd.so so we can
// stop libuhd.so from doing stupid things when it is loaded.
// Putting a constructor function in the source of the binary program did
// not get called before the libuhd.so constructor.  We link our programs
// so that this constructor is called before the constructor in libuhd.so.
// The linker command line order mattered.
/*

 Related discussion but not what we did here, ref:
   https://stackoverflow.com/questions/22941756/how-do-i-prevent-a-\
library-to-print-to-stdout-in-linux-c


*/
static FILE *stdoutOverride(void)
{
    FILE *crtsOut;

    //fprintf(stderr, "%s:%d: calling %s()\n", __FILE__, __LINE__, __func__);

    // When this is called we assume that there has been no data written
    // to the stdout stream or stdout file descriptor.  We need to setup a
    // new file descriptor to catch the stdout stream data from the stupid
    // libuhd.so library and send it to the stderr file descriptor.

    // playing dup games requires that we know what STDOUT_FILENO is and
    // etc.
    DASSERT(STDIN_FILENO == 0, "");
    DASSERT(STDOUT_FILENO == 1, "");
    DASSERT(STDERR_FILENO == 2, "");

    ASSERT(dup2(1,3) == 3, ""); // save stdout FD in 3
    // The file in the kernel which was fd 1 is now fd 3,
    // so writing to fd 3 will go to what the shell setup
    // before now as stdout and fd 1.

    ASSERT(dup2(2,1) == 1, ""); // make 1 be like stderr

    // now stdout is writing to what is also stderr.  The awkward thing is
    // that stdout and stderr have different buffers and different
    // flushing rules.  The two streams will be in correct order within
    // themselves but there will be no way to know the order between the
    // two merged streams.

    // Make crtsOut act like stdout stream used to.
    ASSERT((crtsOut = fdopen(3, "w")) != NULL, "");
    // Now writing to crtsOut will write out to a bash pipe line, for
    // example.

    // We can now write to crtsOut to write to what appears as stdout in a
    // bash pipe line.

#if 0 // testing crtsOut
    fprintf(crtsOut, "crtsOut %s:%d: calling %s()\n", __FILE__, __LINE__, __func__);
    fflush(crtsOut);
#endif

    return crtsOut;
}

// crtsOut will act like stdout after startupFunction() gets called.
// Note: that will happen before main() is called.
FILE *crtsOut = stdoutOverride();
