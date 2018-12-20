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



// output() is called in the users (filter module) implementation of
// CRTSFilter::input().  output() pushes the data flow to the filter that
// is connected via output channel number outChannelNum.
//
// The flows effectively start here in this function from Feed.cpp
// Feed::input() which loops and calls output(0, 0).   And all the filters
// in the stream graph do the same, and so it flows.
//
// This is a filter module writer user interface.
//
void CRTSFilter::output(size_t len, uint32_t outChannelNum)
{
    DASSERT(filterModule, "");
    DASSERT(filterModule->filter, "");
    DASSERT(filterModule->numOutputs > 0, "");
    DASSERT(outChannelNum <= filterModule->numOutputs ||
            outChannelNum == ALL_CHANNELS, "");

    Output **outputs = filterModule->outputs;
    DASSERT(outputs, "");
    DASSERT(outputs[0], "");


#ifdef DEBUG
    if(filterModule->numInputs == 0)
        DASSERT(len == 0 &&
                outChannelNum == 0,"");
#endif


    if(filterModule->numInputs == 0)
    {
        // This is the source Feed filter.  It has a while loop in the
        // CRTSFilter::input() call.  All the other filters do not loop
        // like that.
        //
        // Now feed the first "real" filter in the graph calling the, for
        // example, Stdin::FilterModule::write() passing the
        // feed->output[0] stuff no data.
        //
        // Note: we are skipping the Output::writePush() call for the case
        // (like below) of this Feed filter module.  It always runs in the
        // same thread, and therefore eventually just calls the next
        // CRTSFilter::input() on its' stack.
        //
        outputs[0]->toFilterModule->write(0, 
                outputs[0], false/*is different thread*/);
        return;
    }

    // We do have a output channel ring buffer and this is not the Feed.


    if(outChannelNum == ALL_CHANNELS && filterModule->numOutputs == 1)
        // We only have one channel any way.
        // No reason to loop if we don't have too.
        outChannelNum = 0;

    if(len)
        filterModule->advanceWriteBuffer(len, outChannelNum);

    if(outChannelNum != ALL_CHANNELS)
        outputs[outChannelNum]->writePush(len, filterModule);
    else
        // outChannelNum == ALL_CHANNELS
        for(uint32_t i=0; i < filterModule->numOutputs; ++i)
            outputs[i]->writePush(len, filterModule);
}


void FilterModule::launchFeed(void)
{
    DASSERT(pthread_equal(Thread::mainThread, pthread_self()), "");
    DASSERT(outputs, "");
    DASSERT(numOutputs == 1, "");
    DASSERT(outputs[0], "");
    DASSERT(outputs[0]->input, "");
    DASSERT(numInputs == 0, "");
    DASSERT(dynamic_cast<Feed *>(filter), "");
    // This must run in the same thread as the "to" filter.
    DASSERT(thread == outputs[0]->toFilterModule->thread, "");


    MUTEX_LOCK(&thread->mutex);

    DASSERT(thread->threadWaiting, "");

    // signal the thread that is waiting now.
    // The flag thread->threadWaiting and the mutex guarantee
    // that the thread is waiting now.
    ASSERT((errno = pthread_cond_signal(&thread->cond))
                    == 0, "");
    // The thread will wake up only after we release the threads
    // mutex lock down below here then it will act on this
    // input request.
    //
    // The input() request for thread should be empty now.
    //
    DASSERT(!thread->filterModule, "thread %" PRIu32, thread->threadNum);

    thread->filterModule = this;
    thread->input = 0;
    thread->len = 0;

    MUTEX_UNLOCK(&thread->mutex);
}


// input is a pointer to an Input in this filter that is part of this
// filter.
//
void FilterModule::runUsersActions(size_t len, Input *input)
{
    void *buf = 0;
    uint32_t inputChannelNum = CRTSFilter::NULL_CHANNEL;

#ifdef DEBUG
    if(input == 0)
    {
        DASSERT(dynamic_cast<Feed *>(filter), "");
        DASSERT(numOutputs == 1, "");
        DASSERT(numInputs == 0, "");
        DASSERT(outputs[0], "");
        DASSERT(len == 0, "");
    }
    else
    {
        DASSERT(input, "");
        DASSERT(input->output, "");
    }
#endif

    if(input)
        inputChannelNum = input->inputChannelNum;

    currentInput = input;


    if(!len)
    {
        ControlActions(buf, len, inputChannelNum);
        filter->input(buf, len, inputChannelNum);
        currentInput = 0;
        return;
    }

            
    // input is owned by this filter so we can change it here.

    input->unreadLength += len;

    // Add unread/accumulated data to the len that we
    // will pass to the CRTSFilter::input().
    len = input->unreadLength;
    buf = (void *) input->readPoint;


    while(len)
    {
        size_t startingUnreadLen = input->unreadLength;

        // If the filter has a choke length set we may call the
        // filter->write() many times, so that we do not write more than
        // the choke length and any one call.

        if(stream->isRunning && len < input->thresholdLength)
            // We do not bother the filter if we are not
            // at the threshold.
            return;

        size_t lenIn = len;
        if(input->chokeLength && lenIn > input->chokeLength)
            lenIn = input->chokeLength;

        // Unset the advance input flag.
        advancedInput = false;

        ControlActions(buf, lenIn, inputChannelNum);

        filter->input(buf, lenIn, inputChannelNum);

        // Automatically advance the input buffer pointer if the
        // filter->input() did not.
        //
        if(currentInput && !advancedInput)
            filter->advanceInput(lenIn);

        // Now push out the "totalBytesIn" parameters.
        //
        // TODO: Should this be done (N times) in the while loop here, or
        // maybe these totalBytesIn" parameters should be changed before or
        // after the loop.
        //
        if(!filter->isSource() && startingUnreadLen != input->unreadLength)
        {
            filter->setParameter("totalBytesIn", filter->control->totalBytesIn());
            if(numInputs > 1)
            {
                std::string s = "totalBytesIn";
                s += std::to_string(inputChannelNum);
                filter->setParameter(s, filter->control->totalBytesIn(inputChannelNum));
            }
        }

        len -= lenIn;
    }



    // If the controller needs a hook to be called after the last
    // filter->write() so they can use the controller
    // CRTSController::stop() which is called after the last write.
    //

    currentInput = 0;
};



//
// CAUTION: This code is a little tricky.  If do not understand
// multi-threaded code, go home.
//
// This calls the 
//
void FilterModule::write(size_t len, Output *output, bool isDifferentThread)
{
    DASSERT(!pthread_equal(Thread::mainThread, pthread_self()), "");
    DASSERT(output, "");
    // We will be writing from a different FilterModule output to input in
    // this FilterModule.  In other words we called
    // output->toFilterModule->write(len, output, ...)
    //
    DASSERT(output->input, "");


    if(isDifferentThread)
    {
        // The object thread has the pthread we are writing to from the
        // current pthread which is associated with a different Thread
        // object from 
        //
        // We called "to thread"->write() from a different Thread
        // object.
        //
        // The current pthread is not the same as the running pthread
        // associated with the object thread.
        //
        // Can we say it another way?
        //

        // There must be threads for all FilterModules.
        DASSERT(thread, "");

        // In this case this function is being called from a thread
        // that is not from the thread of this object.

        // This is an interesting point in the simulation.  This is where
        // this thread may be blocked waiting for the thread of the "next
        // filter" to finish.

        MUTEX_LOCK(&thread->mutex);

        if(thread->hasReturned)
        {
            DASSERT(!thread->stream.isRunning, "");
            // This just handles the exit race, the main thread just did
            // not read the stream isRunning flag before another thread
            // set it (in CRTFFiler::write()) after it looked.  No
            // problem, we handle that case here.
            DSPEW("Exit race case: this thread is trying to call "
                    "FilterModule::write() to thread %" PRIu32
                    " but thread %" PRIu32 " has returned already",
                    thread->threadNum, thread->threadNum);
            MUTEX_UNLOCK(&thread->mutex);
            return; // do nothing.
        }

        // TODO: Are there any directed graph stream topologies that
        // deadlock because of this queuing?
        //
        //
        if(thread->filterModule || thread->writeQueue)
                // thread->filterModule is the next request
        {
            // CASE: this current pthread blocks waiting for
            // FilterModule::thread in filterThreadWrite().

            // This is the case where this thread must block because there
            // is a write request for the thread (thread) already.
            //
            // TODO: Add one non-blocking queued request per connected
            // thread.  That's what we have now if there was just one
            // connected thread writing to FilterModule::thread.
            //
            // We save one write request for the thread in
            // thread->filterModule, but if there is one already we wait
            // in this block of code here.

            // This signal wait call is very important.  Without it we
            // could end up having this fast CRTSFilter overrunning its
            // slower adjacent (next) CRTSFilter.
            //
            // We use this stack memory to add to the write queue for
            // thread.  It's very convenient that the stack memory comes
            // and goes with the need of the memory.  We pass the pointer
            // to the other thread that will use it to signal this
            // thread with.
            //
            // Much faster than using the heap allocator as with a
            // std::queue<pthread_cond_t *> writeQueue.
            // Or does the std::queue constructor allocate?
            //
            // Whatever, this code works.
            //
            struct WriteQueue entry =
            { 
                PTHREAD_COND_INITIALIZER,
                0, 0, /*next, prev*/

                output->toFilterModule,
                output->input, len
            };

            // Yes we use stack memory for the entry that we queue up.
            WriteQueue_push(thread->writeQueue, &entry);

            // We release the mutex lock and wait for signal from the
            // thread we are waiting to write to:
            ASSERT((errno = pthread_cond_wait(&entry.cond,
                            &thread->mutex)) == 0, "");
            // Now we have the mutex lock again.

            // The thread that called pthread_cond_signal() will have
            // pulled this off the queue.

            // Cleanup if there are any kernel resources associated with a
            // pthread condition thing.  We are guaranteed that no other
            // thread will access it, by virtue that we have been signaled
            // and we have the mutex lock now.
            ASSERT(0 == pthread_cond_destroy(&entry.cond), "");

            MUTEX_UNLOCK(&thread->mutex);
            return;  // We are done.
        }

        /* TODO: We need to add a queue with an entry for each thread
         * that is connected.  With the variables:
            thread->filterModule = this;
            thread->input = input;
            thread->len = len;
          We in effect have a queue with one entry for whatever pthread
          gets here first.
        */

        if(thread->threadWaiting)
            // signal the thread that is waiting now.
            // The flag thread->threadWaiting and the mutex guarantee
            // that the thread is waiting now.
            ASSERT((errno = pthread_cond_signal(&thread->cond))
                    == 0, "");
            // The thread will wake up only after we release the threads
            // mutex lock down below here then it will act on this
            // write request.

        // The write request for thread should be empty now.
        DASSERT(!thread->filterModule, "thread %" PRIu32, thread->threadNum);

        thread->filterModule = output->toFilterModule;
        thread->input = output->input;
        thread->len = len;

        MUTEX_UNLOCK(&thread->mutex);
    }
    else
    {
        // We are not a different thread.  We call the CRTSFilter::write()
        // on this stack below here with the runUsersActions() wrapper.

        // This is being called from a stack of more than one
        // CRTSFilter::write() calls so there is no need to get a mutex
        // lock, the current thread stack keeps CRTSFilter::write()
        // function calls in a sequence without any execution overlap.
        //
        output->toFilterModule->runUsersActions(len, output->input);
    }
}
