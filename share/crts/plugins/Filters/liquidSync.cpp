#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <complex>

#include "liquid.h"

#include "crts/debug.h"
#include "crts/Filter.hpp"


class LiquidSync : public CRTSFilter
{
    public:

        LiquidSync(int argc, const char **argv);
        ~LiquidSync(void);
        bool start(uint32_t numInChannels, uint32_t numOutChannels);
        bool stop(uint32_t numInChannels, uint32_t numOutChannels);
        void write(void *buffer, size_t len, uint32_t inChannelNum);

    private:

        const unsigned int numSubcarriers;
        const unsigned int cp_len;
        const unsigned int taper_len;
        unsigned char *subcarrierAlloc;
        ofdmflexframesync fs;

        const size_t outBufferLen;

    public:

        unsigned char *outputBuffer;
 
        // bytes out at each write().
        size_t len_out;
};



// TODO: This does not appear to be able to be declared as static because
// is needs to be a friend of LiquidSync.  That's not so bad because when
// this module is loaded the symbols are not exported and are kept hidden
// from the rest of the crts_radio program.
//
static int 
frameSyncCallback(unsigned char *header, int header_valid,
               unsigned char *payload, unsigned int payload_len,
               int payload_valid, framesyncstats_s stats,
               LiquidSync *liquidSync) 
{
#if 0
    DSPEW("payload_valid=%d  header[0]=%d  len=%u", payload_valid,
            header[0], payload_len);
#endif
    if(header_valid)
    {
        // TODO: remove this print and add an error checker!!!!!!!!
        DSPEW("recieved header sequence=%" PRIu64 , *((uint64_t *) header));

        DASSERT(payload_len > 0, "");


        // TODO: The interface to liquid DSP is lacking.  We cannot use
        // our own buffer for it to write to, so we must copy their buffer
        // to our CRTSFilter buffer.
        //

        // TODO: This is bad coding practice. See comment above.
        memcpy(liquidSync->outputBuffer, payload, payload_len);

        liquidSync->outputBuffer += payload_len;
        liquidSync->len_out += payload_len;
    }

    return 0;
}


void LiquidSync::write(void *buffer, size_t len, uint32_t channelNum)
{
    DASSERT(buffer, "");
    DASSERT(len, "");

    DSPEW();

    len_out = 0;

    outputBuffer = (unsigned char *) getOutputBuffer(0);

    // We will eat all the input data.
    //
    advanceInputBuffer(len - len % sizeof(std::complex<float>));
    DSPEW();


    ofdmflexframesync_execute(fs, (std::complex<float> *) buffer,
            len/sizeof(std::complex<float>));\
    DSPEW();


    // TODO: figure out this length.  Not just guessing.
    //
    ASSERT(len_out <= outBufferLen, "");

    writePush(len_out, CRTSFilter::ALL_CHANNELS);
}


LiquidSync::LiquidSync(int argc, const char **argv):
    numSubcarriers(32), cp_len(16), taper_len(4),
    subcarrierAlloc(0), fs(0), outBufferLen(1024),
    outputBuffer(0), len_out(0)
{
    subcarrierAlloc = (unsigned char *) malloc(numSubcarriers);
    if(!subcarrierAlloc) throw "malloc() failed";
    ofdmframe_init_default_sctype(numSubcarriers, subcarrierAlloc);

    fs = ofdmflexframesync_create(numSubcarriers, cp_len,
                taper_len, subcarrierAlloc,
                (framesync_callback) frameSyncCallback,
                this/*callback data*/);

    DSPEW();
}


bool LiquidSync::start(uint32_t numInChannels, uint32_t numOutChannels)
{
    if(numInChannels < 1)
    {
        WARN("Should have 1 input channel got %" PRIu32, numInChannels);
        return true; // fail
    }

    if(numOutChannels < 1)
    {
        WARN("Should have 1 or more output channels got %" PRIu32,
            numOutChannels);
        return true; // fail
    }

    subcarrierAlloc = (unsigned char *) malloc(numSubcarriers);
    if(!subcarrierAlloc) throw "malloc() failed";
    ofdmframe_init_default_sctype(numSubcarriers, subcarrierAlloc);

    fs = ofdmflexframesync_create(numSubcarriers, cp_len,
                taper_len, subcarrierAlloc,
                (framesync_callback) frameSyncCallback,
                this/*callback data*/);

    // We use the same ring buffer for all output channels
    //
    createOutputBuffer(outBufferLen);

    return false; // success
}


bool LiquidSync::stop(uint32_t numInChannels, uint32_t numOutChannels)
{
    DSPEW();

    if(fs)
        ofdmflexframesync_destroy(fs);

    if(subcarrierAlloc)
        free(subcarrierAlloc);

    return false; // success
}


LiquidSync::~LiquidSync(void)
{
    DSPEW();
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(LiquidSync)
