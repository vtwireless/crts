#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "crts/debug.h"
#include "crts/Filter.hpp"


class SleepTest : public CRTSFilter
{
    public:

        SleepTest(int argc, const char **argv);
        ~SleepTest(void);
        ssize_t write(void *buffer, size_t bufferLen,
                uint32_t channelNum);

    private:

        struct timespec t;
};


// From ../Modules/sleepTest.cpp
extern long st_loadCount;
extern pthread_mutex_t st_mutex;


SleepTest::SleepTest(int argc, const char **argv)
{
    t.tv_sec = 0;
    t.tv_nsec = 0;

    struct drand48_data rbuffer;
    ASSERT((errno = pthread_mutex_lock(&st_mutex)) == 0, "");
    // This is this objects unique seed.
    uint64_t seedVal = ++st_loadCount;
    ASSERT((errno = pthread_mutex_unlock(&st_mutex)) == 0, "");

    memset(&rbuffer, 0, sizeof(rbuffer));
    srand48_r(seedVal, &rbuffer);
    lrand48_r(&rbuffer, &t.tv_nsec);
    // sleep time between 0.001 to 0.003 seconds
    t.tv_nsec = t.tv_nsec % 2000000 + 1000000;

    DSPEW("sleep interval = %g seconds", t.tv_nsec/(1.0e9));
}

ssize_t SleepTest::write(void *buffer, size_t len, uint32_t channelNum)
{
    DASSERT(buffer, "");
    DASSERT(len, "");

    nanosleep(&t,0);

    // Send this buffer to the next readers write call
    // on all channels.
    writePush(buffer, len, CRTSFilter::ALL_CHANNELS);

    return len;
}


SleepTest::~SleepTest(void)
{
    DSPEW();
}
 

// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(SleepTest)
