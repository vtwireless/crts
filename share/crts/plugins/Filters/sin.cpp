#include <stdio.h>
#include <inttypes.h>
#include <math.h>

#include "crts/debug.h"
#include "crts/Filter.hpp"


class Sin : public CRTSFilter
{
    public:

        Sin(int argc, const char **argv);
        ~Sin(void);

        ssize_t write(void *buffer, size_t bufferLen,
                uint32_t channelNum);

    private:

        // TODO: We don't bother syncing this "signal" to real-time (wall
        // clock time).  For now we just need this for testing, some kind
        // of signal that is semi-regular.  We'll be using low sample
        // rates and low frequencies.  We assume that it takes no time to
        // compute the values, and so all the rates will be off and we
        // don't care.  We just need something that changes somewhat
        // regularly so we have a regular changing source of floating
        // point data, with parameters that can be changed.

        long samplePeriod/*nano seconds*/;

        size_t samplesPerWrite;

        // Since we are changing parameters, if we wish to keep continuity
        // to second order in the output we keep X and it's derivative V,
        // and recompute the initialPhase and amplitude in order to keep X
        // and V continuous, any time a parameter changes.
        //
        // TODO: Using ODEs (ordinary differential equations) and a solver
        // may make it easier to keep continuity when parameters change.
        // Then nothing would need to be calculated just change the
        // parameters of the ODEs and continuity (C-2) is automatically
        // kept in X and V. 

        float X, V, amplitude, angularFreq, initialPhase,
              sampleRate/*Hz*/, t/*time in seconds*/;

        // TODO: add limits to the parameters that can be changed.
};


Sin::Sin(int argc, const char **argv):
    samplesPerWrite(100), amplitude(1.0F), angularFreq(2.0F*M_PI/0.5F),
    initialPhase(0), sampleRate(100.0F), t(0)
{
    DSPEW();

    // TODO: Add options to initialize parameters.
    
    float phase = angularFreq * t + initialPhase;

    X = amplitude * sinf(phase);
    V = angularFreq * amplitude * cosf(phase);
}

Sin::~Sin(void)
{
    DSPEW();
}

ssize_t Sin::write(void *buffer, size_t len, uint32_t channelNum)
{
    // This filter is a source so there no data passed to
    // whatever called this write().
    //
    DASSERT(buffer == 0, "");

    // TODO: This is pretty much a test.  We don't know yet what is good
    // buffer format, so we'll just start with two floats one for X and
    // one for V.

    len = sizeof(float)*2*samplesPerWrite;
    float *xv = (float *) getBuffer(len);

    for(size_t i=0; i<samplesPerWrite; ++i)
        xv = 0;

    // Send this buffer to the next readers write call.
    writePush(xv, len, ALL_CHANNELS);

    return 1;
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(Sin)
