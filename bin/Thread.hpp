

// TODO: We may need to make a queue of write requests
// that uses struct WriteRequest like so:
#if 0
struct WriteRequest
{
    // The Filter module that will have it's CRTSFilter::write() called
    // next.  Set to 0 if this is none.
    FilterModule *filterModule;

    // buffer, len, channelNum are for calling 
    // CRTSFilter::write(buffer, len, channelNum)
    //
    void *buffer;

    // Buffer length
    size_t len;

    // Current channel to write.
    uint32_t channelNum;
};
#endif


// A queue of condition variables.
//
// You might ask why are we writing our on queue and not using
// std:queue<>.  The simple answer is that this is much much much faster.
// Adding an entry to std::queue requires memory allocation from the heap,
// we get our memory from the function call stack where we push the entry
// on the queue, in Filter::write().  A call to the heap allocator has to
// do a lot of work compared to pretty much none at all here, the stack
// has to be there anyway.  Like comparing doing nothing at all to doing
// something.  Doing nothing always wins, it's always faster to do nothing
// than something.  Doing this is nothing compared to a heap allocation
// (like malloc()), which would be adding a call stack and searching a
// tree or other data structure.  This does not even add a call on the
// stack, given these (WriteQueue_push() and WriteQueue_pop()) are inline
// functions.
//
struct WriteQueue
{
    pthread_cond_t cond;
    struct WriteQueue *next, *prev;
    // The request that is queued.
    FilterModule *filterModule;
    void *buffer;
    size_t len;
    uint32_t channelNum;
};

static inline void WriteQueue_push(struct WriteQueue* &queue, struct WriteQueue *request)
{
    DASSERT(!request->next, "");
    DASSERT(!request->prev, "");

    if(queue && queue->next)
    {
        // CASE 2: There is 2 or more in the queue
        DASSERT(queue->prev, "");
        DASSERT(queue->prev != queue, "");
        DASSERT(queue->next != queue, "");

        request->next = queue;
        request->prev = queue->prev;

        queue->prev->next = request;
        queue->prev = request;
    }
    else if(queue)
    {
        // CASE 1: There is 1 in the queue.
        DASSERT(!queue->prev, "");

        request->next = queue;
        request->prev = queue;
        queue->next = request;
        queue->prev = request;
    }
    else
    {
        // CASE 0: There are 0 in the queue
        queue = request;
    }
}

// Returns a pointer to the WriteQueue that is next in the queue
// or 0 if there is none
static inline struct WriteQueue *WriteQueue_pop(struct WriteQueue* &queue)
{
    if(!queue) return 0; // CASE 0: There are 0 in the queue.

    struct WriteQueue *ret = queue; // this is returned.

    if(queue->next && queue->next != queue->prev)
    {
        // CASE 3: There are 3 or more in the queue.
        // Connect the two adjacent entries.
        queue->prev->next = queue->next;
        queue->next->prev = queue->prev;
        // move to the next in the queue.
        queue = queue->next;
        // Now we have 1 less in the queue
    }
    else if(queue->next)
    {
        // CASE 2: There are just 2 in the queue.
        queue = queue->next;
        queue->next = 0;
        queue->prev = 0;
        // Now there is 1 in the queue.
    }
    else
    {
        // CASE 1: There is just 1 in the queue.
        queue = 0; // We can set a reference.
        // Now there are 0 in the queue.
    }
    return ret;
}


// It groups filters with a running thread.  There is
// a current filter that the thread is calling CRTSFilter::write()
// with and may be other filters that will be used.
//
// There can be many filter modules associated with a given Thread.
// This is just a wrapper of a pthread and it's associated thread
// synchronization primitives, and a little more stuff.
class Thread
{
    public:

        // Launch the pthread via pthread_create()
        //
        // We separated starting the thread from the constructor so that
        // the user can look at the connection and thread topology before
        // starting the threads.  The thread callback function is called
        // and all the threads in all streams will barrier at the top of
        // the thread callback.
        //
        // barrier must be initialized to the correct number of threads
        // plus any number of added barrier calls.
        //
        // This call doe not handle mutex or w/r locking.
        //
        void launch(pthread_barrier_t *barrier);

        inline void addFilterModule(FilterModule *f)
        {
            DASSERT(f, "");
            filterModules.push_back(f);
            f->thread = this;
        };

        // This call doe not handle mutex or w/r locking.
        //
        inline size_t removeFilterModule(FilterModule *f)
        {
            DASSERT(f, "");
            filterModules.push_back(f);
            f->thread = 0;
            delete f;
            return filterModules.size();
        };

        // Returns the total number of existing thread objects in all streams.
        static inline size_t getTotalNumThreads(void)
        {
            return totalNumThreads;
        }


        Thread(Stream *stream);

        // This will pthread_join() after setting a running flag.
        ~Thread();

        pthread_t thread;
        pthread_cond_t cond;
        pthread_mutex_t mutex;


        // We let the main thread be 0 and this starts at 1
        // This is fixed after the object creation.
        // This is mostly so we can see small thread numbers like
        // 0, 1, 2, 3, ... 8 whatever, not like 23431, 5634, ...
        uint32_t threadNum;

        // Number of these objects created.
        static uint32_t createCount;

        static pthread_t mainThread;

        // This barrier gets passed at launch().
        pthread_barrier_t *barrier;

        // stream is the fixed/associated Stream which contains all filter
        // modules, some may not be in the Thread::filtermodules list.  A
        // stream can have many threads.  threads are not shared across
        // Streams.
        //
        Stream &stream;

        // At each loop we may reset the buffer, len, and channel
        // to call filterModule->filter->write(buffer, len, channel);

        /////////////////////////////////////////////////////////////
        //       All Thread data below here is changing data.
        //       We must have the mutex just above to access these:
        /////////////////////////////////////////////////////////////

        // This is set if another thread is blocked by this thread
        // and is waiting in a Filtermodule::write() call.  The other
        // thread will set this flag and call pthread_cond_wait()
        // and the thread of this object will signal it.
        //
        // An ordered queue, first come, first serve.
        //std::queue<pthread_cond_t *> writeQueue;
        struct WriteQueue *writeQueue;

        // We have two cases, blocks of code, when this thread is not
        // holding the mutex lock:
        //
        //       1) in the block that calls CRTSFilter::write()
        //
        //       2) in the pthread_cond_wait() call
        //
        // We need this flag so we know which block of code the thread is
        // in from the main thread, otherwise we'll call
        // pthread_cond_signal() when it is not necessary.
        //
        //
        // TODO: Does calling pthread_cond_signal() when there are no
        // listeners make a mode switch, or use much system resources?
        // I would think checking a flag uses less resources than
        // calling pthread_cond_signal() when there is no listener.
        // It adds a function call or two to the stack and that must be
        // more work than checking a flag.  If it adds a mode switch than
        // this flags adds huge resource savings.
        //
        bool threadWaiting; // thread calling pthread_cond_wait(cond, mutex)


        // All the filter modules that this thread can call
        // CRTSFilter::write() for.  Only this thread pthread is allowed
        // to change this list after the startup barrier.  This list may
        // only be changed if this thread has the stream::mutex lock in
        // addition to the this objects Thread mutex lock.
        std::list<FilterModule*> filterModules;


        bool hasReturned; // the thread returned from its callback

        // The Filter module that will have it's CRTSFilter::write() called
        // next.  Set to 0 if this is none.
        FilterModule *filterModule;

        // buffer, len, channelNum are for calling 
        // CRTSFilter::write(buffer, len, channelNum)
        //
        void *buffer;

        // Buffer length
        size_t len;

        // Current channel to write.  There is one channel per connection.
        // Channel numbers start at 0 and go to N-1, where N is the number
        // of channels.
        uint32_t channelNum;

    private:

        static size_t totalNumThreads; // total all threads in all streams.
};
