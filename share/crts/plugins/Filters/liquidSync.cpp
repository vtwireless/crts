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
        ssize_t write(void *buffer, size_t bufferLen,
                uint32_t channelNum);

    private:

        unsigned int numSubcarriers;
        unsigned int cp_len;
        unsigned int taper_len;
        unsigned char *subcarrierAlloc;
        ofdmflexframesync fs;

        // frameSyncCallback() needs to be able to call
        // CRTSFilter::writePush().
    friend int frameSyncCallback(unsigned char *header, int header_valid,
                unsigned char *payload, unsigned int payload_len,
                int payload_valid, framesyncstats_s stats,
                LiquidSync *liquidSync);
};



// TODO: This does not appear to be able to be declared as static because
// is needs to be a friend of LiquidSync.  That's not so bad because when
// this module is loaded the symbols are not exported and are kept hidden
// from the rest of the crts_radio program.
//
int frameSyncCallback(unsigned char *header, int header_valid,
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

        // TODO: The interface to liquid DSP sucks.  We cannot use
        // our own buffer for it to write to, so we must copy their
        // buffer to our CRTSFilter buffer.  This is very very bad.
        // If we don't use our CRTSFilter buffers then we lose a ability
        // to have "seamless threading", which defeats the whole point
        // of this software project.
        //
        // A possible fix would be to catch their malloc() (or like call)
        // and override it with our own malloc() call.  We'd have to be
        // careful and make sure that the passed buffer pointer is in the
        // correct place in memory for this to work.
        //

        unsigned char *buffer = (unsigned char *)
            liquidSync->getBuffer(payload_len);

        // TODO: This is bad coding practice. See comment above.
        memcpy(buffer, payload, payload_len);

        liquidSync->writePush(buffer, payload_len, CRTSFilter::ALL_CHANNELS);
    }

    return 0;
}


LiquidSync::LiquidSync(int argc, const char **argv):
    numSubcarriers(32), cp_len(16), taper_len(4),
    subcarrierAlloc(0), fs(0)
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


ssize_t LiquidSync::write(void *buffer, size_t len, uint32_t channelNum)
{
    DASSERT(buffer, "");
    DASSERT(len, "");

    ofdmflexframesync_execute(fs, (std::complex<float> *) buffer,
            len/sizeof(std::complex<float>));

    return len;
}


LiquidSync::~LiquidSync(void)
{
    if(fs)
        ofdmflexframesync_destroy(fs);

    if(subcarrierAlloc)
        free(subcarrierAlloc);
    DSPEW();
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(LiquidSync)
