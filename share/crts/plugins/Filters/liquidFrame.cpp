#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <complex>

#include "liquid.h"

#include "crts/debug.h"
#include "crts/Filter.hpp"


class LiquidFrame : public CRTSFilter
{
    public:

        LiquidFrame(int argc, const char **argv);
        ~LiquidFrame(void);
        ssize_t write(void *buffer, size_t bufferLen,
                uint32_t channelNum);

    private:

        unsigned int numSubcarriers;
        unsigned int cp_len;
        unsigned int taper_len;
        unsigned char *subcarrierAlloc;
        ofdmflexframegen fg;
        uint64_t frameCount;
        float softGain;

        size_t fgLen; // For liquid ofdmflexframegen_write()
        size_t arrayLen; // max number of complex elements per writePush()
        size_t outBufferLen; // max bytes of data written per writePush()
        bool padFrames; // flag to send another fgLen complex at end
};



LiquidFrame::LiquidFrame(int argc, const char **argv):
    numSubcarriers(32), cp_len(16), taper_len(4),
    subcarrierAlloc(0), fg(0), frameCount(0),
    softGain(powf(10.0F, -12.0F/20.0F)),
    padFrames(true)
{
    subcarrierAlloc = (unsigned char *) malloc(numSubcarriers);
    if(!subcarrierAlloc) throw "malloc() failed";
    ofdmframe_init_default_sctype(numSubcarriers, subcarrierAlloc);

    /////////////////////////////////////////////////////////////////
    // TODO: maybe these values can be tuned for better performance.
    /////////////////////////////////////////////////////////////////
    fgLen = numSubcarriers + cp_len; // per ofdmflexframegen_write()
    arrayLen = 20*fgLen; // we'll use (fgLen * arrayLen)
    outBufferLen = arrayLen*sizeof(std::complex<float>);
    // padFrames = ???
    /////////////////////////////////////////////////////////////////


    ofdmflexframegenprops_s fgprops; // frame generator properties
    ofdmflexframegenprops_init_default(&fgprops);
    fgprops.check = LIQUID_CRC_32;
    fgprops.fec0 = LIQUID_FEC_HAMMING128;
    fgprops.fec1 = LIQUID_FEC_NONE;
    fgprops.mod_scheme = LIQUID_MODEM_QAM4;

    fg = ofdmflexframegen_create(numSubcarriers, cp_len,
                taper_len, subcarrierAlloc, &fgprops);
    DSPEW();
}


ssize_t LiquidFrame::write(void *buffer, size_t len, uint32_t channelNum)
{
    DASSERT(buffer, "");
    DASSERT(len, "");

    ++frameCount;

    std::complex<float> *fg_buffer = (std::complex<float> *)
        getBuffer(outBufferLen);

    // assemble frame using liquid-dsp
    // Enter the raw data that we just read:
    ofdmflexframegen_assemble( // in buffer with len in bytes.
            fg, (unsigned char *) &frameCount,
            (const unsigned char *) buffer, len);

    //
    // TODO: count values ??
    //
    //size_t numValues = 0; // number of complex values total
    // TODO: With padding or not???
    //

    bool last_symbol = false;
    bool needPad = padFrames;

    while (!last_symbol)
    {
        size_t n = 0; // number of complex values in loop
        size_t i;
        while(n < arrayLen)
        {
            // generate symbol
            last_symbol = ofdmflexframegen_write(fg,
                    &fg_buffer[n], fgLen);
            n += fgLen;
            if(last_symbol) break;
        }
        // Now we have n complex values in fg_buffer

        // Apply the gain
        for(i=0; i<n; ++i)
            fg_buffer[i] *= softGain;

        //numValues += n; // tally number of complex values

        if(needPad && last_symbol && n < arrayLen)
        {
            // We are able to fit the frame pad in with this writePush()
            // call.
            //
            // TODO: Should this be zeros, or does it matter what it is.
            memset(&fg_buffer[n], 0, fgLen*sizeof(std::complex<float>));
            n += fgLen;
            needPad = false; // flag got the padding in this writePush().
        }

        writePush(fg_buffer, n*sizeof(std::complex<float>),
                CRTSFilter::ALL_CHANNELS);
    }


    if(needPad)
    {
        // We were not able to fit the frame pad in with the above
        // writePush() calls, so we write it here.
        //
        // TODO: Should this be zeros, or does it matter what it is
        // so long as it's not NAN or Infinity
        //
        memset(fg_buffer, 0, fgLen*sizeof(std::complex<float>));

        writePush(fg_buffer, fgLen*sizeof(std::complex<float>),
                CRTSFilter::ALL_CHANNELS);
    }

    return 1; // Whatever that means.
}


LiquidFrame::~LiquidFrame(void)
{
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
    DSPEW();
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(LiquidFrame)
