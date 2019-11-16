#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <complex>

#include "liquid.h"

#include "crts/debug.h"
#include "crts/Filter.hpp"
#include "crts/crts.hpp"

#define URL "http://liquidsdr.org/doc/freqmodem/"

class LiquidFMDemod : public CRTSFilter
{
    public:
        LiquidFMDemod(int argc, const char **argv);
        ~LiquidFMDemod(void);
        bool start(uint32_t numInChannels, uint32_t numOutChannels);
        bool stop(uint32_t numInChannels, uint32_t numOutChannels);
        void input(void *buffer, size_t len, uint32_t inChannelNum);

    private:
        freqdem fdem;
        float kf;
        std::complex<float> s;      //input FM signal
        float y;                    //output demodulated bytes

    public:
        std::complex<float> *outputBuffer;
        const size_t outBufferLen;
        //bytes out at each write()
        size_t len_out;
};


LiquidFMDemod::LiquidFMDemod(int argc, const char **argv):
    fdem(0),
    kf(0.1f),
    outputBuffer(0), outBufferLen(2*1024), len_out(0)
{
    fdem = freqdem_create(kf);
    DSPEW();
}

bool LiquidFMDemod::start(uint32_t numInChannels, uint32_t numOutChannels)
{
    DSPEW();

    if(isSource())
    {
        WARN("This filter cannot be a source filter.");
        return true; // fail
    }
    if(numInChannels != 1)
    {
        WARN("Should have 1 input channel, got %" PRIu32,
                numInChannels);
        return true; // fail
    }
    if(numOutChannels != 1)
    {
        WARN("Should have 1 output channel, got %" PRIu32,
            numOutChannels);
        return true; // fail
    }
    fdem = freqdem_create(kf);
    createOutputBuffer(outBufferLen);
    return false; // success
}


bool LiquidFMDemod::stop(uint32_t numInChannels, uint32_t numOutChannels)
{
    DSPEW();
    if(fdem)
    {
        freqdem_destroy(fdem);
        fdem = 0;
    }
    return false; // success
}


void
LiquidFMDemod::input(void *inBuffer, size_t inLen, uint32_t channelNum)
{
    DASSERT(inBuffer, "");
    DASSERT(inLen, "");
    //printf("%zu\n", len );
    len_out = 0;

    outputBuffer = (std::complex<float> *) getOutputBuffer(0);
    advanceInput(inLen-inLen%sizeof(std::complex<float>));
    freqdem_demodulate(fdem, *outputBuffer, &y);
    /*if(len_out == 0)
    {
        INFO("nothing to output");
        return; // nothing to output.
    }
    */
    // TODO: figure out this length.  Not just guessing.
    
    INFO("%lf", y);
    ASSERT(len_out <= outBufferLen, "");

    output(y, CRTSFilter::ALL_CHANNELS);

}


LiquidFMDemod::~LiquidFMDemod(void)
{
    DSPEW();
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(LiquidFMDemod)

