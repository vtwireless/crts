#include <stdio.h>
#include <inttypes.h>

#include "crts/debug.h"
#include "makeRingBuffer.hpp"


int main(void)
{
    float *x, *y;
    size_t len = sizeof(float) + 4096 + 2342;
    size_t overhang = 2*sizeof(float);

    x = (float *) makeRingBuffer(len, overhang);

    printf("length = %zu bytes  giving ="
            "%zu floats  plus %zu extra bytes\n",
            len, len/sizeof(float), len % sizeof(float));
    printf("overhang = %zu bytes\n\n", overhang);

    x[0] = 2332.09F;
    // y is a pointer to a different address that has the
    // same value that x points to
    y = (float *) ((uint8_t*) x + len);
    y[1] = 3.4e6F;

    printf(" x[0] = *(%p) = %f\n", x, x[0]);
    printf(" y[0] = *(%p) = %f\n\n", y, y[0]);

    printf(" x[1] = *(%p) = %f\n", x+1, x[1]);
    printf(" y[1] = *(%p) = %f\n", y+1, y[1]);

    ASSERT(x[0] == y[0] && x[1] == y[1], "We failed");

    printf("\n  %f == %f   %f == %f \n", x[0], y[0], x[1], y[1]);

    freeRingBuffer(x, len, overhang);

    printf("\n  SUCCESS\n");

    return 0;
}
