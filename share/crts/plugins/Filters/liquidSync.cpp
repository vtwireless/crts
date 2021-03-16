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
        void input(void *buffer, size_t len, uint32_t inChannelNum);

    private:

        const unsigned int numSubcarriers;
        const unsigned int cp_len;
        const unsigned int taper_len;
        unsigned char *subcarrierAlloc;
        ofdmflexframesync fs;


    public:

        double getTotalBytesOut(void) {
            return totalBytesOut;
        }

        // We need to access these in frameSyncCallback()
        //
        unsigned char *outputBuffer;
        const size_t outBufferLen;
        
        // bytes out at each write().
        size_t len_out;
        double totalBytesOut;
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
    if(payload_valid)
    {
        // TODO: remove this print and add an error checker!!!!!!!!
        //DSPEW("recieved header sequence=%" PRIu64 " %zu bytes" ,
        //     *((uint64_t *) header), payload_len);

        DASSERT(payload_len > 0, "");


        // TODO: The interface to liquid DSP is lacking.  We cannot use
        // our own buffer for it to write to, so we must copy their buffer
        // to our CRTSFilter buffer.
        //

        // TODO: This is bad coding practice. See comment above.
        memcpy(liquidSync->outputBuffer, payload, payload_len);

        // Go to forward in the output buffer.
        liquidSync->outputBuffer += payload_len;
        liquidSync->len_out += payload_len;
        liquidSync->totalBytesOut += payload_len;

        // Do not overrun the output buffer.
        ASSERT(liquidSync->len_out <= liquidSync->outBufferLen, "");
    }

    return 0;
}


void LiquidSync::input(void *buffer, size_t len, uint32_t channelNum)
{
    DASSERT(buffer, "");
    DASSERT(len, "");

    len_out = 0;

    outputBuffer = (unsigned char *) getOutputBuffer(0);

    // We will eat all the input data except the remainder of
    // sizeof(std::complex<float>).  We cannot use half of a
    // complex<float>.  If the data is not aligned at complex<float> edges
    // we are screwed.
    //
    advanceInput(len - len % sizeof(std::complex<float>));


    ofdmflexframesync_execute(fs, (std::complex<float> *) buffer,
            len/sizeof(std::complex<float>));


    if(len_out == 0) return; // nothing to output.


    // TODO: figure out this length.  Not just guessing.
    //
    ASSERT(len_out <= outBufferLen, "");

    output(len_out, CRTSFilter::ALL_CHANNELS);
    

    totalBytesOut += len_out;
    setParameter("totalBytesOut", totalBytesOut);
}


LiquidSync::LiquidSync(int argc, const char **argv):
    numSubcarriers(200), cp_len(16), taper_len(4),
    subcarrierAlloc(0), fs(0),
    outputBuffer(0), outBufferLen(2*1024), len_out(0)
{
    subcarrierAlloc = (unsigned char *) malloc(numSubcarriers);
    if(!subcarrierAlloc) throw "malloc() failed";
    ofdmframe_init_default_sctype(numSubcarriers, subcarrierAlloc);

    fs = ofdmflexframesync_create(numSubcarriers, cp_len,
                taper_len, subcarrierAlloc,
                (framesync_callback) frameSyncCallback,
                this/*callback data*/);

    addParameter("totalBytesOut",
                [&]() { return getTotalBytesOut(); },
                0/* no set()*/
    );

    DSPEW();
}


bool LiquidSync::start(uint32_t numInChannels, uint32_t numOutChannels)
{
    totalBytesOut = 0;

    if(numInChannels != 1)
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

    setParameter("totalBytesOut", totalBytesOut);

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
