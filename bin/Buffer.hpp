
// defining BUFFER_DEBUG will add extra debugging code.


#ifdef BUFFER_DEBUG
// Stuff to check for memory leaks in our buffering system:
//
extern pthread_mutex_t bufferDBMutex;
extern uint64_t bufferDBNum; // Number of buffers that exist
extern uint64_t bufferDBMax; // max Number of buffers that existed
#endif



struct Header
{
#ifdef BUFFER_DEBUG
    uint64_t count;
#endif

#ifdef DEBUG
    uint64_t magic; // So we can see it's one of ours a free time.
#endif

    // If there are threads then this
    // mutex lock is held by the thread calling CTRSFilter::write().
    //pthread_mutex_t mutex;

    // Then useCount drops to zero we recycle this buffer.  useCount is
    // used in a multi-threaded version of reference counting.  Since this
    // struct is only defined in this file, you can follow the use of this
    // useCount in just this file.
    std::atomic<uint32_t> useCount;
    size_t len; // is constant after being created
};


// A struct with added memory to the bottom of it.
struct Buffer
{
    struct Header header; // header must be first in struct

    // This pointer should stay aligned so we can offset
    // to this pointer and set this pointer to any struct
    // or array that we want.

    /* think struct padding HERE */

    uint8_t ptr[1]; // just a pointer that already has 1 byte
    // plus the memory below this struct.  The compiler guarantees that
    // ptr is memory aligned because it is 1 byte in a structure.

    /* think padding HERE TOO */
};

// Size of the buffer with added x bytes of memory.
#define BUFFER_SIZE(x)  (sizeof(struct Buffer) + (x-1))

// To access the top of the buffer from the ptr pointer.
#define BUFFER_HEADER(ptr)\
    ((struct Header*)\
        (\
            ((uint8_t*) ptr) \
                - sizeof(struct Header)\
        )\
    )

// Pointer to ptr in struct Buffer
#define BUFFER_PTR(top)\
    ((void*) (((struct Buffer *) top)->ptr))

// Filters, by default, do not know if they run as a single separate
// thread, of with a group filters that share a thread.  That is decided
// from the thing that starts the scenario which runs the program crts_radio
// via command-line arguments, or other high level interface.  We call it
// filter thread (process) partitioning.
//
// TODO: Extend thread partitioning to thread and process partitioning.
//
// This buffer pool thing is so we can pass buffers between any number of
// filters.  By not having this memory on the stack we enable the filter
// to be able to past this buffer from one thread filter to another, and
// so on down the line.
//
// A filter can choose to reuse and pass through a buffer that was passed
// to it from a previous adjust filter, or it can add another buffer to
// pass up stream using the first buffer as just an input buffer.
//
// So ya, cases are:
//
//   1.  through away the box (buffer); really we recycle it; or
//
//   2.  repackage the data using the same box (buffer)
//
//
#ifdef DEBUG
// TODO: pick a better magic salt
#  define MAGIC ((uint64_t) 1224979098644774913)
#endif



// TODO: Figure out the how to do the simplified case when the
// mutex and conditional is not needed and the filter we write
// is in the same thread.
//
// TODO: Figure out how to seamlessly, from the filter writers
// prospective, do the inter-process filter write case.

// We make a buffer that adds extra header to the top of it.


// This must be thread safe.
//
static inline void freeBuffer(struct Header *h)
{
    DASSERT(h, "");

#ifdef DEBUG
    DASSERT(h->magic == MAGIC, "Bad memory pointer");
    DASSERT(h->len > 0, "");
    h->magic = 0;
    memset(h, 0, h->len);
#endif
#ifdef BUFFER_DEBUG
    MUTEX_LOCK(&bufferDBMutex);
    --bufferDBNum;
    //WARN("freeing buffer(%" PRIu64 ") =%p", bufferDBNum,  h);

    MUTEX_UNLOCK(&bufferDBMutex);
#endif

    free(h);
}


#if 0
void CRTSFilter::setBufferQueueLength(uint32_t n)
{
    filterModule->bufferQueueLength = n;
}
#endif
