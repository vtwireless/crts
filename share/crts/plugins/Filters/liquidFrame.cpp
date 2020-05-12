#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <complex>

#include "liquid.h"

#include "crts/debug.h"
#include "crts/Filter.hpp"
#include "crts/crts.hpp"

#define URL "http://liquidsdr.org/doc/ofdmflexframe/"

// In this case gain is 10
//
#define SOFT_GAIN_FACTOR  (powf(10.0F, -12.0F/20.0F))



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
        size_t payloadLength;
        size_t complexArrayLen; // number of complex elements per output()

        uint64_t frameCount;
        const float softGain;
        const size_t outChunk;

        void _output(size_t numComplex, std::complex<float> *x);
};



LiquidFrame::LiquidFrame(int argc, const char **argv):
    fg(0),
    numSubcarriers(200), cp_len(16), taper_len(4), subcarrierAlloc(0),
    numPadComplexFloat(0), payloadLength(1024),
    frameCount(0), softGain(SOFT_GAIN_FACTOR),
    outChunk(512)
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

    // We wish to take an encode all the data that we receive in a given
    // input call, but we can't know ahead of time how large the output
    // will be, even when we know how large the input is.

    // The size of one liquid DSP symbol that is outputted is:
    // (numSubcarriers + cp_len) * sizeof(std::complex<float>)


#define PACK_FACTOR  (5.0)

    size_t numSymbols = (PACK_FACTOR * payloadLength) /
        ((double) ((numSubcarriers + cp_len)*sizeof(std::complex<float>)))
        + 1;

    complexArrayLen = numSymbols * (numSubcarriers + cp_len) +
        numPadComplexFloat;

    DASSERT(complexArrayLen >= numPadComplexFloat, "");
    DASSERT(complexArrayLen >= outChunk,
            "complexArrayLen=%zu < %zu=outChunk",
            complexArrayLen, outChunk);


    /////////////////////////////////////////////////////////////////


    ofdmflexframegenprops_s fgprops; // frame generator properties
    ofdmflexframegenprops_init_default(&fgprops);
    fgprops.check = LIQUID_CRC_32;
    fgprops.fec1 = LIQUID_FEC_HAMMING128;
    fgprops.fec0 = LIQUID_FEC_NONE;
    fgprops.mod_scheme = LIQUID_MODEM_QAM4;

    fg = ofdmflexframegen_create(numSubcarriers, cp_len,
                taper_len, subcarrierAlloc, &fgprops);

    // Setup the things associated with CRTS and this filter and
    // how it gets data from the CRTS filter stream.
    //
    createOutputBuffer(complexArrayLen*sizeof(std::complex<float>),
            (uint32_t) 0);

    // We make it so that unless the stream is shutting down there will be
    // a fixed input() size in of payloadLength bytes.  But at shutdown
    // there may be less input.
    //
    setInputThreshold(payloadLength, 0/*output channel*/);
    setMaxUnreadLength(payloadLength, 0/*output channel*/);
    setChokeLength(payloadLength, 0/*output channel*/);

    DSPEW("per output numSymbols=%zu  complexArrayLen=%zu outChunk=%zu",
            numSymbols, complexArrayLen, outChunk);

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


void LiquidFrame::_output(size_t numComplex, std::complex<float> *x)
{
    DASSERT(numComplex, "");

    for(size_t i=0; i<numComplex; ++i)
        x[i] *= softGain;

    output(numComplex*sizeof(std::complex<float>), 0);
}


void
LiquidFrame::input(void *inBuffer, size_t inLen, uint32_t inputChannelNum)
{
    DASSERT(inBuffer, "");
    DASSERT(inLen, "");

#ifdef DEBUG
    // If the CRTS code works as documented this is what should happen:
    //
    if(stream->isRunning)
        // When it is running in a regular fashion this is
        // what we requested in start().
        DASSERT(inLen == payloadLength, "");
    else
        // In shutdown mode we could get less than the requested data and
        // maybe even more than once, depending on how other filters feed
        // this filter.
        DASSERT(inLen <= payloadLength, "");
#endif

    ++frameCount;


    // Get a pointer to memory in the ring buffer which we will
    // write to using the liquid DSP API (Application Programming
    // Interface).
    //
    std::complex<float> *outBuffer =
        (std::complex<float> *) getOutputBuffer(0);


    // Assemble frames using liquid-dsp.
    // Enter the raw data that we just read:
    ofdmflexframegen_assemble(
            fg, (unsigned char *) &frameCount,
            (const unsigned char *) inBuffer, inLen);

    bool last_symbol = false;
    size_t numComplexOut = 0;

    DASSERT(complexArrayLen >= numComplexOut + outChunk, "");

    // In the below, we call output() as many times as we need to in order
    // to frame all the input data and send it with output() and
    // optionally we send some 0 padding after the framed data.

    while(!last_symbol)
    {
        if(complexArrayLen >= numComplexOut + outChunk)
        {
            // Generate symbols for output.  I do not know why, but it must
            // be done in chunks and the chunk size seems arbitrary too.
            // Ya WTF.
            //
            last_symbol = ofdmflexframegen_write(fg,
                    &outBuffer[numComplexOut], outChunk);
            numComplexOut += outChunk;
        }
        else
        {
            DASSERT(numComplexOut, "output Buffer is not large enough");

            // Output what we have so far.  output() will do what it needs
            // to call other filter input() functions.
            //
            _output(numComplexOut, outBuffer);

            // reset.
            outBuffer = 0;
            numComplexOut = 0;

            if(!last_symbol || numPadComplexFloat)
            {
                // We have more to write out. So
                //
                // go to the next chunk in the ring buffer
                outBuffer = (std::complex<float> *) getOutputBuffer(0);
            }
        }
    }

    if(numPadComplexFloat)
    {
        // This block is to send the optional padding.

        DASSERT(outBuffer, "");

        if(complexArrayLen < numComplexOut + numPadComplexFloat)
        {
            // We cannot fix this padding in the current out buffer.
            // So we'll flush it and send a new batch.
            //
            DASSERT(numComplexOut, "");
                
            // Output what we have so far.  output() will do what it
            // needs to to call other filter input() functions.
            //
            _output(numComplexOut, outBuffer);
            numComplexOut = 0;
        }

        memset(&outBuffer[numComplexOut], 0,
                numPadComplexFloat * sizeof(std::complex<float>));
        numComplexOut += numPadComplexFloat;
    }

    if(numComplexOut)
        _output(numComplexOut, outBuffer);
}


LiquidFrame::~LiquidFrame(void)
{
    DSPEW();
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(LiquidFrame)
