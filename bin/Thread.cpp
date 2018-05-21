#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <inttypes.h>
#include <atomic>
#include <map>
#include <list>

#include "crts/debug.h"
#include "crts/crts.hpp"
#include "crts/Filter.hpp" // CRTSFilter user module interface
#include "pthread_wrappers.h"
#include "Feed.hpp"
#include "FilterModule.hpp" // opaque filter co-class
#include "Thread.hpp"
#include "Stream.hpp"


static void threadExitCatcher(int sig)
{
    // We just need this to not kill the thread.
    //
    // If a thread is calling a blocking read call this catcher is called
    // and the read() returns -1 an sets errno = EINTR.  The users
    // CRTSFilter::write() function need to see the read() fail and return
    // from the CRTSFilter::write() call.
    //
    INFO("CRTSFilter Thread Caught signal %d", sig);
}


// This is the pthread_create() callback function.
//
static void *filterThreadWrite(Thread *thread)
{
    DASSERT(thread, "");

    {
        // Setup the exit signal catcher for this thread.
        //
        // This is needed so as to help the CRTSFilter::write() get out of
        // a blocking call like read(2) or select(2).  A corresponding
        // pthread_kill() called in Stream::~Stream(void) in Stream.cpp.
        //
        struct sigaction act;
        memset(&act, 0, sizeof(act));
        act.sa_handler = threadExitCatcher;
        errno = 0;
        ASSERT(sigaction(THREAD_EXIT_SIG, &act, 0) == 0, "");
    }

    // Reference to stream isRunning
    std::atomic<bool> &isRunning = thread->stream.isRunning;

    // Put some constant pointers on the stack.
    pthread_mutex_t* mutex = &(thread->mutex);
    pthread_cond_t* cond = &(thread->cond);

    DASSERT(mutex, "");
    DASSERT(cond, "");
    DASSERT(&Stream::mutex, "");
    DASSERT(&Stream::cond, "");


    // These variables will change at every loop:
    FilterModule *filterModule = thread->filterModule;
    Input *input;
    size_t len;


    // mutex limits access to all the data in Thread starting at
    // Thread::filterModule in the class declaration, which can
    // and will be changed between loops.
    //
    MUTEX_LOCK(mutex);
    DSPEW("thread %" PRIu32 " starting", thread->threadNum);

    DASSERT(!thread->filterModule, "");

    // Now that we have the threads mutex lock we can wait for all
    // the threads and the main thread to be in an "initialized" state.
    //
    // This BARRIER stops the main thread from queuing up thread read
    // events before we have the threads ready to receive them via
    // the pointer, thread->filterModule.
    DASSERT(thread->barrier, "");

    BARRIER_WAIT(thread->barrier);

    // We can't have a request yet, while we hold the lock.
    DASSERT(!thread->filterModule, "");



    while(true) // This loop is the guts of the whole thing.
    {
        if(!filterModule)
        {
            thread->threadWaiting = true;
            // WAITING FOR SIGNAL HERE
            ASSERT((errno = pthread_cond_wait(cond, mutex)) == 0, "");
            // Now we have the mutex lock again.
            thread->threadWaiting = false;

            if(!thread->filterModule)
                // There is no request so this is just a signal to return;
                // this threads work is done.
                break;


            // Receive the orders for this thread.  We need to set local
            // stack variables with the values for this write() request,
            // getting the values while we have the mutex lock.
            filterModule = thread->filterModule;
            input = thread->input;
            len = thread->len;
        }

        // If another thread wants to know, they can look at this pointer
        // to see this thread is doing its next writes, and we mark that
        // we are ready to get the next request if we continue to loop.
        thread->filterModule = 0;

        MUTEX_UNLOCK(mutex);


        // While this thread it carrying out its' orders new orders
        // may be set, queued up, by another thread.
        //
        // This may be a time consuming call, just how much is up
        // to the CRTS filter writer.
        //
        filterModule->runUsersActions(len, input);


        MUTEX_LOCK(mutex);

        // Hi, we're back from a users CRTSFilter::input() call
        // and now we have the mutex lock again.

        if(!isRunning)
        {
            // A call to filterModule->filter->write() in this or another
            // thread unset isRunning flag for this stream.

            // We can be a little slow here, because we are now in a
            // shutdown transient state.
            //
            // The stream is going to be deleted soon.  When it is we will
            // wake up from pthread_cond_wait() above and see that
            // thread->filterModule is not set.
            //
            // The main thread is now waiting for all the threads in the
            // stream to be not busy.
            //
            // Since the threads run asynchronously there could be
            // unwritten data headed to this thread now, just at a
            // different thread now.
            //
            // Since we don't know what thread set the Stream::isRunning
            // to false all the threads must try to wake the main thread,
            // otherwise the main thread could wait forever.
            //
            // This is just in this shutdown transit state, the
            // Stream::isRunning flag is usually set to true so this slow
            // MUTEX_LOCK() pthread_cond_signal() MUTEX_UNLOCK() step is
            // usually skipped.
            //

            MUTEX_LOCK(&Stream::mutex);
            if(Stream::waiting)
                // The main thread is calling pthread_cond_wait().
                ASSERT((errno = pthread_cond_signal(&Stream::cond)) == 0, "");
            MUTEX_UNLOCK(&Stream::mutex);
        }


        // TODO: Add one non-blocking queued request per connected thread.
        // That's what we have now if there is just one connected thread.
        // If we start connecting many threads to a single filter we
        // need to add one non-blocking queue per connected thread.

        struct WriteQueue *writeQueue;

        if(thread->filterModule)
        {
            // CASE 1: We have the next write request.
            //
            // No threads are waiting for a signal because of this request
            // was just dumped in this thread object like so, and the
            // thread that dumped it went right on running.
            //
            // Setup the next write request.
            //
            filterModule = thread->filterModule;
            input = thread->input;
            len = thread->len;
        }
        else if((writeQueue = WriteQueue_pop(thread->writeQueue)))
        {
            // CASE 2: Request in the queue from threads that are waiting,
            // and there is not a request in thread->filterModule.
            //
            // Another thread beat this request in CASE 1.  Now some poor
            // thread is waiting in a pthread_cond_wait() call.
            //

            ASSERT((errno = pthread_cond_signal(&writeQueue->cond)) == 0, "");
            // The other thread that we just signaled will be blocking
            // until we release the thread->mutex lock.

            // The memory for this struct WriteQueue was on the stack in
            // the other thread that we signaled, we just got a pointer to
            // it using the thread mutex as protection.  Pretty neat.

            // Setup the next write request we need and let the requesting
            // thread know that, and we also need any other threads
            // (including this one) know that via thread->filterModule.
            thread->filterModule = filterModule = writeQueue->filterModule;
            input = writeQueue->input;
            len = writeQueue->len;
        }
        else
        {
            // CASE 3: We have no requests yet from anywhere and we will
            // wait above.
            //
            filterModule = 0;
        }
    }

    // Let the main thread know that we are done running this thread.  If
    // another thread sees this as not set than this thread is still
    // running.
    thread->hasReturned = true;

    MUTEX_UNLOCK(mutex);

    // Now signal the master/main thread.
    MUTEX_LOCK(&Stream::mutex);
    DASSERT(isRunning == false, "");

    if(Stream::waiting)
        // The main thread is calling pthread_cond_wait().
        ASSERT((errno = pthread_cond_signal(&Stream::cond)) == 0, "");
    MUTEX_UNLOCK(&Stream::mutex);
 
    DSPEW("thread %" PRIu32 " finished returning", thread->threadNum);

    return 0;
}


void Thread::launch(pthread_barrier_t *barrier_in)
{
    DASSERT(barrier_in, "");
    //DSPEW("Thread %" PRIu32 " creating pthread", threadNum);

    filterModule = 0;
    barrier = barrier_in;
    hasReturned = false;


    errno = 0;
    ASSERT((errno = pthread_create(&thread, 0/*pthread_attr_t* */,
                (void *(*) (void *)) filterThreadWrite,
                (void *) this)) == 0, "");
}


uint32_t Thread::createCount = 0;
pthread_t Thread::mainThread = pthread_self();
size_t Thread::totalNumThreads = 0;


// We should have a write lock on the stream to call this.
// TODO: or add a lock and unlock call to this.
Thread::Thread(Stream *stream_in):
    cond(PTHREAD_COND_INITIALIZER),
    mutex(PTHREAD_MUTEX_INITIALIZER),
    threadNum(++createCount),
    barrier(0),
    stream(*stream_in),
    writeQueue(0),
    hasReturned(false),
    filterModule(0)
{
    ++totalNumThreads;
    DASSERT(pthread_equal(mainThread, pthread_self()), "");
    // There dam well better be a Stream object,
    DASSERT(stream_in, "");
    // and it better be in a running mode.
    stream.threads.push_back(this);
    //DSPEW("thread %" PRIu32, threadNum);
}


void Thread::join(void)
{
    // If there is not barrier set than there never was a thread.
    //
    if(barrier)
    {
        MUTEX_LOCK(&mutex);

        // This thread is not busy and should be waiting
        // on a pthread_cond_wait()
        ASSERT((errno = pthread_cond_signal(&cond)) == 0, "");

        MUTEX_UNLOCK(&mutex);

        DSPEW("waiting for thread %" PRIu32 " to join", threadNum);

        ASSERT((errno = pthread_join(thread, 0/*void **retval */) == 0), "");

        // We use barrier as a flag too.
        barrier = 0;

        // remove this object from the list.
        DSPEW("Thread %" PRIu32 " joined", threadNum);
    }
}


// We should have a write lock on the stream to call this.
// TODO: or add a lock and unlock call to this.
Thread::~Thread()
{
    DASSERT(pthread_equal(mainThread, pthread_self()), "");
    // We better be in stream shutdown mode.
    // TODO: until we make threads more dynamic.
    DASSERT(!stream.isRunning, "");

    DASSERT(!filterModule, "");

    join();

    while(filterModules.size())
        // FilterModule::~FilterModule(void) will remove it.
        delete *(filterModules.rbegin());

    stream.threads.remove(this);
    --totalNumThreads;
}
