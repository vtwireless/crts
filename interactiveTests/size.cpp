#include <stdio.h>
#include <complex>
#include <vector>
#include <string.h>



int main(void)
{
    std::complex<float> f[10];
    std::vector<std::complex<float>> buffer(10);

    

    memset(f, 0, sizeof(f));

    printf("sizeof(f) = %zu buffer.size()=%zu\n", sizeof(f), buffer.size());

    return 0;
}
