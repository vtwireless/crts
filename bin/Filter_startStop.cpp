#include <map>
#include <list>

#include "crts/debug.h"
#include "crts/Filter.hpp" // CRTSFilter user module interface
#include "crts/crts.hpp"
#include "pthread_wrappers.h" 
#include "Feed.hpp"
#include "FilterModule.hpp"
#include "Thread.hpp"
#include "Stream.hpp"
#include "makeRingBuffer.hpp"



// We need a larger ring buffer if we have different threads accessing it
// between adjacent filters.  We only need to count the given set of
// outputs once if it has a different thread from the filter feeding it.
//
static size_t RingBufferCountThreads(const FilterModule *fm,
        const RingBuffer *ringBuffer)
{
    bool gotOne = false;
    size_t count = 0;

    for(uint32_t i=fm->numOutputs-1; i<(uint32_t)-1; --i)
    {
        Output *output = fm->outputs[i];
    
        DASSERT(output, "");
        DASSERT(fm != output->toFilterModule, "");

        if(output->ringBuffer == ringBuffer)
        {
            if(fm->thread != output->toFilterModule->thread)
            {
                // We only add one at a given filter (fm).
                if(!gotOne)
                {
                    ++count;
                    gotOne = true;
                }
            }
            count += RingBufferCountThreads(fm->outputs[i]->toFilterModule,
                    ringBuffer);
        }
    }
    return count;
}


// Using recursion, this flows from filter to filter along given ring
// buffer from output to input to output to input and so on until it is at
// the end of the flow for that ring buffer.  Along the way we find the
// hungriest channel; data hungry that is.
//
static void GetRingBufferMaxLength(const FilterModule *fm,
        const RingBuffer *ringBuffer,
        size_t &maxLength)
{
    for(uint32_t i=fm->numOutputs-1; i<(uint32_t)-1; --i)
    {
        Output *output = fm->outputs[i];

        if(output->ringBuffer == ringBuffer)
        {
            if(output->maxLength > maxLength)
                maxLength = output->maxLength;
            if(output->input->maxUnreadLength > maxLength)
                maxLength = output->input->maxUnreadLength;
        }

        GetRingBufferMaxLength(output->toFilterModule,
                ringBuffer, maxLength);
    }
}


static void SetupRingBuffer(const FilterModule *fm,
        const RingBuffer *ringBuffer)
{
    for(uint32_t i=fm->numOutputs-1; i<(uint32_t)-1; --i)
    {
        Output *output = fm->outputs[i];

        if(output->ringBuffer == ringBuffer)
        {
            Input *input = output->input;
            input->readPoint = ringBuffer->start;
            input->unreadLength = 0;

            // Nothing to do for output.

            SetupRingBuffer(output->toFilterModule, ringBuffer);
        }

    }
}



// At this point the RingBuffer objects exist and their connections are
// made, but they have no size and memory mappings.
//
// TODO: We need to check how to get the ring buffers the minimum size
// such that the writer never overruns a slow reader.  We may have it
// correct but we're not quite sure.
//
void FilterModule::initRingBuffer(RingBuffer *ringBuffer)
{
    size_t maxLength = 1;
    size_t threadCount = RingBufferCountThreads(this, ringBuffer);

    DASSERT(ringBuffer, "");
    DASSERT(ringBuffer->start == 0, "");
    DASSERT(ringBuffer->ownerOutput, "");

    //////////////////////////////////////////////////////////////////////
    // First calculate the size of the ring buffer based on the size of
    // all the Outputs using it and the thread partitioning.
    //////////////////////////////////////////////////////////////////////

    GetRingBufferMaxLength(this, ringBuffer, maxLength);

    // If there are any different threads accessing the ring buffer we
    // must make it larger for queuing at the filters in the other
    // threads.
    //
    ringBuffer->length = (threadCount * 3 + 2) * maxLength;
    ringBuffer->overhang = maxLength;

    ringBuffer->maxLength = maxLength;

    // makeRingBuffer() will round length and overhang up to the nearest
    // pagesize.
    //
    ringBuffer->start = (uint8_t *)
        makeRingBuffer(ringBuffer->length, ringBuffer->overhang);
    DSPEW("Filter \"%s\" created ring buffer mapping "
            "length=%zu bytes and overhang=%zu bytes",
            name.c_str(), ringBuffer->length, ringBuffer->overhang);
    ringBuffer->writePoint = ringBuffer->start;

    // Initialize the inputs and outputs that use this ring buffer.
    //
    SetupRingBuffer(this, ringBuffer);
}


RingBuffer::~RingBuffer(void)
{
    if(start)
        freeRingBuffer(start, length, overhang);

    length = 0;
    overhang = 0;
    maxLength = 0;
    ownerOutput = 0;
    writePoint = 0;
    start = 0;
}
