#ifndef __Filter_h__
#define __Filter_h__

#include <inttypes.h>
#include <pthread.h>
#include <list>
#include <atomic>

#include <crts/MakeModule.hpp>


// FilterModule is a opaque module thingy that the user need not worry
// about much.  We just needed it to add stuff to the CRTSFilter that we
// wanted to do some lifting without being seen by the user interface.
// CRTSFilter is the particular users filter implementation that has a
// standard CRTSFilter interface.  FilterModule just adds more genetic
// code to the CRTSFilter making it a software plugin filter module which
// you string together to build a CRTS filter "Stream".
//
class FilterModule;


// CRTSStream is a user interface to set and get attributes of the Stream
// which is the related to the group of Filters that are connected (write
// and read) to each other.  There is a pointer to a CRTSStream in the
// CRTSFilter called stream.
//
class CRTSStream
{
    public:

        // A reference to the Stream isRunning flag.
        std::atomic<bool> &isRunning;

    private:

        // The CRTSFilter user does not make a CRTSStream.  It gets made
        // for them, hence this constructor is private so they can't
        // make one.
        CRTSStream(std::atomic<bool> &isRunning);


    friend FilterModule;
};


// The CRTSFilter is the user component module base class.
//
// We use the word filter to refer to general data processing component.
// Systems engineers would likely not want to refer to these
// data processing components as filters, but strictly speaking they are
// software filters, ref:  https://en.wikipedia.org/wiki/Filter_(software),
// so we call them filters which is short for "software filters".
// Some CRTSFilter objects have zero input, but the interface to these
// filters with no inputs is the same as the filters with input, so they
// are filters with null inputs.  We call them source filters.
//
// There are sink filters too. ...
//
//
// The Unix philosophy encourages combining small, discrete tools to
// accomplish larger tasks.  Reference:
//
//       https://en.wikipedia.org/wiki/Filter_(software)#Unix
//
// A stream can be thought of as items on a conveyor belt being processed
// one at a time.  In our case in CRTSFilters are placed at points along
// the conveyor belt to transform the data stream as it flows.
// This idea of "stream" seems to fit what we are doing.  We assemble
// CRTSFilters along our stream.  Reference:
//
//       https://en.wikipedia.org/wiki/Stream_(computing)
//       https://en.wikipedia.org/wiki/Filter_(software)
//

// When these modules run the coding interfaces are such that we do not
// have to know if these modules run on separate threads or not.  Wither
// or not these modules run on different threads is decided at run time.
// This is a requirement so that run/start time optimization is possible.
//
// Sometimes things are so simple that running a single thread is much
// faster than running more than one thread; and sometimes processing in a
// module takes so long that multi-threading, and multi-buffering between,
// the modules is faster.  We can test different filter/thread topologies
// on the fly and the filter writer does not have to be concerned about
// threading, which is a run time decision.  The cost for such flexibility 
// is requiring a particular interface, the CRTSFilter object.
//
// TODO:  It may be possible to make parts of this stream have CRTSFilter
// modules running in different processes, not just threads.  This should
// be able to be added seamlessly at without changing the users CRTSFilter
// code.
//
//
//
//         CRTSFilter trade-offs and considerations:
//
// If unlocked thread looping process time, for more than one thread in
// the stream, is much greater than inter thread contention time per loop,
// then we will get better performance with multi-threading.  If one
// module thread is the bottle-neck (slower than all others) than there
// may be no benefit to multi-threading.
//
// We can put any number of adjacent modules in the same thread.
//
//
// This idea of "stream" seems to fit what we are doing.
// https://en.wikipedia.org/wiki/Filter_(computing)
// 
// Base class for crts_radio IO (stream) modules or or call them stream
// modules.
//
// We are also concerned with sources and sinks of these streams.
// A source of a stream is a CRTSFilter with no data being written to it.
// A sink of a stream is a CRTSFilter with no data being written from it.
//
// This is a stream like thing using only the exposed write() methods
// for each part of the software filter stream.
//
//
// A common filter stream processing interface to seamlessly provide
// runtime optimization of threading and process topology selection.  Kind
// of like in optimization of MPI (massage passing interface) HPC (high
// performance computing) applications, we have node/thread runtime
// partitioning.
//
//
// See REDHAWK
// http://redhawksdr.github.io/Documentation/
//
//
class CRTSFilter
{
    public:

        static const uint32_t ALL_CHANNELS;

        // Function to write data to this filter.
        //
        // This stream gets data when this is called by the "writer" and
        // in response may call this may call the reader->write().  This
        // is how data flows in this group of connected Filter objects.
        //
        // write() must call reader->write() when it wishes to push data
        // along the "stream" because the particular instance is the only
        // thing that knows what data is available to be pushed along.  A
        // CRTSFilter is a software filter with it's own ideas of what it
        // is doing.
        //
        // If this is a source (writer=0, below) this write() will be
        // called with buffer=0.
        //
        // In a sense this write() executes the stream.
        //
        // Clearly the writer (caller of this) dictates the buffer size.
        //
        // channelNum is set to non-zero values in order to merge filter
        // streams.  Most CRTSFilters will not care about channelNum.
        //
        // 0 <= channelNum < N   It's up to the CRTSFilter to decide what
        // to do with channelNum.  A CRTSFilter code that looks at
        // channelNum may be a stream merging filter, or a general stream
        // switching filter.
        virtual
        ssize_t write(
                void *buffer,
                size_t bufferLen,
                uint32_t channelNum=0) = 0;

        virtual ~CRTSFilter(void);

        // The default canWriteBufferIn value is set just here.
        // canWriteBufferIn tells the inner works of CRTSFilter that this
        // module may be writing to the buffer that is passed to
        // CRTSFilter::write(buffer,,).  That info is needed so that the
        // buffer may be shared between threads without the buffer memory
        // getting corrupted by having more than one thread access it
        // at once.
        CRTSFilter(bool canWriteBufferIn = true);

        CRTSStream *stream;


        // releaseBuffer() is not required to be called in
        // CRTSFilter::write().  It is used to free up the buffer that may
        // be being accessed in another thread where by freeing up
        // contention between threads that is associated with the sharing
        // of buffers between threads.  You can call this in your modules
        // CRTSFilter::write() when you know that CRTSFilter::write() is
        // in a part of your code that may take a while and it will not
        // access (read or write) to the buffer again in that call to
        // CRTSFilter::write().
        //
        // What releaseBuffers() does is depends on filter module
        // connection topology and thread grouping.  Using it may or may
        // not make the stream run faster.  It just depends.
        //
        // TODO: This may not be needed given the source filters no longer
        // loop, and return without looping in CRTSFilter::write().
        //void releaseBuffers(void);

        // Releases a buffer lock if this module has a different thread
        // than the module that wrote to this module.  The module may
        // hold more than one lock, so that adjacent buffers may be
        // compared without memory copies.
        //
        // This is automatically done for after CRTSFilter::write().
        //
        static void releaseBuffer(void *buffer);

    protected:

        // User interface to write to the next module in the stream.

        // The CRTSFilter code know if they are pushing to more than on
        // channel.  The channel we refer to here is just an attribute of
        // this filter point (node) in this stream.  ChannelNum goes from
        // 0 to N-1 where N is the total CRTSFilters that are connected
        // to push this data to.  It's up to the writer of the CRTSFilter
        // to define how to push to each channel, and what channel M (0 <=
        // M < N) means.  Do not confuse this with other channel
        // abstractions like a frequency sub band or other software
        // signal channel.  Use channelNum=0 unless this CRTSFilter is
        // splitting the stream.
        //
        // TODO: Named channels and other channel mappings that are not
        // just a simple array index thing.
        //
        // TODO: Varying number of channels on the fly.  This may just
        // work already.
        //
        // If a channel is not "connected" this does nothing or fails.
        //
        // writePush() may be called many times in a given write() call.
        // writePush() manages itself keeping track of how many channels
        // there are just from the calls to writePush(), there is no need
        // to create channels explicitly, but we can manage channels
        // explicitly too.
        //
        // channelNum handles the splitting of the stream.  We can call
        // writePush() many times to write the same thing to many
        // channels; with the cost only being an added call on the
        // function stack.
        void writePush(void *buffer, size_t len, uint32_t channelNum = 0);


        // Returns a buffer if this module has a reader that may be in
        // a different thread.  We will recycle buffers automatically,
        // as the CRTSFilter::write() stack is popped.
        void *getBuffer(size_t bufferLen);



        // Increment the buffer useCount.  Since the buffers created by
        // CRTSFilter::getBuffer() are shared between threads, we needed a
        // buffer use count thingy.  This needs to be called before the
        // buffer is passed to another thread for use.
        void incrementBuffer(void *buffer);


        // TODO: this...
        //
        // This maximum buffer queue length may be needed in the case
        // there one of the filters in the stream is very slow compared to
        // the others, and the feeding filters make a lot of buffers
        // feeding this bottleneck.
        //
        // Think how many total packages can we handle on the conveyor
        // belts, held at the packagers (writers), and held at the
        // receivers (readers).  This is the buffer queue that is between
        // all the modules that access (read or write) this buffer
        // queue.
        //
        // Think: What is the maximum number of packages that will fit on
        // the conveyor belt.
        //void setBufferQueueLength(uint32_t n);
        //
        //static const uint32_t defaultBufferQueueLength;
        

    // The FilterModule has to manage the CRTSFilter adding readers and
    // writers from between separate CRTSFilter objects.  This is better
    // than exposing methods that should not be used by CRTSFilter
    // implementers. Because the user never knows what a FilterModule is,
    // the API (application programming interface) and ABI (application
    // binary interface) never changes when FilterModule changes, whereby
    // making this interface minimal and more stable.
    //
    // For example, we can add new functionality to CRTSFilter by adding
    // code to the FilterModule class, and we would not even have to
    // recompile the users CRTSFilter code, because the users CRTSFilter
    // ABI does not change.  The cost is one pointer deference at most
    // CRTSFilter methods.
    friend FilterModule;  // The rest of the filter code and data.

    private:

        // Pointer to the opaque FilterModule co-object.  The two objects
        // could be one object, except that we need to hide the data and
        // methods in the FilterModule part of it.  So CRTSFilter is the
        // "public" user interface and FilterModule is the ugly opaque
        // co-class that does the heavy lifting, which keeps the exposed
        // parts of CRTSFilter smaller.
        FilterModule *filterModule;
};


#define CRTSFILTER_MAKE_MODULE(derived_class_name) \
    CRTS_MAKE_MODULE(CRTSFilter, derived_class_name)

#endif // #ifndef __Filter__
