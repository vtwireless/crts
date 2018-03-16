#include <complex>
#include <values.h> // INT_MAX

#include <crts/debug.h>
#include <crts/Filter.hpp>
#include <crts/crts.hpp> // for:  FILE *crtsOut



class Int2Float : public CRTSFilter
{
    public:

        Int2Float(int argc, const char **argv)
        { 
            CRTSModuleOptions opt(argc, argv);
            scale = opt.get("--scale", 1.0e-2);
            DSPEW("--scale %g", scale);
        };

        ~Int2Float(void) { DSPEW(); };

        ssize_t write(void *buffer, size_t bufferLen, uint32_t channelNum);

    private:

        float scale;
};


// len is the number of bytes not complex floats.
ssize_t Int2Float::write(void *buffer, size_t len, uint32_t channelNum)
{
    DASSERT(len, "");
    DASSERT(buffer, "");

    // Allocate a complex for every 2 integers.

    // TODO: We could reuse the buffer as the output too, but than we'd
    // have less values produced per values input.  Hard to say what way
    // would be better.  It may depend on the use case.

    size_t n = len/(2*sizeof(int)); // number of pairs of int

    std::complex<float> *out = (std::complex<float> *) getBuffer(n*sizeof(std::complex<float>));
    int *i = (int *) buffer;

    // Reuse variables buffer and len.
    buffer = out;
    len = n*sizeof(std::complex<float>);

    while(n--)
    {
        out->real(scale * (*(i++))/((float) INT_MAX));
        out->imag(scale * (*(i++))/((float) INT_MAX));
        ++out; // go to the next one.
    }

    writePush(buffer, len, CRTSFilter::ALL_CHANNELS);

    return 1;
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(Int2Float)
