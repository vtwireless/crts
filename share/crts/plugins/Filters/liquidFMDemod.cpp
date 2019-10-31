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
        unsigned char *outputBuffer;
        const size_t outBufferLen;
        //bytes out at each write()
        size_t len_out;
    };



    LiquidFMDemod::LiquidFMDemod(int argc, const char **argv):
    fdem(0),
    kf(0.1f),
    outBufferLen(2*1024), len_out(0)
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
LiquidFMDemod::input(void *buffer, size_t len, uint32_t channelNum)
{
    DASSERT(buffer, "");
    DASSERT(len, "");
    //printf("%zu\n", len );

    // buf is a buffer that we can index a byte at a time.
    //
    // g++ will not let me increment buffer, even with casting, so we use
    // buf as a byte pointer that points to buffer.  I do not understand
    // why that is we have to declare another variable, when type casing
    // could do it.
    uint8_t *buf = (uint8_t *) buffer;

    while(len) {

        size_t outLen = len;
        if(outLen > maxBufferLen)
            outLen = maxBufferLen;
        
        // Move as much as we can of the input data to the output buffer.
        memcpy(getOutputBuffer(outputChannel[inChannelNum]), buf, outLen);

        // Push input to output; in this case without modification, but we
        // could do something like multiply by a constant.  Then again we did
        // not type this data, so how could we change it.

        // This writes to the output buffer.  It knows that it must copy from
        // the current input buffer to outputChannel[i], because this is the
        // source of outputChannel[inChannelNum] via createOutputBuffer() in
        // start().
        //
        // Call the next filter input:
        output(outLen, outputChannel[inChannelNum]);

        // keep byte count of the output.
        len -= outLen;

        // Go to the next chuck of the input buffer.  This is why we
        // declared buf to walk along buffer.
        // ((uint8_t *)(buffer)) += outLen;  will not compile.
        buf += outLen;
    }

    
    len_out = 0;
    outputBuffer = (unsigned char *) getOutputBuffer(0);
    advanceInput(len-len%sizeof(std::complex<float>));
    freqdem_demodulate(fdem, s, &y);
    if(len_out == 0) return; // nothing to output.


    // TODO: figure out this length.  Not just guessing.
    //
    ASSERT(len_out <= outBufferLen, "");

    output(len_out, CRTSFilter::ALL_CHANNELS);

}


LiquidFMDemod::~LiquidFMDemod(void)
{
    DSPEW();
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(LiquidFMDemod)
