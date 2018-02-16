#include <stdio.h>
#include <pthread.h>
#include "liquid.h"
#include "crts/debug.h"


void *tcall(void *ptr)
{
    SPEW();

    return (void *) -1;
}


int main(int argc, char **argv)
{
    printf("hello\n");

    SPEW();

    pthread_t t;
    void *ret;
    errno = 44;

    ASSERT(pthread_create(&t, 0, tcall, 0) == 0, "");

    WARN();

    ASSERT((errno = pthread_join(t, &ret)) == 0, "");

    printf("thread returned %p\n", ret);

    FAIL("Calling FAIL() %d %d %d,  This is just a test ...", 1, 2, 3);

    return 0;
}
