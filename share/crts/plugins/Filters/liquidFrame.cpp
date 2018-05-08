#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <complex>

#include "liquid.h"

#include "crts/debug.h"
#include "crts/Filter.hpp"
#include "crts/crts.hpp"

#define URL "http://liquidsdr.org/doc/ofdmflexframe/"

static void usage(void)
{
    char nameBuf[64], *name;
    name = CRTS_BASENAME(nameBuf, 64);

    fprintf(stderr,
"\n"
"\n"
"  Usage: %s\n"
"\n"
"      Read 1 channel in and write 1 channel out.  The output with be from the\n"
"   Liquid DSP frame generator, ofdmflexframegen, " URL "\n"
"\n"
"\n",
    name);

    errno = 0;
    throw "usage help"; // This is how return an error from a C++ constructor
    // the module loader will catch this throw.
}


class LiquidFrame : public CRTSFilter
{
    public:

        LiquidFrame(int argc, const char **argv);
        ~LiquidFrame(void);
        bool start(uint32_t numInChannels, uint32_t numOutChannels);
        bool stop(uint32_t numInChannels, uint32_t numOutChannels);
        void input(void *buffer, size_t len, uint32_t inChannelNum);

    private:

        ofdmflexframegen fg;
        const unsigned int numSubcarriers;
        const unsigned int cp_len;
        const unsigned int taper_len;
        unsigned char *subcarrierAlloc;

        size_t numPadComplexFloat;

        uint64_t frameCount;
        const float softGain;

        size_t complexArrayLen; // number of complex elements per output()
        bool padFrames;
};



LiquidFrame::LiquidFrame(int argc, const char **argv):
    fg(0),
    numSubcarriers(32), cp_len(16), taper_len(4), subcarrierAlloc(0),
    numPadComplexFloat(4),
    frameCount(0), softGain(powf(10.0F, -12.0F/20.0F)),
    padFrames(true)
{
    CRTSModuleOptions opt(argc, argv, usage);

    DSPEW();
}

bool LiquidFrame::start(uint32_t numInChannels, uint32_t numOutChannels)
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

    subcarrierAlloc = (unsigned char *) malloc(numSubcarriers);
    if(!subcarrierAlloc) throw "malloc() failed";
    // TODO: check for error here:
    ofdmframe_init_default_sctype(numSubcarriers, subcarrierAlloc);

    /////////////////////////////////////////////////////////////////
    // TODO: maybe these values can be tuned for better performance.
    /////////////////////////////////////////////////////////////////
    complexArrayLen = numSubcarriers + cp_len + numPadComplexFloat;
    /////////////////////////////////////////////////////////////////


    ofdmflexframegenprops_s fgprops; // frame generator properties
    ofdmflexframegenprops_init_default(&fgprops);
    fgprops.check = LIQUID_CRC_32;
    fgprops.fec0 = LIQUID_FEC_HAMMING128;
    fgprops.fec1 = LIQUID_FEC_NONE;
    fgprops.mod_scheme = LIQUID_MODEM_QAM4;

    fg = ofdmflexframegen_create(numSubcarriers, cp_len,
                taper_len, subcarrierAlloc, &fgprops);

    // We use the same ring buffer for all output channels
    // however many there are.
    //
    createOutputBuffer(complexArrayLen*sizeof(std::complex<float>),
            (uint32_t) 0);

    return false; // success
}


bool LiquidFrame::stop(uint32_t numInChannels, uint32_t numOutChannels)
{
    DSPEW();

    if(fg)
    {
        ofdmflexframegen_destroy(fg);
        fg = 0;
    }

    if(subcarrierAlloc)
    {
        free(subcarrierAlloc);
        subcarrierAlloc = 0;
    }

    return false; // success
}



void
LiquidFrame::input(void *buffer, size_t len, uint32_t inputChannelNum)
{
    DASSERT(buffer, "");
    DASSERT(len, "");

    ++frameCount;

#if 0
    // outBuffer is a pointer to the Ring Buffer where we last
    // wrote to:
    //
    std::complex<float> *outBuffer =
        (std::complex<float> *) getOutputBuffer(0);

    // assemble frame using liquid-dsp
    // Enter the raw data that we just read:
    ofdmflexframegen_assemble( // in buffer with len in bytes.
            fg, (unsigned char *) &frameCount,
            (const unsigned char *) buffer, len);

    bool last_symbol = false;

    size_t n = 0; // number of complex values in loop
    while(n < complexArrayLen)
    {
        // generate symbol
        last_symbol = ofdmflexframegen_write(fg,
                    &outBuffer[n], fgLen);
        n += fgLen;
        if(last_symbol) break;
    }
    
    // Now we have n complex values in outBuffer

    // Apply the gain
    for(size_t i=0; i<n; ++i)
        outBuffer[i] *= softGain;

    if(padFrames && last_symbol && n < complexArrayLen)
    {
        // We are able to fit the frame pad in with this output()
        // call.
        //
        // TODO: Should this be zeros, or does it matter what it is.
        memset(&outBuffer[n], 0, fgLen*sizeof(std::complex<float>));
        n += fgLen;
    }

    output(n*sizeof(std::complex<float>), ALL_CHANNELS);
#endif
}


LiquidFrame::~LiquidFrame(void)
{
    DSPEW();
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(LiquidFrame)
