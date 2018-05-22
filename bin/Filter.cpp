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



const uint32_t CRTSFilter::ALL_CHANNELS = (uint32_t) -1;
const uint32_t CRTSFilter::NULL_CHANNEL = (uint32_t) -2;


CRTSStream::CRTSStream(std::atomic<bool> &isRunning_in):
    isRunning(isRunning_in)
{

}



FilterModule::FilterModule(Stream *stream_in, CRTSFilter *filter_in,
        void *(*destroyFilter_in)(CRTSFilter *), int32_t loadIndex_in,
        std::string name_in):
    stream(stream_in),
    filter(filter_in),
    destroyFilter(destroyFilter_in),
    loadIndex(loadIndex_in),
    outputs(0), inputs(0),
    currentInput(0),
    numOutputs(0), numInputs(0),
    advancedInput(false),
    name(name_in),
    thread(0)
{
    // This is the main thread.
    DASSERT(pthread_equal(Thread::mainThread, pthread_self()), "");
    DASSERT(filter, "");

    filter->filterModule = this;

    filter->stream = new CRTSStream(stream->isRunning);
    name += "(";
    name += std::to_string(loadIndex);
    name += ")";
    DSPEW();
}


FilterModule::~FilterModule(void)
{
    // At this time the CRTSFilter object still exists and we will destroy
    // it in the end of this function, but first while the filter still
    // exists we need to do a few things.

    DASSERT(stream, "");
    DASSERT(filter, "");

    DASSERT(!numOutputs || outputs, "");
    DASSERT(!numInputs || inputs, "");

    // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
    //
    // TODO: Editing of the filter stream is not supported yet.
    //
    // We need to make changes to the adjacent filters here too.  But
    // since we do not support connect changes after the first start, this
    // will work for now.
    //
    // We can at least free object and memory.
    //
    // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX


    // Free Outputs

    DASSERT(numOutputs || !outputs, "");

    for(uint32_t i=0; i<numOutputs; ++i)
        delete outputs[i];

    if(outputs)
        free(outputs);


    // Free Inputs

    DASSERT(numInputs || !inputs, "");

    for(uint32_t i=0; i<numInputs; ++i)
    {
        Input *input = inputs[i];
        DASSERT(input, "");

        delete input;
    }

    if(inputs)
        free(inputs);

    if(thread)
    {
        DASSERT(thread->filterModule != this,
            "this filter module has a thread set to call its' write()");
        thread->filterModules.remove(this);
    }

     if(destroyFilter)
        // Call the CRTSFilter factory destructor function that we got
        // from loading the plugin.
        destroyFilter(filter);
    else
        // It was not loaded from a plugin.
        delete filter;

    stream->map.erase(loadIndex);


    DSPEW("deleted filter: \"%s\"", name.c_str());

    // The std::string name is part of the object so is automatically
    // destroyed.
}



CRTSFilter::~CRTSFilter(void)
{
    //controllers.clear();
    DASSERT(pthread_equal(Thread::mainThread, pthread_self()), "");

    while(controls.size())
        delete controls.rbegin()->second;

    DSPEW();
}


CRTSFilter::CRTSFilter(void):
    // We use this pointer variable as a flag before we use it to be the
    // pointer to the Filtermodule, just so we do not have to declare
    // another variable in CRTSFilter.  See FilterModule::FilterModule().
    _totalBytesIn(0), _totalBytesOut(0)
{
    DASSERT(pthread_equal(Thread::mainThread, pthread_self()), "");
    DSPEW();
}

uint64_t CRTSFilter::totalBytesIn(uint32_t inChannelNum) const
{
    DASSERT(filterModule, "");
    if(inChannelNum == ALL_CHANNELS)
        return _totalBytesIn;
    DASSERT(filterModule->numInputs > inChannelNum, "");
    DASSERT(filterModule->inputs[inChannelNum], "");
    return filterModule->inputs[inChannelNum]->totalBytesIn;
}

uint64_t CRTSFilter::totalBytesOut(uint32_t outChannelNum) const
{
    DASSERT(filterModule, "");
    if(outChannelNum == ALL_CHANNELS)
        return _totalBytesOut;
    DASSERT(filterModule->numOutputs > outChannelNum, "");
    DASSERT(filterModule->outputs[outChannelNum], "");
    return filterModule->outputs[outChannelNum]->totalBytesOut;
}


Output::Output(FilterModule *fM, FilterModule *toFM):
    input(0),
    toFilterModule(toFM),
    ringBuffer(0)
{
    reset();
}


bool CRTSFilter::isSource(void)
{
    // We assume that the Feed filter does not call this.
    DASSERT(filterModule->numInputs > 0, "");
    DASSERT(filterModule->inputs, "");
    DASSERT(filterModule->inputs[0]->output, "");

    // The Feed filter has an output with no ring buffer, so a source
    // filter will be will have an input from a Feed filter with no ring
    // buffer.  So this ...
    //
    return !(filterModule->inputs[0]->output->ringBuffer);
}

// If ownerOutput is 0 this will set it to the owner output.
// else it will use it as the owner output.
//
void FilterModule::createOutputBuffer(size_t maxLength,
        uint32_t outputChannelNum, Output * &ownerOutput,
        bool isPassThrough)
{
    DSPEW();
    DASSERT(pthread_equal(Thread::mainThread, pthread_self()), "");
    DASSERT(maxLength, "");
    DASSERT(filter, "");
    DASSERT(outputChannelNum <= numOutputs, "");
    DASSERT(outputs, "");

    Output *output = outputs[outputChannelNum];

    DASSERT(output, "");
    DASSERT(output->input, "");
    DASSERT(output->ringBuffer == 0, "");
    DASSERT(output->maxLength == 0,
            "the output channel %" PRIu32
            " is already set up.", outputChannelNum);
    DASSERT(output->input, "");
    DASSERT(output->toFilterModule, "");
    DASSERT(output->toFilterModule != this, "");

    output->maxLength = maxLength;
    output->isPassThrough = isPassThrough;

#ifdef DEBUG
    if(isPassThrough)
    {
        DASSERT(ownerOutput, "");
    }
#endif

    if(ownerOutput)
    {
        DASSERT(ownerOutput->ringBuffer, "");
        output->ringBuffer = ownerOutput->ringBuffer;
    }
    else
    {
        // ownerOutput is the owner in a set of outputs
        // that share the same ring buffer.
        //
        DASSERT(output->ringBuffer == 0, "");
        output->ringBuffer = new RingBuffer(output);
        ownerOutput = output;
        output->ringBuffer->ownerOutput = output;
    }

    output->isPassThrough = isPassThrough;

    if(!ownerOutput) ownerOutput = output;
}



void CRTSFilter::createOutputBuffer(size_t maxLength,
        uint32_t outputChannelNum)
{
    DASSERT(filterModule, "");

    Output *ownerOutput = 0;

    if(outputChannelNum != ALL_CHANNELS)
        filterModule->createOutputBuffer(maxLength,
                outputChannelNum, ownerOutput);
    else
        for(uint32_t i=0; i<filterModule->numOutputs; ++i)
            filterModule->createOutputBuffer(
                    maxLength, i, ownerOutput);

    // Note: the memory for the ring buffer is not allocated until after
    // all filter start() functions are called so that we can compute the
    // total size of it, based on all the filters requested maxLength
    // values.
}


void CRTSFilter::createOutputBuffer(size_t maxLength,
        const uint32_t *outputChannelNums)
{
    DASSERT(filterModule, "");
    DASSERT(outputChannelNums, "");
    DASSERT(*outputChannelNums != NULL_CHANNEL, "");

    int loopCount = 0;
    Output *ownerOutput = 0;
    
    while(*outputChannelNums != NULL_CHANNEL)
    {
        ASSERT(++loopCount < 1000, "Too many buffers requested.");

        filterModule->createOutputBuffer(maxLength, *outputChannelNums++,
                ownerOutput);
    }
}

void CRTSFilter::createPassThroughBuffer(
                uint32_t inputChannelNum,
                uint32_t outputChannelNum,
                size_t maxLength,
                size_t thresholdLength)
{
    DASSERT(filterModule, "");
    DASSERT(inputChannelNum <= filterModule->numInputs, "");
    DASSERT(outputChannelNum <= filterModule->numOutputs, "");
    DASSERT(maxLength >= thresholdLength, "");

    // Find the output channel of the up-stream filter that is feeding our
    // input.  That's not from outputChannelNum, which leads to the
    // output from inputChannelNum.
    Output *feedOutput = 0;

    Input *input = filterModule->inputs[inputChannelNum];
    Output *output = filterModule->outputs[outputChannelNum];

    // Flow is like so:
    //
    //    feedOutput (from up stream filter) ==> input ==> output
    //
    DASSERT(output, "");
    DASSERT(input, "");
    DASSERT(output->input != input, "");


    for(auto it : filterModule->stream->map)
    {
        FilterModule *filterModule = it.second;
        for(uint32_t i=filterModule->numOutputs-1; i<(uint32_t)-1; --i)
            if(filterModule->outputs[i]->input == input)
            {
                feedOutput = filterModule->outputs[i];
                break;
            }
    }

    DASSERT(feedOutput, "");
    DASSERT(feedOutput->ringBuffer, "");
    DASSERT(feedOutput->ringBuffer->ownerOutput, "");


    filterModule->createOutputBuffer(maxLength, outputChannelNum,
            feedOutput->ringBuffer->ownerOutput,
            true/*is pass through*/);

    input->thresholdLength = thresholdLength;

    // Note: the memory for the ring buffer is not allocated until after
    // all filter start() functions are called so that we can compute the
    // total size of it, based on all the filters requested maxLength
    // and thresholdLength values that effect the reading and writing of
    // the buffer.
}


static inline void
AdvanceInput(size_t len, Input *input, FilterModule *fm)
{
    DASSERT(input, "");
    DASSERT(input->output, "");
    DASSERT(input->output->ringBuffer, "");
    // Do not, under any circumstances, overrun the buffer.
    ASSERT(len <= input->unreadLength, "");

    if(len)
    {
        input->unreadLength -= len;
        input->output->ringBuffer->advancePointer(input->readPoint, len);
    }
    fm->advancedInput = true;
}

#if 0
// TODO: This may or may not make sense.
//
void CRTSFilter::advanceInput(size_t len, uint32_t inputChannelNum)
{
    DASSERT(!pthread_equal(Thread::mainThread, pthread_self()), "");
    DASSERT(filterModule, "");
    DASSERT(filterModule->numInputs >= inputChannelNum, "");

    AdvanceInput(len, filterModule->inputs[inputChannelNum], filterModule);
}
#endif


void CRTSFilter::advanceInput(size_t len)
{
    DASSERT(!pthread_equal(Thread::mainThread, pthread_self()), "");
    DASSERT(filterModule, "");
    DASSERT(filterModule->numInputs, "");

    AdvanceInput(len, filterModule->currentInput, filterModule);
}


void CRTSFilter::setMaxUnreadLength(size_t len, uint32_t inputChannelNum)
{
    DASSERT(pthread_equal(Thread::mainThread, pthread_self()),
            "You can only call this in start() and stop().");
    DASSERT(filterModule, "");
    DASSERT(filterModule->inputs, "");
    DASSERT(filterModule->numInputs > inputChannelNum ||
            inputChannelNum == ALL_CHANNELS, "");

    if(inputChannelNum == ALL_CHANNELS)
    {
        for(uint32_t i=filterModule->numInputs-1; i<(uint32_t)-1; --i)
        {
            DASSERT(filterModule->inputs[i], "");
            DASSERT(filterModule->inputs[i]->thresholdLength <= len, "");
            filterModule->inputs[i]->maxUnreadLength = len;
        }
    }
    else
    {
        DASSERT(filterModule->inputs[inputChannelNum], "");
        DASSERT(filterModule->inputs[inputChannelNum]->thresholdLength
                <= len, "");
        filterModule->inputs[inputChannelNum]->maxUnreadLength = len;
    }
}


void CRTSFilter::setChokeLength(size_t len, uint32_t inputChannelNum)
{
    DASSERT(pthread_equal(Thread::mainThread, pthread_self()),
            "You can only call this in start() and stop().");
    DASSERT(filterModule, "");
    DASSERT(filterModule->inputs, "");
    DASSERT(filterModule->numInputs > inputChannelNum ||
            inputChannelNum == ALL_CHANNELS, "");

    if(inputChannelNum == ALL_CHANNELS)
    {
        for(uint32_t i=filterModule->numInputs-1; i<(uint32_t)-1; --i)
        {
            DASSERT(filterModule->inputs[i], "");
            DASSERT(filterModule->inputs[i]->thresholdLength <= len, "");
            filterModule->inputs[i]->chokeLength = len;
        }
    }
    else
    {
        DASSERT(filterModule->inputs[inputChannelNum], "");
        DASSERT(filterModule->inputs[inputChannelNum]->thresholdLength
                <= len, "");
        filterModule->inputs[inputChannelNum]->chokeLength = len;
    }

}


void CRTSFilter::setInputThreshold(size_t len, uint32_t inputChannelNum)
{
    DASSERT(pthread_equal(Thread::mainThread, pthread_self()),
            "You can only call this in start() and stop().");
    DASSERT(filterModule, "");
    DASSERT(filterModule->inputs, "");
    DASSERT(filterModule->numInputs > inputChannelNum ||
            inputChannelNum == ALL_CHANNELS, "");

    if(inputChannelNum == ALL_CHANNELS)
    {
        for(uint32_t i=filterModule->numInputs-1; i<(uint32_t)-1; --i)
        {
            DASSERT(filterModule->inputs[i], "");
            filterModule->inputs[i]->thresholdLength = len;
        }
    }
    else
    {
        DASSERT(filterModule->inputs[inputChannelNum], "");
        filterModule->inputs[inputChannelNum]->thresholdLength = len;
    }
}


// This is to get the buffers next write position.
//
// This cannot be called for passThrough buffers.
//
// The user only needs to get the buffer once for each shared ring buffer
// set of buffers.
//
void *CRTSFilter::getOutputBuffer(uint32_t outputChannelNum)
{
    DASSERT(!pthread_equal(Thread::mainThread, pthread_self()), "");
    DASSERT(filterModule, "");
    DASSERT(filterModule->outputs, "");
    DASSERT(filterModule->numOutputs > outputChannelNum, "");

    Output *output = filterModule->outputs[outputChannelNum];

    DASSERT(output, "");
    DASSERT(output->maxLength, "");
    DASSERT(output->ringBuffer, "");
    DASSERT(output->ringBuffer->start, "");

    DASSERT(output->ringBuffer->writePoint >= output->ringBuffer->start, "");
    DASSERT(output->ringBuffer->writePoint <=
            output->ringBuffer->start + output->ringBuffer->length, "");

    return (void *) output->ringBuffer->writePoint;
}


void FilterModule::InputOutputReport(FILE *file)
{
    fprintf(file,
"=======================================================================\n"
"                         Filter \"%s\"\n"
"\n"
"    All Channels:  Input: %" PRIu64 " bytes  Output: %" PRIu64 " bytes\n"
"   -----------------------------------------------------------------\n",
        name.c_str(), filter->totalBytesIn(), filter->totalBytesOut());

    for(uint32_t i=0; i<numInputs; ++i)
        fprintf(file, "    Input Channel %" PRIu32 " <== %" PRIu64 " bytes\n",
                i, filter->totalBytesIn(i));

    fprintf(file,
"   -----------------------------------------------------------------\n");
 

    for(uint32_t i=0; i<numOutputs; ++i)
        fprintf(file, "   Output Channel %" PRIu32 " ==> %" PRIu64 " bytes\n",
                i, filter->totalBytesOut(i));

    fprintf(file,
"=======================================================================\n\n"
            );
}


bool FilterModule::callStopForEachOutput(void)
{
    if(stopped)
        // We have been here before, so we do not call stop() again.
        return false;

    bool ret = filter->stop(numInputs, numOutputs);

    // Call all the Controller stop() functions for this filter.
    for(auto const &controlIt: filter->controls)
        for(auto const &controller: controlIt.second->controllers)
        {
            try
            {
                controller->stop(controlIt.second);
            }
            catch(std::string str)
            {
                // The offending filter start() will likely spew too.
                //
                WARN("Controller \"%s\" stop() through an"
                        " exception and failed:\n%s",
                        controller->getName(), str.c_str());
                ret = true; // One or more filter stop() failed
            }
            catch(...)
            {
                // The offending will likely spew too.
                //
                WARN("Controller \"%s\" stop() through an"
                        " exception and failed",
                        controller->getName());
                ret =  true; // One or more filter stop() failed
            }
        }

    // If any stop() call fails (returns true) we have it all return true
    // (fail).

    stopped = true; // Mark flag, we called stop().

    // Recure.  Stop() the children, but only once per filter, hence the
    // stopped flag above.
    //
    for(uint32_t i=0; i<numOutputs; ++i)
        if(outputs[i]->toFilterModule->callStopForEachOutput())
            ret = true;
    
    return ret; // error state = true if any filter failed.
}
