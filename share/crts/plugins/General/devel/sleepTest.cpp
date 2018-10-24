#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

#include "crts/debug.h"
#include "crts/Module.hpp"


// All this module does is share this global variable loadCount
// between other modules.
uint64_t st_loadCount = 0;
pthread_mutex_t st_mutex = PTHREAD_MUTEX_INITIALIZER;


class SleepTest : public CRTSModule
{
    public:

        SleepTest(int argc, const char **argv);
        ~SleepTest(void);
};

SleepTest::SleepTest(int argc, const char **argv) { DSPEW(); }

SleepTest::~SleepTest(void) { DSPEW(); }


// Define the module loader stuff to make one of these class objects.
CRTSMODULE_MAKE_MODULE(SleepTest)
