#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif
#include <dlfcn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <map>
#include <list>

#include "crts/debug.h"
#include "crts/crts.hpp"
#include "LoadModule.hpp"
#include "pthread_wrappers.h"
#include "crts/crts.hpp"
#include "crts/Filter.hpp"
#include "Feed.hpp"
#include "FilterModule.hpp"
#include "Thread.hpp"
#include "Stream.hpp"
#include "makeRingBuffer.hpp"


// Call filter start() for just this one stream.
//
bool Stream::start(void)
{
    // At this time the connections between the filters is set and the so
    // is the thread partitioning, so we know what all the input and
    // output channels are; but we need to call the CRTSFilter::start()
    // functions telling the filters what the connections are and in turn
    // the CRTSFilter::start() functions will tell us how to buffer the
    // channels.

    // TODO: We need to add the finding of stream sources and the creation
    // of feed filters here.  For now that is done in crts_radio.cpp at
    // the end of parseArgs() and so restart is not possible yet.

    

    ///////////////////////////////////////////////////////////////////////
    // 1: First call the filter start() functions.  The
    //    CRTSFilter::start() functions will set parameters via
    //    CRTSFilter::createOutputBuffer() and
    //    CRTSFilter::creatPassThroughBuffer()
    //
    //    This will create the RingBuffer objects and
    //    set filterModule->output[]->ringBuffer
    //
    for(auto it : map)
    {
        // it.second is a FilterModule pointer
        try
        {
            if(it.second->filter->start(
                    it.second->numInputs, it.second->numOutputs))
            {
                WARN("Calling filter %s start() failed",
                        it.second->name.c_str());
                // We do not call the rest of them.
                return true; // One filter start() failed, we are screwed.
            }
        }
        catch(std::string str)
        {
            // The offending filter start() will likely spew too.
            //
            WARN("Calling filter %s start() through an"
                    " exception and failed:\n%s",
                    it.second->name.c_str(), str.c_str());
            // We do not call the rest of them.
            return true; // One filter start() failed, we are screwed.
        }
        catch(...)
        {
            // The offending will likely spew too.
            //
            WARN("Calling filter %s start() through an"
                    " exception and failed",
                    it.second->name.c_str());
            // We do not call the rest of them.
            return true; // One filter start() failed, we are screwed.
        }
    }


    ///////////////////////////////////////////////////////////////////////
    // 2: Check that all outputs will have a ring buffer.  If a filter
    //    module did not call createOutputBuffer() or
    //    createPassThroughBuffer() for a output channel than there will
    //    be no ring buffer for that output.
    //
    //    This can fail because a filter is not written correctly.
    //
    for(auto it : map)
    {
        // loop through every filter in this stream
        class FilterModule *filterModule = it.second;
        class Output **outputs = filterModule->outputs;

        // Skip any Feed filters.
        //
        if(filterModule->numInputs)
        {
            DASSERT(filterModule->numInputs > 0, "");

            for(uint32_t i=filterModule->numOutputs-1; i<(uint32_t)-1; --i)
            {
                Output *output = outputs[i];
                DASSERT(output, "");

                if(!output->ringBuffer)
                {
                    WARN("Filter \"%s\" output channel %" PRIu32
                            " does not have a buffer. "
                            "Call createOutputBuffer() or "
                            "createPassThroughBuffer() in the"
                            " filter.",
                            filterModule->name.c_str(), i);
                    return true;
                }

                DASSERT(output->ringBuffer->ownerOutput, "");
                DASSERT(output->ringBuffer->start == 0, "");
            }
        }
#ifdef DEBUG
        else
        {
            // This is a Feed filter so:
            //
            DASSERT(dynamic_cast<Feed *>(filterModule->filter), "");
            DASSERT(filterModule->numOutputs == 1, "");
            DASSERT(filterModule->outputs[0], "");
            DASSERT(filterModule->outputs[0]->ringBuffer == 0, "");
        }
#endif
    }


    ///////////////////////////////////////////////////////////////////////
    // 3: Now we set up all the ring buffers, creating the memory
    //    mappings.
    //
    for(auto it : map)
    {
        // loop through every filter in this stream
        class FilterModule *filterModule = it.second;
        class Output **outputs = filterModule->outputs;

        // Skip any Feed filters.
        //
        if(filterModule->numInputs)
        {

            for(uint32_t i=filterModule->numOutputs-1; i<(uint32_t)-1; --i)
            {
                if(outputs[i]->ringBuffer->ownerOutput == outputs[i])
                    filterModule->initRingBuffer(outputs[i]->ringBuffer);
            }
        }
    }


    // TODO: Instead of 2 above
    ///////////////////////////////////////////////////////////////////////
    // 4: Create ring Buffers for the channels that have none yet.  The
    //    filter start() did not request one so we make one for each by
    //    default.
    //


    return false; // success
}


// Call filter stop() for just this one stream.
bool Stream::stop(void)
{
    // TODO: we need to add the finding of stream sources and the
    // destruction of feed filters here. See crts_radio.cpp at the end of
    // parseArgs().

    // TODO: For this "start" and "stop" thing to be useful: We need to
    // add a filter reconnecting mechanism and interface to such a thing.
    // That's a large request, and may require a bit of refactoring.


    bool ret = false;

    // 1: We must stop the filters in reverse order because some filters
    //    may refer to other filters resources.
    //
    for(auto it = map.rbegin(); it != map.rend(); ++it)
        // it.second is a FilterModule pointer
        if(it->second->filter->stop(
                it->second->numInputs, it->second->numOutputs))
        {
            WARN("Calling filter %s stop() failed",
                    it->second->name.c_str());
            ret = true; // fail but keep going.
        }


    // 2: Now we un-map the buffers. In what order does not matter
    // because the stream is not running.
    //
    for(auto it : map)
    {
        class FilterModule *filterModule = it.second;
        class Output **outputs = filterModule->outputs;

        DASSERT(outputs || filterModule->numOutputs == 0,"");

        // Skip any Feed filters.
        //
        if(filterModule->numInputs)
        {
            for(uint32_t i=filterModule->numOutputs-1; i<(uint32_t)-1; --i)
            {
                Output *output = outputs[i];
                DASSERT(output, "");
                DASSERT(output->input, "");
                DASSERT(output->input->output == outputs[i], "");
                DASSERT(output->ringBuffer, "");

                // outputs[i]->reset() will delete the ring buffer if
                // outputs[i]->ringBuffer->ownerOutput == outputs[i].
                //
                outputs[i]->reset();
                outputs[i]->input->reset();

                // RingBuffers and memory mappings can be setup again in
                // the next start.
            }
        }
#ifdef DEBUG
        else
        {
            // This is a Feed filter so:
            //
            DASSERT(dynamic_cast<Feed *>(filterModule->filter), "");
            DASSERT(filterModule->numOutputs == 1, "");
            DASSERT(filterModule->outputs[0], "");
            DASSERT(filterModule->outputs[0]->ringBuffer == 0, "");
            filterModule->outputs[0]->reset();
        }
#endif
    }

    return ret;
}


// Calls the all the CRTSFilter::start() functions
// and allocates buffers, getting ready to run.
//
bool Stream::startAll(void)
{

    // 1.  Check that we have no cycles in the filter streams directed
    //     graph.  We do this now so if this fails we it will kill the
    //     whole program.
    //
    for(auto stream : streams)
        for(auto filterModule : stream->sources)
        {
            int depth = 0;
            if(filterModule->depthTest(depth))
                return true; // fail
        }


    // 2. Now call all the different stream start()s.
    //
    for(auto stream : streams)
        if(stream->start())
            return true; // failed

    for(auto stream : streams)
        stream->isRunning = true;

    return false; // success
}


// Calls the all the CRTSFilter::stop() functions
// and deallocates buffers, getting ready to reconfigure
// for a re-start or exit gracefully.
//
bool Stream::stopAll(void)
{
    bool ret = false; // false == success by default

    for(auto stream : streams)
        stream->isRunning = false;

    for(auto sit = Stream::streams.rbegin();
           sit != Stream::streams.rend(); ++sit)
        if((*sit)->stop())
            ret = true; // set error
    return ret;
}
