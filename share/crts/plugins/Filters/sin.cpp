#include <stdio.h>
#include <inttypes.h>
#include <math.h>
#include <time.h>

#include "crts/debug.h"
#include "crts/Filter.hpp"
#include "rk4.hpp"


class Sin : public CRTSFilter, private RK4<float, float, float>
{
    public:

        Sin(int argc, const char **argv);
        ~Sin(void);

        ssize_t write(void *buffer, size_t bufferLen,
                uint32_t channelNum);

    protected:

        void setPeriod(float period_in)
        {
            period = period_in;
            angularFreq_2 = 2.0F*M_PI/period;
            angularFreq_2 *= angularFreq_2;
            // This time step, tStep, is for the RK4 ODE solver.
            tStep = period/6.0F;
        };

        float getPeriod(void) { return period; };

        void derivatives(float t, const float *x, float *xDot);


    private:

        // TODO: We don't bother syncing this "signal" to real-time (wall
        // clock time).  For now we just need this for testing, some kind
        // of signal that is semi-regular.  We'll be using low sample
        // rates and low frequencies.  We assume that it takes no time to
        // compute the values, and so all the rates will be off a little
        // and we don't care.  We just need something that changes
        // somewhat regularly so we have a regular changing source of
        // floating point data, with parameters that can be changed.  It's
        // just for testing/development.
        
        long samplePeriod/*nano seconds must be less than 1,000,000,000 */;
        size_t samplesPerWrite;

        // Since we are changing parameters, if we wish to keep continuity
        // to second order in the output we keep X and it's derivative V,
        // and recompute the initialPhase and amplitude in order to keep X
        // and V continuous, any time a parameter changes.
        //
        // Using ODEs (ordinary differential equations) and a solver may
        // make it easier to keep continuity when parameters change and so
        // we can extend this into a more complex system of ODEs.  Nothing
        // would need to be calculated just change the parameters of the
        // ODEs and continuity (C-2) is automatically kept in the state, X
        // and V (x[2]).  If this is extended to more complex system of
        // ODEs this will still be true and the method for changing
        // parameters will be the same.

        float x, v, // phase space dynamic variables
            angularFreq_2, // angular frequency squared
            // period saves having to take a square root of angularFreq_2
            period,
            sampleRate/*Hz*/, t/*time in seconds*/;

        // TODO: add limits to the parameters that can be changed.
};


Sin::Sin(int argc, const char **argv):
    RK4(2/*num degree of freedom*/, 0.0F/*time step gets set in setPeriod()*/),
    samplesPerWrite(10), sampleRate(2.0F)
{
    DSPEW();

    // TODO: Add options parsing to initialize parameters.

    x = 1.0F; // initial x
    v = 0.0F; // initial v = x_dot
    samplePeriod = 1.0e9F/sampleRate; // in nano seconds
}

Sin::~Sin(void)
{
    DSPEW();
}

void Sin::derivatives(float t, const float *x, float *xDot)
{
    xDot[0] = x[1]; // x^\dot = v
    xDot[1] = - angularFreq_2 * x[0]; // v^\dot = - \omega^2 x
}

ssize_t Sin::write(void *buffer, size_t len, uint32_t channelNum)
{
    // This filter is a source so there no data passed to
    // whatever called this write().
    //
    DASSERT(buffer == 0, "");

    struct timespec ts { 0, samplePeriod /*nano seconds*/ };
    nanosleep(&ts, 0);

    // TODO: This is pretty much a test.  We don't know yet what is good
    // buffer format, so we'll just start with two floats one for X and
    // one for V.

    len = sizeof(float)*2*samplesPerWrite;
    float *xv = (float *) getBuffer(len);

    // Set the last state values as the start of this group of time steps.
    // Note: we never get the first initial conditions as output.
    xv[0] = x;
    xv[1] = v;

    // This set of ODEs does not depend on t, so we can start at
    // 0 at each write().
    float dt = 1.0F/(samplesPerWrite * sampleRate);
    size_t i;

    for(i=0; i<samplesPerWrite-1; ++i)
    {
        // TODO: Remove this data copying, which means fix the interface
        // to the rk4 solver.
        go(&xv[i*2], i * dt);
        xv[(i+1)*2] = xv[i*2];
        xv[(i+1)*2+1] = xv[i*2+1];
    }
    go(&xv[i*2], i * dt);

    // Save the last value as the initial conditions for the next write().
    x = xv[2*(samplesPerWrite-1)];
    v = xv[2*(samplesPerWrite-1)+1];

    // Send this buffer to the next readers write call.
    writePush(xv, len, ALL_CHANNELS);

    return 1;
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(Sin)
