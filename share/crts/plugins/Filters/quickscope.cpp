#include <stdio.h>
#include <pthread.h>
#include <queue>

#include <quickscope.h>

#include "crts/debug.h"
#include "crts/Filter.hpp"


// Make an X Y scope plot of the data from write() assuming that in coming
// data is a pair of 2 floats, x, y.
//
class Quickscope : public CRTSFilter
{
    public:

        Quickscope(int argc, const char **argv);
        ~Quickscope(void);
        ssize_t write(void *buffer, size_t bufferLen, uint32_t channelNum);

        struct QsSource *source;
};


static pthread_t pthread; // quickscope widget thread.

// mutex to control access to buffers queue just below:
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct BufferEntry
{
    float *buffer;
    int num2Floats; // number of pairs of floats
};


class Buffers
{
    public:

        int getNumFloatPairs(void)
        {
            ASSERT(0 == pthread_mutex_lock(&mutex), "");
            int ret = numFloatPairs;
            // NOTE: The number of Float Pairs will be at least this much,
            // but more could be added after we release this mutex lock.
            ASSERT(0 == pthread_mutex_unlock(&mutex), "");
            return ret;
        };


        void push(void *buffer, size_t len)
        {
            DASSERT(len%(2*sizeof(float)) == 0, "");
            int l = len/(2*sizeof(float));
            struct BufferEntry e = { (float *) buffer, l };
            ASSERT(0 == pthread_mutex_lock(&mutex), "");
            numFloatPairs += l;
            queue.push(e);
            ASSERT(0 == pthread_mutex_unlock(&mutex), "");
        };

        bool getNextBuffer(float* &buffer, int &len)
        {
            if(lastBuffer)
            {
                // Free a previous buffer, or mark it as released to be
                // freed in another thread that we don't know about.
                CRTSFilter::releaseBuffer(lastBuffer);
                lastBuffer = 0;
            }

            // Protect the queue with this mutex.
            ASSERT(0 == pthread_mutex_lock(&mutex), "");
            if(queue.empty())
            {
                ASSERT(0 == pthread_mutex_unlock(&mutex), "");
                buffer = 0;
                len = 0;
                return false; // No more data at this time.
            }

            struct BufferEntry buf = queue.front();
            queue.pop();
            ASSERT(0 == pthread_mutex_unlock(&mutex), "");

            lastBuffer = (void *) (buffer = buf.buffer);
            DASSERT(buffer, "");
            len = buf.num2Floats;
            numFloatPairs -= len;
            DASSERT(len, "");
            return true; // Tell the caller we have more data.
        };

        Buffers(void): lastBuffer(0), numFloatPairs(0),
                mutex(PTHREAD_MUTEX_INITIALIZER)
        {};

        ~Buffers(void)
        {
            float *buffer;
            int len;
            while(getNextBuffer(buffer, len)) ;
        };


    private:

        void *lastBuffer;
        int numFloatPairs;
        pthread_mutex_t mutex;

        // TODO: the use of std::queue will drive into the heap allocator
        // again and again, as we push and pop.  We need to just allocate
        // the memory once and reuse it again and again, without many
        // added calls on the stack due to heap allocator again and
        // again.  Well, it's not like there will be a system call again
        // and again.

        std::queue<struct BufferEntry> queue;

};


static Buffers buffers;


static
int mySource_readCB(struct QsSource *s,
        long double tB, long double tBPrev, long double tCurrent,
        long double deltaT, int nFrames, bool underrun,
        /* qs is the user callback data */
        Quickscope *qs)
{
    if(nFrames == 0)
        // Some kind of scope reset or whatever.
        return 0; // we set no data.

    // Only one thread will access this mySource_readCB()
    // so static is okay.
    static float *buffer = 0; // float pairs array
    static int bufferLen = 0; // number of float pairs from before.
    int numFloatPairs = buffers.getNumFloatPairs() + bufferLen;
  
    // We have been told we can fill in at most nFrames values into the
    // quickscope circular buffer at this call.
    if(nFrames > numFloatPairs)
        // But we only have this many we can fill.
        nFrames = numFloatPairs;

    while(nFrames)
    {
        int n;
        float *vals; // pointer for values to set
        long double *t; // pointer for time array

        n = nFrames;
        vals = qsSource_setFrames(s, &t, &n);

        /* now n is the number of frames (float pairs) we can fill
         * in this vals array that is in the scopes circular buffer. */

        nFrames -= n;
        
        while(n) // until we write n
        {
            if(!bufferLen)
            {
                if(!buffers.getNextBuffer(buffer, bufferLen))
                    // we have no more data from the other thread.
                    return 1; // we must have had data.
            }

            // Copy from buffer to vals

            while(bufferLen && n)
            {
                *vals++ = *buffer++; // copy one float
                *vals++ = *buffer++; // copy another float
                // We have no other scope sources so this should be
                // the master (time-keeping) source.
                DASSERT(deltaT, "");
                if(deltaT)
                    // the quickscope likes having the idea of absolute
                    // time and we need to tell it what time it is.
                    // We don't really care if the time is synced with
                    // real-time (wall clock time), or not.
                    *t++ = (tCurrent += deltaT);

                --bufferLen; // one pair from buffer
                --n; // one pair to quickscope
            }

            // keep going until we write n pairs.
        }


    }

    return numFloatPairs?1:0; // we have data = 1, else 0
}


void *pthreadCB(Quickscope *qs)
{
    /* We'll make it read slowly just see we can play. */
    qsInterval_create(0.016F /* interval period seconds */);

    // qsApp is a global that the quickscope API makes.

    qsApp->op_fade = true; // fade on
    qsApp->op_fadePeriod = 3.5F; // seconds
    qsApp->op_fadeDelay = 4.0F; // seconds

    qs->source = (struct QsSource *) qsSource_create(
            (QsSource_ReadFunc_t) mySource_readCB,
            2 /* numChannels */,
            1000 /* maxNumFrames  per cb_read() call */,
            0 /* source Group 0 = create new Group*/,
            0/*object size default = 0*/);

    /* We'll make a source that can vary the source sample rate.*/
    const float minMaxSampleRates[] = { 0.01F , 2*44100.0F };
    qsSource_setFrameRateType(qs->source, QS_TOLERANT, minMaxSampleRates,
        100.0F/*Hz, requested frame sample rate, that's 50 points plotted
               per second */);
    qsSource_setCallbackData(qs->source, qs);

    /* create a trace */
    qsTrace_create(NULL /* QsWin, NULL to use a default Win */,
        qs->source, 0, /*X-source, channel number*/
        qs->source, 1, /*Y-source, channel number*/
      0.4F, 0.4F,/* xscale, yscale*/
      0.0F, 0.0F, /* xshift, yshift */
      true, /* lines */ 1, 0, 1 /* RGB line color */);

    qsApp_main(); // This is call gtk_main() which loops forever until ...

    qs->stream->isRunning = false;

    return 0;
}

// Counter to keep from starting more than one quickscope.
static int loadCount = 0;

Quickscope::Quickscope(int argc, const char **argv)
{
    DSPEW();

    if(!loadCount)
        ASSERT(pthread_create(&pthread, 0,
                (void *(*) (void *)) pthreadCB, (void *) this) == 0, "");

    ++loadCount;
}


Quickscope::~Quickscope(void)
{
    if(loadCount == 1)
        ASSERT(pthread_join(pthread, 0) == 0, "");

    --loadCount;

    DSPEW();
}


ssize_t Quickscope::write(void *buffer, size_t len, uint32_t channelNum)
{
    DASSERT(buffer, "");
    DASSERT(len, "");
    DASSERT(len%(2*sizeof(float)) == 0, "not any even number of float values");

    // Protect the queue with this mutex lock.
    ASSERT(0 == pthread_mutex_lock(&mutex), "");

    // Put the buffer in the queue.
    buffers.push(buffer, len);

#if 0
    // TODO: we need to deal with being feed data to fast.  There may
    // be manys ways to handle this.
    if(buffers.size() > 10)
    {

    }
#endif
    // Increment the buffer use count for the above quickscope thread will
    // pull this buffer out of the queue.  The quickscope thread will call
    // CRTSFilter::releaseBuffer() to free it when it's done with it.
    incrementBuffer(buffer);

    ASSERT(0 == pthread_mutex_unlock(&mutex), "");
 


    return 1;
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(Quickscope)
