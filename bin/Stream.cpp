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



std::list<Stream*> Stream::streams;

uint32_t Stream::streamCount = 0;

// static Stream mutex and condition variables for
// Stream::wait()
//
pthread_mutex_t Stream::mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t Stream::cond = PTHREAD_COND_INITIALIZER;
bool Stream::waiting = false; // flag that main thread is waiting on cond


// We have yet another thread call this, because it catches the exit
// signal, and signal handlers and pthread synchronization primitives do
// not mix well.  The alternative would be to add timeouts to the pthread
// synchronization calls, but that's not a clean design.
//
// See signalExitThreadCB() in crts_radio.cpp.
//
// This call must be thread terminate safe, we can have the thread that
// calls it get terminated at any point in the call.  It has no system
// resources that will persist if the calling thread terminates in the
// middle of calling this.
//
void Stream::signalMainThreadCleanup(void)
{
    // This better not be the main thread
    DASSERT(!pthread_equal(Thread::mainThread, pthread_self()), "");
    // Or any of the other filter thread either.

    MUTEX_LOCK(&mutex);

    for(auto stream: streams)
        // Let the other threads know that we are shutting down.
        // The main thread will handle signaling them if need be.
        stream->isRunning = false;

    if(Stream::waiting)
    {
        // Tell the main thread to wake up and go into cleanup
        // mode.
        ASSERT((errno = pthread_cond_signal(&cond)) == 0, "");
    }

    MUTEX_UNLOCK(&mutex);
}



// Waits until a stream and all it's threads is cleaned up.  Returns the
// number of remaining running streams.
//
// Only the main thread will delete streams and only when the atomic bool
// isRunning is set to false. So we do not need a mutex to check this flag
// because it is atomic and it is only set to false once.  We call when
// all streams use the isRunning flags, a graceful exit.
//
//
// We must has the streams mutex lock before calling this.
//
//
size_t Stream::wait(void)
{
    // We assume that the list of streams is a small list, like
    // the number of cores on the CPU.

    // This is the main thread.
    DASSERT(pthread_equal(Thread::mainThread, pthread_self()), "");

    // Only this thread can change the streams.size().


    // No thread may signal this thread until they
    // get this mutex lock.

    if(streams.size())
    {
        for(auto stream: streams)
        {
            if(!stream->isRunning)
            {
                // It's okay that we edit this streams list, because will
                // will return without going to the next element.
                //
                // We cannot delete the stream while holding the lock.
                // We don't need the lock to delete the stream.
                //
                MUTEX_UNLOCK(&mutex);
                delete stream;
                // Only this thread can change the streams.size() so it's
                // okay to call size().
                return streams.size();
            }
        }
    }
    else
    {
        MUTEX_UNLOCK(&mutex);
        return 0; // We're done.  There are no streams left.
    }

    // All streams have isRunning set or are about to signal us after they
    // get the mutex lock.  The threads that are setting stream->isRunning
    // to false now, will signal us in their last block of code in their
    // pthread callback where they block getting the mutex before
    // signaling.  I.E. this should work in all cases so long as
    // this is the main thread.

    DSPEW("waiting for %" PRIu32 " stream(s) to finish", streams.size());

    waiting = true;

    // Here we loose the mutex lock and wait
    //
    // pthread_cond_wait() will still block after a signal handler is
    // called, so the main thread can catch the exit signal and this will
    // not return.
    errno = pthread_cond_wait(&cond, &mutex);
    ASSERT(errno == 0, "");

    waiting = false;

    DSPEW("Got cond signal after waiting for %" PRIu32 " stream(s)",
            streams.size());

    // Now we have the mutex lock again.

    // The number of streams should not have changed while we where
    // waiting.
    DASSERT(streams.size(), "streams.size()=%zu", streams.size());

    // We should be able to find a stream with isRunning == false.
    for(auto stream: streams)
    {
        if(!stream->isRunning)
        {
            // We cannot delete the stream while holding the lock.
            // We don't need the lock to delete the stream.
            //
            MUTEX_UNLOCK(&mutex);
            delete stream;
            // Only this thread can change the streams.size() so it's okay
            // to call size().
            return streams.size();
        }
    }

    // We should not get here.  It would mean that other threads are
    // deleting streams, and we assumed that that can't happen.  Only the
    // main thread can delete streams.

    DASSERT(0, "There are no streams with isRunning false,"
            " streams.size()=%zu", streams.size());

    MUTEX_UNLOCK(&mutex);

    // We should not get here.

    return streams.size();
}


// Compiles the list of sources after connections are finished being
// added.
void Stream::getSources(void)
{
    // This is the main thread.
    DASSERT(pthread_equal(Thread::mainThread, pthread_self()), "");

    sources.clear();
    for(auto &val : map)
        if(val.second->isSource())
            // It has no writers so val->second is a FilterModule that is
            // a source.
            sources.push_back(val.second);

    // We need there to be at least one source FilterModule
    DASSERT(sources.size() > 0, "");
}


Stream::Stream(void):
    isRunning(false), haveConnections(false), loadCount(0),
    streamNum(streamCount++)
{ 
    // This is the main thread.
    DASSERT(pthread_equal(Thread::mainThread, pthread_self()), "");
 
    streams.push_back(this);
    DSPEW("now there are %d Streams", streams.size());
}


// This returns when all the threads in this stream are in the
// pthread_cond_wait() call in filterThreadWrite() in Thread.cpp.
// So we can know that the main thread is the only "running"
// thread when this returns.
//
void Stream::waitForCondWaitThreads(void)
{
    // We must have some threads in this stream.
    DASSERT(threads.begin() != threads.end(), "");

    bool threadNotWaiting = true;
    int loopCount = 0;

    while(threadNotWaiting)
    {
        threadNotWaiting = false;

        // TODO: In general it is usually found that using sleep is the
        // sign of a design flaw.

        // Search all Threads in the stream.
        //
        // 1. First get a lock of all the threads, crazy but we must,
        // otherwise we would could have one changing the waiting flag
        // while looking at another thread waiting flag.
        for(auto tt = threads.begin(); tt != threads.end(); ++tt)
            MUTEX_LOCK(&(*tt)->mutex);

        // 2. Second check if threads are not waiting or have a request.
        for(auto tt = threads.begin(); tt != threads.end(); ++tt)
            if(!(*tt)->threadWaiting
                    ||
                    (*tt)->filterModule/*= have a request*/)
            {
                // This thread (*tt) is not calling pthread_cond_wait() at
                // this time.  So we'll signal it depending on how long
                // we've been waiting so far.
                //
                if(loopCount > 1)
                {
                    // This is in case the thread is stuck in to something
                    // like a blocking read(2) call; otherwise this could
                    // wait forever.
                    //
                    // If this call fails there not much we can do about
                    // it, and it's not a big deal.
                    DSPEW("signaling thread %" PRIu32 " with signal %d",
                            (*tt)->threadNum, THREAD_EXIT_SIG);
                    pthread_kill((*tt)->thread, THREAD_EXIT_SIG);
                }

                threadNotWaiting = true;
            }

        // If we made it through the above block than all threads are
        // waiting in pthread_cond_wait() and none of them have a
        // request queued.

        // 3. Third unlock all the thread mutexes.
        for(auto tt = threads.begin(); tt != threads.end(); ++tt)
            MUTEX_UNLOCK(&(*tt)->mutex);


        if(!threadNotWaiting) break;

        // We thought this sleeping when in this transit flush mode
        // was better than adding another stupid flag that all the
        // threads would have to look at in every loop when in a
        // non-transit state.  This keeps the worker thread loops
        // simpler.
        struct timespec t { 0/* seconds */, 4000000 /*nano seconds*/};
        // If nanosleep fails it does not matter there's nothing we 
        // could do about it anyway.  A signal could make it fail.
        //
        //DSPEW("Waiting nanosleep() for cleanup");
        nanosleep(&t, 0);

        ++loopCount;
    }

    // All threads are calling pthread_cond_wait() in filterThreadWrite()
    // no other thread is coded to signal and wake them, so we can assume
    // that they are all under this main threads control now.

    DSPEW();
}


Stream::~Stream(void)
{
    DSPEW("cleaning up stream with %" PRIu32 " filters with %zu threads",
            loadCount, threads.size());
    // This is the main thread.
    DASSERT(pthread_equal(Thread::mainThread, pthread_self()), "");
    DASSERT(!isRunning, "");


    // We are in shutdown mode now so being a little slow is okay.
    // We are avoiding added move thread synchronization code to
    // the steady state parts of the code.

    // Now we wait to all the threads to be in the pthread_cond_wait()
    // call in filterThreadWrite() callback.
    //
    // The checking that all threads are waiting has to be possible.
    // Threads can be woken and put to sleep while we at doing this check,
    // so we must have another stinking flag that makes all the threads
    // know that we are in "shutdown mode" which is where we add the main
    // thread mutex locking and unlocking to all the threads
    // pthread_cond_wait() calls in the filterThreadWrite() call.
    //

    // Clearly the main thread is the only thread that has access to the
    // thread list while we are doing this:


    // TODO: This will need to get refactored when we need to be able
    // to restart the filter stream flow.
    stop(); // Call all the filters, in this stream, CRTSFilter::stop().


    if(threads.empty())
    {
        DSPEW("Cleaning up filter modules with no threads set up");
        // We have not thread objects yet so we need to
        // delete the filter modules in the streams.
        while(map.size())
            // We let the filter module edit this map.
            delete map.rbegin()->second;
    }
    else
    {
        // The thread destructor removes itself from the
        // threads list.
        for(auto tt = threads.rbegin();
            tt != threads.rend();
            tt = threads.rbegin())
            // The thread will delete all the filter modules
            // that in-turn destroys the CRTSFilter in the filter
            // module.  This will not edit this threads list.
            delete *tt;
    }

    // Remove this from the streams list
    streams.remove(this);

    DSPEW("now there are %d Streams", streams.size());
}


// Destroy the whole factory.
//
void Stream::destroyStreams(void)
{
    // This is the main thread.
    DASSERT(pthread_equal(Thread::mainThread, pthread_self()), "");

    auto it = streams.begin();
    for(;it != streams.end(); it = streams.begin())
        delete (*it);
}


FilterModule* Stream::load(CRTSFilter *crtsFilter,
        void *(*destroyFilter)(CRTSFilter *),
        const char *name)
{
    // This is the main thread.
    DASSERT(pthread_equal(Thread::mainThread, pthread_self()), "");

    // Add the opaque FilterModule data to the new CRTSFilter.
    FilterModule *m = new FilterModule(this, crtsFilter,
            destroyFilter, loadCount, name);

    map.insert(std::pair<uint32_t, FilterModule*>(loadCount, m));

    ++loadCount;

    return m;
}


// Return false on success.
//
// Each Stream is a factory of filter modules.  stream->load()
// is how we make them.
//
bool Stream::load(const char *name, int argc, const char **argv)
{
    // This is the main thread.
    DASSERT(pthread_equal(Thread::mainThread, pthread_self()), "");

    void *(*destroyFilter)(CRTSFilter *);

    CRTSFilter *crtsFilter = LoadModule<CRTSFilter>(name, "Filters",
            argc, argv, destroyFilter);

    if(!crtsFilter || !destroyFilter)
        return true; // fail

    FilterModule *m = load(crtsFilter, destroyFilter, name);

    // If there was no CRTSControl for this CRTS Filter we will add
    // a default CRTSControl.
    //
    if(m->getControls().size() == 0 && !m->makeControl(argc, argv))
        return true; // fail

    return false; // success
}


// Return false on success.
//
// Each stream manages the connections of filter modules.
//
// Connect based on filter load number, starting with the first filter
// loaded in this stream numbered as 0, and so on.
//
bool Stream::connect(uint32_t from, uint32_t to)
{
    // This is the main thread.
    DASSERT(pthread_equal(Thread::mainThread, pthread_self()), "");

    if(from == to)
    {
        ERROR("The filter numbered %" PRIu32
                " cannot be connected to its self");
        return true; // failure
    }

    //std::map<uint32_t,FilterModule*>::iterator it;

    auto it = this->map.find(from);
    if(it == this->map.end())
    {
        ERROR("There is no filter numbered %" PRIu32, from);
        return true; // failure
    }
    FilterModule *f = it->second;

    it = this->map.find(to);
    if(it == this->map.end())
    {
        ERROR("There is no filter numbered %" PRIu32, to);
        return true; // failure
    }

    return connect(f, it->second);
}


bool Stream::connect(FilterModule *from,  FilterModule *to)
{
    ////////////////////////////////////////////////////////////
    // Connect these two filters in this direction
    // Output from "from" to Input at "to".
    ////////////////////////////////////////////////////////////

    // Currently using arrays to construct this Output and Input list
    // which will allow very fast access, but slow editing.

    from->outputs = (Output **) realloc(from->outputs,
            sizeof(Output *)*(from->numOutputs+1));
    ASSERT(from->outputs, "realloc() failed");
    from->outputs[from->numOutputs] = new Output(from, to);


    to->inputs = (Input **) realloc(to->inputs,
            sizeof(Input *)*(to->numInputs+1));
    ASSERT(to->inputs, "realloc() failed");
    
    // t->numInputs is the input channel number that t will see it's
    // write()s are from.
    //
    to->inputs[to->numInputs] = new
        Input(from->outputs[from->numOutputs],
                to->numInputs/*input Channel Number*/);

    // Connect output to input.
    from->outputs[from->numOutputs]->input = to->inputs[to->numInputs];


    ++from->numOutputs;
    ++to->numInputs;

    // Set this flag so we know there was at least one connection.
    //
    haveConnections = true;


    DSPEW("Connected filters: %s writes to %s",
            from->name.c_str(), to->name.c_str());

    return false; // success
}


// Finish building the default thread groupings for all streams.
// All filter modules must belong to a Thread in a given stream.
//
// This is a static Stream method.
//
void Stream::finishThreads(void)
{
    // This is the main thread.
    DASSERT(pthread_equal(Thread::mainThread, pthread_self()), "");

    // Unless we wish to make this a select(2), poll(2), or epoll(2) based
    // service.  All the source filters need to have a separate thread,
    // because their write() calls can block waiting for data.  That's
    // just how UNIX works, assuming we don't force them to be based on
    // non-blocking I/O (like in GNU radio?), which would be too hard for
    // our level of filter module code writers.
    //
    // So we made the source filters write calls never return until they
    // have no more data, like an end of file condition.  Simple enough.
    // This will prevent an unexpected blocking read(2) call from
    // unintentionally completely blocking any multi-source streams.  We
    // assume that filter module code writers are not sophisticated enough
    // to understand how to code blocking and non-blocking read(2) calls.
    // The parallel source inputs will just work if they are in different
    // threads, irregardless of whither read calls are blocking or not.
    //
    // Broken case: if the running makes separate source filters in the
    // same thread.  One source filter could block the other source filter
    // from reading input data.  By default we avoid this case, but users
    // could force this broken case to happen.
    //
    // If this thread default thread grouping does not work the user
    // should specify one on the command line that will work.
    //
    for(auto stream : streams)
    {
        //
        // For each stream we make sure that all filter modules are part
        // of a thread group; filter modules that were not in a thread
        // group get put in a new thread together.
        //
        // Source filters cannot be put in the same thread as another
        // source filter.
        //
        Thread *newThread = 0;

        // Loop through all filter modules in the stream.
        //
        for(auto it : stream->map)
        {
            FilterModule *filterModule = it.second;

            if(!filterModule->thread)
            {
                // The feed always gets added to the thread of the
                // source filters, so they should already have
                // a thread.
                //
                DASSERT(dynamic_cast<Feed *>(filterModule->filter) == 0, "");

                // It's not in a thread.
                //
                // Source filters cannot be put in the same thread as
                // another source filter.
                //
                if(!newThread || filterModule->isSource())
                    newThread = new Thread(stream);

                // Add this Filtermodule to this thread
                newThread->addFilterModule(filterModule);

                DSPEW("%sfilter \"%s\" added to thread %" PRIu32,
                        (filterModule->isSource())?"source ":"",
                        filterModule->name.c_str(),
                        newThread->threadNum);
            }
        }
    }
}
